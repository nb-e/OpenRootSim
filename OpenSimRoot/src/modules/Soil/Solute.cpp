/*
 Copyright © 2016 Forschungszentrum Jülich GmbH
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted under the GNU General Public License v3 and provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 Disclaimer
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 You should have received the GNU GENERAL PUBLIC LICENSE v3 with this file in license.txt but can also be found at http://www.gnu.org/licenses/gpl-3.0.en.html

 NOTE: The GPL.v3 license requires that all derivative work is distributed under the same license. That means that if you use this source code in any other program, you can only distribute that program with the full source code included and licensed under a GPL license.

 */


#include "Solute.hpp"

#include "../../math/BiCGSTAB.hpp"
//#include <iomanip> /* setprecision */

#include "Mapping.hpp"
/*NLEVEL+1 and eps = 0, explicit scheme, NLEVEL=1 and eps=1 implicit, and NLEVEL=2 and eps=0.5 midpoint. */
#define NLEVEL  2

Solute::Solute(const Watflow & water, const std::string & nameNutrient, const int meshrefinement) :
		d95Solute(nullptr), totSoluteInColumn(nullptr), totSoluteInColumnDissolved(nullptr),
		totSoluteInColumnAbsorbed(nullptr), totSoluteChange(nullptr), massBalanceError_(nullptr),
		totSink_(nullptr), sumTopBoundaryFlux_(nullptr), sumBottomBoundaryFlux_(nullptr),
		totalMineralization_(nullptr), sPrec_(nullptr), sinterception_(nullptr), scPrec_(nullptr),
		water_(water), factor(meshrefinement),
		soluteMesh(factor), mineral(nullptr), // for example mineralization on solute Mesh because it interacts with cSink
		NumNP(soluteMesh.getNumNP()), NumEl(soluteMesh.getNumEl()),
		NumBP(soluteMesh.getNumBP()), x(soluteMesh.getCordX()), y(soluteMesh.getCordY()), z(soluteMesh.getCordZ()),
		soluteMatrix(NumNP), PeCr(5.0), /* TODO what is a good criterion? */
		PeCrMx(0.), Peclet(0.), Courant(0.), dtMaxC(1.e+30), Conc(0., NumNP),  // this is set and used in this class
		cSink(0., NumNP),
		Dispxx(0., NumNP), Dispyy(0., NumNP), Dispzz(0., NumNP), Dispxy(0., NumNP), Dispxz(0., NumNP), Dispyz(0., NumNP),
		ListNE(soluteMesh.getListNE()), bulkDensity(0., NumNP), satDiffusionCoeff(0., NumNP),
		longitudinalDispersivity(0., NumNP), transverseDispersivity(0., NumNP), bulkDens_times_adsorpCoeff(0., NumNP), adsorptionCoeff(0., NumNP), nutrient(nameNutrient),
		//these are currently not used at all.
		/*mu_w(0., NumNP),     First-order rate constant for dissolved phase
		 mu_s(0., NumNP),     First-order rate constant for solid phase
		 gamma_w(0., NumNP),  Zero-order rate constant for dissolved phase
		 gamma_s(0., NumNP),   Zero-order rate constant for solid phase      */
		seedReserves(0), ConVolI(0.), matchmode(1), Nsub(soluteMesh.getNsubel()), fine(factor > 1 ? true : false),
		Ac(0., NumNP), Dif(0.,NumNP) {

	// need matrix structure
	soluteMesh.easy_init_matrix(soluteMatrix);

	//mineralization model
	if (nutrient == "nitrate") {
		mineral = new Mineralization(soluteMesh);
	}

	double Y(0);

	//set d95 solute container
	SimulaBase* p = ORIGIN->getChild("soil")->existingChild(nutrient);
	if (p) {
		SimulaBase *d = p->existingChild("D90", "cm");
		if (d)
			d95Solute = dynamic_cast<SimulaTable<Time>*>(d);

		d = p->existingChild("totalSoluteInColumn", "umol");
		if (d)
			totSoluteInColumn = dynamic_cast<SimulaTable<Time>*>(d);
		d = p->existingChild("totalDissolvedSoluteInColumn", "umol");
		if (d)
			totSoluteInColumnDissolved = dynamic_cast<SimulaTable<Time>*>(d);
		d = p->existingChild("totalAbsorbedSoluteInColumn", "umol");
		if (d)
			totSoluteInColumnAbsorbed = dynamic_cast<SimulaTable<Time>*>(d);

		//todo come up with better names for these
		d = p->existingChild("totalSoluteChange", "umol");
		if (d)
			totSoluteChange = dynamic_cast<SimulaTable<Time>*>(d);

		d = p->existingChild("topBoundaryFluxRate", "umol/day");
		if (d)
			sumTopBoundaryFlux_ = dynamic_cast<SimulaTable<Time>*>(d);
		d = p->existingChild("bottomBoundaryFluxRate", "umol/day");
		if (d)
			sumBottomBoundaryFlux_ = dynamic_cast<SimulaTable<Time>*>(d);

		d = p->existingChild("totalMineralizationRate", "umol/day");
		if (d)
			totalMineralization_ = dynamic_cast<SimulaTable<Time>*>(d);

		d = p->existingChild("massBalanceError", "umol");
		if (d)
			massBalanceError_ = dynamic_cast<SimulaTable<Time>*>(d);

		d = p->existingChild("totalSinkRate", "umol/day");
		if (d)
			totSink_ = dynamic_cast<SimulaTable<Time>*>(d);

	}

	totalNutrientUptake_ = ORIGIN->existingPath("/plants/nitrate/plantNutrientUptake", "umol"); //"umol"
	//todo for backwards compatibility, simply insert this, instead of not calculating the balance
	if (totalNutrientUptake_)
		totalNutrientUptake_->get(0, seedReserves);	//todo these are a problem, when plants are not planted at time 0
	sPrec_ = ORIGIN->existingPath("/environment/atmosphere/precipitation", "cm/day");
	sinterception_ = ORIGIN->existingPath("/atmosphere/interception", "cm/day");
	scPrec_ = ORIGIN->existingPath("/atmosphere/concentrationInPrecipitation", "umol/ml");

	p = ORIGIN->getPath("environment/soil/" + nutrient + "/concentration");
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		Conc[i] = Y;
	}

	p = ORIGIN->getPath("environment/soil/bulkDensity");
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		bulkDensity[i] = Y;
	}
	// Ionic or molecular diffusion coefficient in free water
	p = ORIGIN->getPath("environment/soil/" + nutrient + "/saturatedDiffusionCoefficient");
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		satDiffusionCoeff[i] = Y*(double)soluteMesh.getpotmask()[i];
	}

	p = ORIGIN->getPath("environment/soil/" + nutrient + "/longitudinalDispersivity");
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		longitudinalDispersivity[i] = Y;
	}

	p = ORIGIN->getPath("environment/soil/" + nutrient + "/transverseDispersivity");
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		transverseDispersivity[i] = Y;
	}

	p = ORIGIN->getPath("environment/soil/" + nutrient + "/adsorptionCoefficient");
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		adsorptionCoeff[i] = Y;
	}

	bulkDens_times_adsorpCoeff = bulkDensity * adsorptionCoeff;

	//try to determine initial timestep based on peclet courant numbers
	double dt = 0.001; //just an initial guess to work with

	if (fine) {
		tVector hNew(0., NumNP), Con(0., NumNP), thOld(0., NumNP);

		Mapping::interpolate(water_.getMesh(), water_.gethNew(), hNew, factor);
		Mapping::interpolate(water_.getMesh(), water_.getCon(), Con, factor);
		Mapping::interpolate(water_.getMesh(), water_.getthOld(), thOld, factor);

		tVector Vx(0., NumNP), Vy(0., NumNP), Vz(0., NumNP);
		velocities(hNew, Con, Vx, Vy, Vz);
		dispersion_coeff(dt, thOld, Vx, Vy, Vz);
		peclet_courant(dt, thOld, Vx, Vy, Vz);
	} else {
		tVector Vx(0., NumNP), Vy(0., NumNP), Vz(0., NumNP);
		velocities(water_.gethNew(), water_.getCon(), Vx, Vy, Vz); // set Vx, Vy, Vz
		dispersion_coeff(dt, water_.getthOld(), Vx, Vy, Vz); // set needed in Peclet_courant, and upstream_weighing_factors
		peclet_courant(dt, water_.getthOld(), Vx, Vy, Vz); //used for initial timestep
	}

	// find out what method to use for matching roots to fem nodes
	p = ORIGIN->existingPath("environment/dimensions/rootMatchingMode");
	if (p) {
		std::string mode;
		p->get(mode);
		if (mode == "postma and lynch, 2011") {
			matchmode = 1;
		} else if (mode == "ignore root placement") {
			matchmode = 2;
			msg::warning("Solute: using rootMatchingMode: 'ignore root placement' as set in environment/dimensions/rootMatchingMode");
		} else {
			msg::error("Solute: unknown string for environment/dimensions/rootMatchingMode ");
		}
	}

	//report initial values for solid in the column
	const tVector Qwater(0., NumNP);
	double cPrec(0);
	this->reportTotSolidColumn(0, Qwater, cPrec);
	tVector B(0., NumNP);
	this->cbound_condition(0., 0., Qwater, B, cPrec);
	if (totSink_)
		totSink_->set(0, 0); //normally should be zero, will be reset as soon as we do a timestep,

	this->setrootsurfacevalues(0.);		//initial root surface values.
}

//time t at the start of the timestep and the size of the timestep
void Solute::calculate(const double & t, const double & dt) {

	// if no interpolation, these are empty
	tVector hNew;
	tVector Con;
	tVector thNew;
	tVector thOld;
	tVector ConOld;
	tVector hOld;
	tVector wSink;
	tVector Qwater;
	if (fine) {
		hNew.resize(NumNP);
		hOld.resize(NumNP);
		Con.resize(NumNP);
		ConOld.resize(NumNP);
		thNew.resize(NumNP);
		thOld.resize(NumNP);
		Qwater.resize(NumNP);
		Mapping::interpolate(water_.getMesh(), water_.gethOld(), hOld, factor);
		Mapping::interpolate(water_.getMesh(), water_.gethNew(), hNew, factor);
		Mapping::interpolate(water_.getMesh(), water_.getCon(), Con, factor);
		Mapping::interpolate(water_.getMesh(), water_.getConOld(), ConOld, factor);
		Mapping::interpolate(water_.getMesh(), water_.getthNew(), thNew, factor);
		Mapping::interpolate(water_.getMesh(), water_.getthOld(), thOld, factor);
		wSink.resize(NumNP);
		Mapping::interpolate(water_.getMesh(), water_.getwSink(), wSink, factor);
		Mapping::interpolate(water_.getMesh(), water_.getQ(), Qwater, factor);
		Qwater /= pow(factor, 2);
	}

	const tVector & Fc = (fine) ? wSink : (water_.getwSink());
	const tVector & Q = fine ? Qwater : water_.getQ();

	// (Re-)Initialisation
	tVector B(0., NumNP);
	soluteMatrix.resetToZero(); // set soluteMatrix to zero with the right entries



	//################################################################################   S   ##################################################
	//equation 5.6

	// NLEVEL is 1 or if epsi<1 it is 2
	// epsi should be 0.5 or 1 for 0=explicit method, 0.5 heuns implicit, 1 fully implicit method
#if NLEVEL == 1
	const double eps=1.; //implicit
#else
	const double eps=0.5; //midway point
	const double alf=0.5; //=1-eps
	//const tVector Hhdt  (fine? hOld * alf + hNew * eps   : water_.gethOld() * alf + water_.gethNew() * eps);
	//const tVector Conhdt(fine? ConOld * alf + Con * eps  : water_.getConOld() * alf + water_.getCon() * eps );
	//const tVector Thhdt (fine? thOld * alf + thNew * eps : water_.getthOld() * alf + water_.getthNew() * eps );
	const tVector &Hhdt  (fine? hOld   : water_.gethOld() );
	const tVector &Conhdt(fine? ConOld : water_.getConOld()  );
	const tVector &Thhdt (fine? thOld  : water_.getthOld()  );
	//compute and add (1-e)[S]j*Conc_j to the right hand side, see 5.11
	computeS(true, alf, dt, Thhdt, Hhdt, Conhdt,  Fc, B);
#endif


	//do the step at time=t+dt
	const tVector & theta (fine? thNew  : water_.getthNew()  );
	const tVector & hhead (fine? hNew   : water_.gethNew() );
	const tVector & hconductivity (fine? Con : water_.getCon()  );
	computeS(false, eps, dt, theta, hhead, hconductivity,  Fc, B);//compute and add e[S]j+1 to the the matrix, see eq 5.11


	//################################################################################   f(n)   ##################################################
	//equation 5.7

	//computation of the sink term (S)
	//Note in the manual sink (s) is based on a mixing of ConcOld and ConcNew, depending on method.
	//Note that  the root surface values are concold (at t) and will be extrapolated?
	// to have different behaviour insert values for conc at t+dt before running setSoluteSink
	// by running this->setrootsurfacevalues(t+dt);
	const double t1 = t + dt;
	tVector F(0., NumNP);
	this->setSoluteSink(t, t1, cSink);
	const tVector Gc(cSink * -1.);
	tIndex numSEl = 0; //  number of subelements thetrahedrals
	for (list_const_iter it = soluteMesh.getSubelementlist().begin(); it != soluteMesh.getSubelementlist().end(); ++it) { // Loop over sub-elements
		const tIndex i((*it)[0]), j((*it)[1]), k((*it)[2]), l((*it)[3]);
		const double GcE =  (Gc[i] + Gc[j] + Gc[k] + Gc[l]);
		const double VE(soluteMesh.getSubelemetVolumes()[numSEl]);
		const double FMul(VE / 20.);  // this is a weight, for GcE and F , see Manual Equation 5.8
		F[i] += FMul * (GcE + Gc[i]);
		F[j] += FMul * (GcE + Gc[j]);
		F[k] += FMul * (GcE + Gc[k]);
		F[l] += FMul * (GcE + Gc[l]);
		++numSEl;
	}
	//f(n) goes on the right hand side, see 5.11
	B -=  F;


//################################################################################   Q(n,m)   ##################################################
	///manual Q (see eq5.5) on the diagonal and RHS
	tVector DS(0., NumNP);
	numSEl = 0; //  number of subelements thetrahedrals
	// used to have (ThOld[i]*alf+ThNew[i]*epsi)
#if NLEVEL == 1
		Ac = -theta - bulkDens_times_adsorpCoeff; //implicit, assuming theta at t+dt
#else
		Ac = -(Thhdt+theta)/2. - bulkDens_times_adsorpCoeff; //assuming thhdt is theta at t+0.5*dt
#endif
	for (list_const_iter it = soluteMesh.getSubelementlist().begin(); it != soluteMesh.getSubelementlist().end(); ++it) { // Loop over sub-elements
		//index values of the nodes of this tetrahedal ellement
		const tIndex i((*it)[0]), j((*it)[1]), k((*it)[2]), l((*it)[3]);
		const double VE(soluteMesh.getSubelemetVolumes()[numSEl]);
		const double FMul(VE / 20.);  // see Manual Equation 5.8
		const double AcE =  (Ac[i] + Ac[j] + Ac[k] + Ac[l]);
		DS[i] += FMul * (AcE + Ac[i]);
		DS[j] += FMul * (AcE + Ac[j]);
		DS[k] += FMul * (AcE + Ac[k]);
		DS[l] += FMul * (AcE + Ac[l]);
		++numSEl;
	}
	DS/=dt;
	//Q goes on the right hand side  and onto the diagnol of the matrix, see equation 5.11
	B += DS * Conc;
	for (tIndex i = 0; i < NumNP; ++i) {
		soluteMatrix.addValueUnsafely(i, i, DS[i] );
	}


	//################################################################################   Boundaries   ##################################################

	// Boundary condition
	double cPrec;
	cbound_condition(t+dt/2., eps, Q,  B, cPrec);//precipitation simply based on mid point.

	// Debug only: Look at first matrix
//	    std::ofstream smatr;
//	    smatr << std::scientific;
//	    smatr.open("soluteMatrix.txt");
//	    soluteMatrix.print_sparse(smatr);
//	    smatr.close();
//	    exit(0);

	//################################################################################   Solving   ##################################################



	BiCGSTAB::solve(soluteMatrix, B, Conc, 1.e-8, NumNP, NO_PRECOND); // conc is the solution, B is rhs


	//################################################################################   reporting   ##################################################

	// the SolInf calculations for output
	this->reportTotSolidColumn(t1,Q, cPrec);
	this->setrootsurfacevalues(t1);		//new values are for the end of the timestep

	//check sink did not change dramatically
	//this->setSoluteSink(t, t1, cSink);//todo if we want this, we should avoid that mineralization is called twice.
	//const double err=(cSink+Gc).sum();
	//std::cout<<std::endl<<"at t "<<1000*t<<" to "<<1000*t1<<" csinksum "<<1000*cSink.sum()<<" err "<<1000*err;


	return;
}



//computes Snm as in equation 5.6
void Solute::computeS(const bool forward, const double frac, const double dt, const tVector &theta, const tVector &hhead, const tVector &hconductivity,  const tVector & Fc, tVector &B) {

	//calculate velocity vx,vy,vz
	tVector Vx(0., NumNP), Vy(0., NumNP), Vz(0., NumNP);
	velocities(hhead, hconductivity, Vx, Vy, Vz);
	//dispersion & diffusion is calculated from the velocity
	dispersion_coeff(dt, theta, Vx, Vy, Vz);
	//computes the maximum local Peclet and Courant numbers and the maximum permissible time step
	peclet_courant(dt, theta, Vx, Vy, Vz);
	//weighing factors for all sides of all elements.
	//effects matrix WeTab
#ifdef lUpW
	tVector WeTab0(Nsub * NumEl), WeTab1(Nsub * NumEl), WeTab2(Nsub * NumEl), WeTab3(Nsub * NumEl), WeTab4(Nsub * NumEl), WeTab5(Nsub * NumEl);
	upstream_weighing_factors(0, Vx, Vy, Vz, WeTab0);
	upstream_weighing_factors(0, Vx, Vy, Vz, WeTab1);
	upstream_weighing_factors(0, Vx, Vy, Vz, WeTab2);
	upstream_weighing_factors(0, Vx, Vy, Vz, WeTab3);
	upstream_weighing_factors(0, Vx, Vy, Vz, WeTab4);
	upstream_weighing_factors(0, Vx, Vy, Vz, WeTab5);
#else
	// no artificial dispersion and no upward scaling
	const tVector DPom(dt / 6. / (theta + bulkDens_times_adsorpCoeff));//elementwise
	Dispxx += Vx * Vx * DPom;
	Dispyy += Vy * Vy * DPom;
	Dispzz += Vz * Vz * DPom;
	Dispxy += Vx * Vy * DPom;
	Dispxz += Vx * Vz * DPom;
	Dispyz += Vy * Vz * DPom;
#endif

//       Loop on elements
// in our case these are cm cubes
	tIndex numSEl = 0;
	unsigned int count = 0;
	for (list_const_iter it = soluteMesh.getSubelementlist().begin();
			it != soluteMesh.getSubelementlist().end(); ++it) { // Loop over sub-elements
		//index values of the nodes of this tetrahedal ellement
		const tIndex i((*it)[0]), j((*it)[1]), k((*it)[2]), l((*it)[3]);

		// The volume of a tetrahedron is equal to the determinant formed by
		// writing the coordinates of the vertices as columns and
		// then appending a row of ones along the bottom.

		const double VE(soluteMesh.getSubelemetVolumes()[numSEl]);
		const double Det(VE * 6.);

		// Calculate Velocities - these are simply differences in hhead in x,y and z direction divided
		// by the determinant which is either the volume of the cube or twice the volume of the cube

		const tVector & Bi(soluteMesh.getBi()), &Ci(soluteMesh.getCi()), &Di(
				soluteMesh.getDi());
		const double Vxx(
				(Bi[count] * hhead[i] + Bi[count + 1] * hhead[j]
						+ Bi[count + 2] * hhead[k]
						+ Bi[count + 3] * hhead[l]) / Det);
		const double Vyy(
				(Ci[count] * hhead[i] + Ci[count + 1] * hhead[j]
						+ Ci[count + 2] * hhead[k]
						+ Ci[count + 3] * hhead[l]) / Det);
		const double Vzz(
				((Di[count] * hhead[i] + Di[count + 1] * hhead[j]
						+ Di[count + 2] * hhead[k]
						+ Di[count + 3] * hhead[l]) / Det) + 1); // Kazz=1, det=6*Ve, see eq 4.25 manual, assuming no further anisotropy

		// save velocity based on nodal values
		std::array<double, 4> VxE, VyE, VzE;
		VxE[0] = -hconductivity[i] * Vxx;
		VyE[0] = -hconductivity[i] * Vyy;
		VzE[0] = -hconductivity[i] * Vzz;
		VxE[1] = -hconductivity[j] * Vxx;
		VyE[1] = -hconductivity[j] * Vyy;
		VzE[1] = -hconductivity[j] * Vzz;
		VxE[2] = -hconductivity[k] * Vxx;
		VyE[2] = -hconductivity[k] * Vyy;
		VzE[2] = -hconductivity[k] * Vzz;
		VxE[3] = -hconductivity[l] * Vxx;
		VyE[3] = -hconductivity[l] * Vyy;
		VzE[3] = -hconductivity[l] * Vzz;

		// calculate velocity based on elemental values
		// the vol weighing here is probably at par with the linear assumption above, but probably not worse than 1/4 weighing
		//average conductivity in the element
		const double ConE(
				0.25
						* (hconductivity[i] + hconductivity[j]
								+ hconductivity[k] + hconductivity[l]));

		const double VxEE = -ConE * Vxx;
		const double VyEE = -ConE * Vyy;
		const double VzEE = -ConE * Vzz;

		// Diffusion & dispersion?
		const double SMul1 = -1. / VE / 36.;
		const double SMul2 = VE / 120.;
		const double FcE = Fc[i] + Fc[j] + Fc[k] + Fc[l];
		const double EC1 = 0.25
				* (Dispxx[i] + Dispxx[j] + Dispxx[k] + Dispxx[l]);
		const double EC2 = 0.25
				* (Dispyy[i] + Dispyy[j] + Dispyy[k] + Dispyy[l]);
		const double EC3 = 0.25
				* (Dispzz[i] + Dispzz[j] + Dispzz[k] + Dispzz[l]);
		const double EC4 = 0.25
				* (Dispxy[i] + Dispxy[j] + Dispxy[k] + Dispxy[l]);
		const double EC5 = 0.25
				* (Dispxz[i] + Dispxz[j] + Dispxz[k] + Dispxz[l]);
		const double EC6 = 0.25
				* (Dispyz[i] + Dispyz[j] + Dispyz[k] + Dispyz[l]);

#ifdef lUpW
		//upwind
		//todo this could at the cost of memory be expanded by storing the A arrays.
		std::array<double, 4> Wx = {0., 0., 0., 0.}, Wy = {0., 0., 0., 0.}, Wz = {0., 0., 0., 0.};
		const double W12 = WeTab0[numSEl];
		const double W13 = WeTab1[numSEl];
		const double W14 = WeTab2[numSEl];
		const double W23 = WeTab3[numSEl];
		const double W24 = WeTab4[numSEl];
		const double W34 = WeTab5[numSEl];
		const double A11 = -2. * W12 + 2. * W14 + 2. * W13;
		const double A12 = -2. * W12 + W14 + W13;
		const double A13 = -W12 + W14 + 2. * W13;
		const double A14 = -W12 + 2. * W14 + W13;
		const double A21 = -W23 + 2. * W12 + W24;
		const double A22 = -2. * W23 + 2. * W12 + 2. * W24;
		const double A23 = -2. * W23 + W12 + W24;
		const double A24 = -W23 + W12 + 2. * W24;
		const double A31 = -W34 + W23 - 2. * W13;
		const double A32 = -W34 + 2. * W23 - W13;
		const double A33 = -2. * W34 + 2. * W23 - 2. * W13;
		const double A34 = -2. * W34 + W23 - W13;
		const double A41 = -2. * W14 + W34 - W24;
		const double A42 = -W14 + W34 - 2. * W24;
		const double A43 = -W14 + 2. * W34 - W24;
		const double A44 = -2. * W14 + 2. * W34 - 2. * W24;
		Wx[0] = VxE[0] * A11 + VxE[1] * A12 + VxE[2] * A13 + VxE[3] * A14;
		Wx[1] = VxE[0] * A21 + VxE[1] * A22 + VxE[2] * A23 + VxE[3] * A24;
		Wx[2] = VxE[0] * A31 + VxE[1] * A32 + VxE[2] * A33 + VxE[3] * A34;
		Wx[3] = VxE[0] * A41 + VxE[1] * A42 + VxE[2] * A43 + VxE[3] * A44;
		Wy[0] = VyE[0] * A11 + VyE[1] * A12 + VyE[2] * A13 + VyE[3] * A14;
		Wy[1] = VyE[0] * A21 + VyE[1] * A22 + VyE[2] * A23 + VyE[3] * A24;
		Wy[2] = VyE[0] * A31 + VyE[1] * A32 + VyE[2] * A33 + VyE[3] * A34;
		Wy[3] = VyE[0] * A41 + VyE[1] * A42 + VyE[2] * A43 + VyE[3] * A44;
		Wz[0] = VzE[0] * A11 + VzE[1] * A12 + VzE[2] * A13 + VzE[3] * A14;
		Wz[1] = VzE[0] * A21 + VzE[1] * A22 + VzE[2] * A23 + VzE[3] * A24;
		Wz[2] = VzE[0] * A31 + VzE[1] * A32 + VzE[2] * A33 + VzE[3] * A34;
		Wz[3] = VzE[0] * A41 + VzE[1] * A42 + VzE[2] * A43 + VzE[3] * A44;
#endif

		//for (tIndex j1 = 0; j1 < 4; ++j1) { // here we simply looping through the node points of the element
		unsigned int j1 = 0;
		for (auto i1 : (*it)) {
			//const tIndex i1 = (*it)[j1];
			//for (tIndex j2 = 0; j2 < 4; ++j2) { //here we simply looping through the node points of the element
			unsigned int j2 = 0;
			for (auto i2 : (*it)) {
				//const tIndex i2 = (*it)[j2];
				double m_ij = SMul1
						* (EC1 * Bi[count + j1] * Bi[count + j2]
								+ EC2 * Ci[count + j1] * Ci[count + j2]
								+ EC3 * Di[count + j1] * Di[count + j2]
								+ EC4
										* (Bi[count + j1] * Ci[count + j2]
												+ Ci[count + j1]
														* Bi[count + j2])
								+ EC5
										* (Bi[count + j1] * Di[count + j2]
												+ Di[count + j1]
														* Bi[count + j2])
								+ EC6
										* (Ci[count + j1] * Di[count + j2]
												+ Di[count + j1]
														* Ci[count + j2]));
				m_ij -= (Bi[count + j2] / 30. * (VxEE + VxE[j1] / 4.)
						+ Ci[count + j2] / 30. * (VyEE + VyE[j1] / 4.)
						+ Di[count + j2] / 30. * (VzEE + VzE[j1] / 4.));
#ifdef lUpW
				m_ij -= (Bi[j2] / 240. * Wx[j1] + Ci[j2] / 240. * Wy[j1] + Di[j2] / 240. * Wz[j1]);
#endif

				// see manual ex 5.6 this is part   Ve/120 *(4F+Fn+Fm)(1+δ)
				const double Ftmp = SMul2 * (FcE + (Fc[i1] + Fc[i2]));
				m_ij += Ftmp;
				if (i1 == i2)
					m_ij += Ftmp; //two times F3 on the diagonal.

				if (forward) { //see equation 5.11
					B[i1] -= frac * m_ij * Conc[i2];
				} else {
					soluteMatrix.addValueUnsafely(i1, i2, frac * m_ij);
				}
				++j2;
			} //end of inner loop over ijkl
			++j1;
		} //end of outer loop over ijkl
		count += 4;
		++numSEl;
	} // tetrahedra loop (sub elements)
}





void Solute::peclet_courant(const double & dt, const tVector & theta_, const tVector & Vx, const tVector & Vy, const tVector & Vz) {

	double dtMaxCl=10.;//large causes oscillations, as we do not redo the timestep, only use this for the next timestep.
	// Loop over sub elements
	for (list_const_iter it = soluteMesh.getSubelementlist().begin(); it != soluteMesh.getSubelementlist().end(); ++it) { // Loop over sub-elements
		//index values of the nodes of this tetrahedral element
		const tIndex i((*it)[0]), j((*it)[1]), k((*it)[2]), l((*it)[3]);

		double xmax = std::max( { x[i], x[j], x[k], x[l] });
		double xmin = std::min( { x[i], x[j], x[k], x[l] });
		double ymax = std::max( { y[i], y[j], y[k], y[l] });
		double ymin = std::min( { y[i], y[j], y[k], y[l] });
		double zmax = std::max( { z[i], z[j], z[k], z[l] });
		double zmin = std::min( { z[i], z[j], z[k], z[l] });
		double delX = xmax - xmin;
		double delY = ymax - ymin;
		double delZ = zmax - zmin;
		double DxE = std::max(0.25 * (Dispxx[i] + Dispxx[j] + Dispxx[k] + Dispxx[l]), 1.e-30);
		double DyE = std::max(0.25 * (Dispyy[i] + Dispyy[j] + Dispyy[k] + Dispyy[l]), 1.e-30);
		double DzE = std::max(0.25 * (Dispzz[i] + Dispzz[j] + Dispzz[k] + Dispzz[l]), 1.e-30);
		double VxE = std::fabs(0.25 * (Vx[i] + Vx[j] + Vx[k] + Vx[l]));
		double VyE = std::fabs(0.25 * (Vy[i] + Vy[j] + Vy[k] + Vy[l]));
		double VzE = std::fabs(0.25 * (Vz[i] + Vz[j] + Vz[k] + Vz[l]));
		double VxMax = std::max( { std::fabs(Vx[i]) / theta_[i], std::fabs(Vx[j]) / theta_[j], std::fabs(Vx[k]) / theta_[k], std::fabs(Vx[l]) / theta_[l] });
		double VyMax = std::max( { std::fabs(Vy[i]) / theta_[i], std::fabs(Vy[j]) / theta_[j], std::fabs(Vy[k]) / theta_[k], std::fabs(Vy[l]) / theta_[l] });
		double VzMax = std::max( { std::fabs(Vz[i]) / theta_[i], std::fabs(Vz[j]) / theta_[j], std::fabs(Vz[k]) / theta_[k], std::fabs(Vz[l]) / theta_[l] });

		double PecX = VxE * delX / DxE;
		double PecY = VyE * delY / DyE;
		double PecZ = VzE * delZ / DzE;
		Peclet = std::max( { Peclet, PecX, PecY, PecZ });
		Peclet = std::min(Peclet, 99999.);

		double R1 = 1. + bulkDens_times_adsorpCoeff[i] / theta_[i];
		double R2 = 1. + bulkDens_times_adsorpCoeff[j] / theta_[j];
		double R3 = 1. + bulkDens_times_adsorpCoeff[k] / theta_[k];
		double R4 = 1. + bulkDens_times_adsorpCoeff[l] / theta_[l];
		double RMin = std::min( { R1, R2, R3, R4 });

		double CourX = VxMax * dt / delX / RMin;
		double CourY = VyMax * dt / delY / RMin;
		double CourZ = VzMax * dt / delZ / RMin;
		Courant = std::max( { Courant, CourX, CourY, CourZ });

#if defined(lUpW) || defined(lArtD)
		double Cour1 = 1.0;
		double Cour2 = 1.0;
		double Cour3 = 1.0;
#else
		double Cour1 = std::min(1., PeCr / std::max(0.5, PecX));
		double Cour2 = std::min(1., PeCr / std::max(0.5, PecY));
		double Cour3 = std::min(1., PeCr / std::max(0.5, PecZ));
#endif
		double dt1(VxMax > 0.?	 Cour1 * delX * RMin / VxMax : 1e30);
		double dt2(VyMax > 0.?   Cour2 * delY * RMin / VyMax : 1e30);
		double dt3(VzMax > 0.?   Cour3 * delZ * RMin / VzMax : 1e30);
		dtMaxCl = std::min( { dtMaxCl, dt1, dt2, dt3 });

		PeCrMx = std::max( { PeCrMx, PecX * CourX, PecY * CourY, PecZ * CourZ });

	}

	if(dtMaxC>dtMaxCl){
		dtMaxC=dtMaxCl;
	}else if(dtMaxC<2.*dtMaxCl){
		dtMaxC*=1.01; //try to slowly reduce the restriction
	}//else do nothing.

	return;
}

#ifdef lUpW

void Solute::upstream_weighing_factors(const int j, const tVector & Vx, const tVector & Vy, const tVector & Vz, tVector &WeTab) const {
	tIndex M1(0), M2(0), M3(0), M4(0), NUS(Nsub);
	std::array<tIndex, 4> List;
	double VV, Vxx, Vyy, Vzz, A, B, C, Aleng, cosx, cosy, cosz, cosDel, Delta;
	double Val, Vel, Dxx, Dyy, Dzz, Dxy, Dxz, Dyz, DAL, Disp, aa;

	tIndex numSEl = 0; //  number of subelements thetrahedrals
	// Loop over sub elements
	for (list_const_iter it = soluteMesh.getSubelementlist().begin(); it != soluteMesh.getSubelementlist().end(); ++it) { // Loop over sub-elements
		M1 = (*it)[0];
		M2 = (*it)[1];
		M3 = (*it)[2];
		M4 = (*it)[3];

		List[0] = M1;
		List[1] = M2;
		List[2] = M3;
		List[3] = M4;

		WeTab[numSEl] = 0.;

		switch (j) {
			case 0: // TODO nothing to do in this case?
			M1 = List[0];
			M2 = List[1];
			break;
			case 1:
			M1 = List[2];
			M2 = List[0];
			break;
			case 2:
			M1 = List[3];
			M2 = List[0];
			break;
			case 3:
			M1 = List[1];
			M2 = List[2];
			break;
			case 4:
			M1 = List[3];
			M2 = List[1];
			break;
			case 5:
			M1 = List[2];
			M2 = List[3];
			break;
		}

		A = x[M2] - x[M1];
		B = y[M2] - y[M1];
		C = z[M2] - z[M1];
		Aleng = std::sqrt(A * A + B * B + C * C);
		cosx = 1. / Aleng * A;
		cosy = 1. / Aleng * B;
		cosz = 1. / Aleng * C;
		Vxx = (Vx[M1] + Vx[M2]) * 0.5;
		Vyy = (Vy[M1] + Vy[M2]) * 0.5;
		Vzz = (Vz[M1] + Vz[M2]) * 0.5;
		VV = std::sqrt(Vxx * Vxx + Vyy * Vyy + Vzz * Vzz);

		if (std::fabs(VV) < 1.e-30) {
			continue;
		}

		cosDel = 1. / VV / Aleng * (A * Vxx + B * Vyy + C * Vzz);
		Delta = std::acos(std::fabs(cosDel));

		if (NUS != Nsub && Delta > 3.1415926536) {
			continue;
		}

		Val = Vxx * cosx + Vyy * cosy + Vzz * cosz;
		Dxx = (Dispxx[M1] + Dispxx[M2]) * 0.5;
		Dyy = (Dispyy[M1] + Dispyy[M2]) * 0.5;
		Dzz = (Dispzz[M1] + Dispzz[M2]) * 0.5;
		Dxy = (Dispxy[M1] + Dispxy[M2]) * 0.5;
		Dxz = (Dispxz[M1] + Dispxz[M2]) * 0.5;
		Dyz = (Dispyz[M1] + Dispyz[M2]) * 0.5;
		DAL = std::fabs(Dxx * cosx * cosx + Dyy * cosy * cosy + Dzz * cosz * cosz + 2. * Dxy * cosx * cosy + 2. * Dxz * cosx * cosz + 2. * Dyz * cosy * cosz);
		Vel = Val * Aleng;
		Disp = 2.0 * DAL;
		aa = 11.;
		if (Disp > 0.) {
			aa = std::fabs(Vel / Disp);
		}
		if (Disp < 1.e-30 || std::fabs(Vel) < 0.001 * VV || std::fabs(aa) > 10.) {
			if (std::fabs(Vel) < 0.001 * VV) {
				WeTab[numSEl] = 0.0;
			}
			if (Vel > 0.001 * VV) {
				WeTab[numSEl] = 1.0;
			}
			if (Vel < -0.001 * VV) {
				WeTab[numSEl] = -1.0;
			}
		} else {
			WeTab[numSEl] = 1.0 / std::tanh(Vel / Disp) - Disp / Vel;
		}

		++numSEl;
	}

	return;
}
#endif

void Solute::dispersion_coeff(const double & dt, const tVector & theta_, const tVector & Vx, const tVector & Vy, const tVector & Vz) {
//	    tVector Tau(NumNP);
	double DispL, DispT, Vabs;

//		for (tIndex i = 0; i < NumNP; ++i) {
//			// tortuosity factor according to Millington and Quirk [1961]
//			Tau[i] = std::pow(theta_[i], (7. / 3.)) / (thSat[i] * thSat[i]);
//			// diffusion = theta * dif in water * Darcian fluid flux density * tau
//			Dif[i] = theta_[i] * satDiffusionCoeff[i] * Tau[i];
//		}

	tVector thSat;
	if (fine) {
		thSat.resize(NumNP);
		Mapping::interpolate(water_.getMesh(), water_.getthSat(), thSat, factor);
	}
	const tVector Dif =	fine ? (theta_ * satDiffusionCoeff * std::pow(theta_, (7. / 3.)) / std::pow(thSat, 2.)) : 
	(theta_ * satDiffusionCoeff * std::pow(theta_, (7. / 3.))
							/ std::pow(water_.getthSat(), 2.)); // component wise

	// todo this can now be pulled apart into array operations?
	for (tIndex i = 0; i < NumNP; ++i) {
		// dispersion
		DispL = longitudinalDispersivity[i];
		DispT = transverseDispersivity[i];
		Vabs = std::sqrt(Vx[i] * Vx[i] + Vy[i] * Vy[i] + Vz[i] * Vz[i]);
#ifdef lArtD
		if (Vabs > 0.) {
			// adjust dispersion so the Peclet number criterium is met
			DispL = std::max(DispL, Vabs * dt / (theta_[i] + bulkDens_times_adsorpCoeff[i]) / PeCr - Dif[i] / Vabs);
		}
#endif
		Dispxx[i] = Dif[i];
		Dispyy[i] = Dif[i];
		Dispzz[i] = Dif[i];
		Dispxy[i] = 0.;
		Dispxz[i] = 0.;
		Dispyz[i] = 0.;
		if (Vabs > 0.) {
			// velocity is greater than 0
			Dispxx[i] = DispL * Vx[i] * Vx[i] / Vabs + DispT * Vy[i] * Vy[i] / Vabs + DispT * Vz[i] * Vz[i] / Vabs + Dif[i];
			Dispyy[i] = DispL * Vy[i] * Vy[i] / Vabs + DispT * Vx[i] * Vx[i] / Vabs + DispT * Vz[i] * Vz[i] / Vabs + Dif[i];
			Dispzz[i] = DispL * Vz[i] * Vz[i] / Vabs + DispT * Vx[i] * Vx[i] / Vabs + DispT * Vy[i] * Vy[i] / Vabs + Dif[i];
			Dispxy[i] = (DispL - DispT) * Vx[i] * Vy[i] / Vabs;
			Dispxz[i] = (DispL - DispT) * Vx[i] * Vz[i] / Vabs;
			Dispyz[i] = (DispL - DispT) * Vy[i] * Vz[i] / Vabs;
		}
	}
	return;
}

void Solute::velocities(const tVector & hNew_, const tVector & Con_, tVector & Vx, tVector & Vy, tVector & Vz) const {
	double Vxx, Vyy, Vzz, Det;
	std::valarray<tIndex> List(4);
	std::valarray<double> B(4), C(4), D(4);

	// see equation 4.25 in the user manual, computation of nodal fluxes
	// note that we threw out Ka which contains anisotropy of Conductivity values and was
	// present in the original code as conaxx,
	// etc. this is probably only correct for a regular spaced grid with equal distances in all directions

	//Vx = 0.; Vy = 0.; Vz = 0.; // Valarrays assumed to be 0 initialized

	// Loop over sub elements
	tIndex count = 0, sub = 0;
	for (list_const_iter it = soluteMesh.getSubelementlist().begin(); it != soluteMesh.getSubelementlist().end(); ++it) { // Loop over sub-elements
		//index values of the nodes of this tetrahedal ellement
		const tIndex i((*it)[0]), j((*it)[1]), k((*it)[2]), l((*it)[3]);

//	        B[0]=-(y[k]-y[j])*(z[l]-z[j])+(y[l]-y[j])*(z[k]-z[j]);
//	        B[1]=+(y[l]-y[k])*(z[i]-z[k])-(y[i]-y[k])*(z[l]-z[k]);
//	        B[2]=-(y[i]-y[l])*(z[j]-z[l])+(y[j]-y[l])*(z[i]-z[l]);
//	        B[3]=+(y[j]-y[i])*(z[k]-z[i])-(y[k]-y[i])*(z[j]-z[i]);
//	        C[0]=+(x[k]-x[j])*(z[l]-z[j])-(x[l]-x[j])*(z[k]-z[j]);
//	        C[1]=-(x[l]-x[k])*(z[i]-z[k])+(x[i]-x[k])*(z[l]-z[k]);
//	        C[2]=+(x[i]-x[l])*(z[j]-z[l])-(x[j]-x[l])*(z[i]-z[l]);
//	        C[3]=-(x[j]-x[i])*(z[k]-z[i])+(x[k]-x[i])*(z[j]-z[i]);
//	        D[0]=-(x[k]-x[j])*(y[l]-y[j])+(x[l]-x[j])*(y[k]-y[j]);
//	        D[1]=+(x[l]-x[k])*(y[i]-y[k])-(x[i]-x[k])*(y[l]-y[k]);
//	        D[2]=-(x[i]-x[l])*(y[j]-y[l])+(x[j]-x[l])*(y[i]-y[l]);
//	        D[3]=+(x[j]-x[i])*(y[k]-y[i])-(x[k]-x[i])*(y[j]-y[i]);
//	        Det=(x[l]-x[i])*B[3]+(y[l]-y[i])*C[3]+(z[l]-z[i])*D[3];

		Det = soluteMesh.getSubelemetVolumes()[sub] * 6.;

		// tension differences over distance per surface area (and so per Ve)
		const tVector & Bi(soluteMesh.getBi()), &Ci(soluteMesh.getCi()), &Di(soluteMesh.getDi());
		Vxx = (Bi[count] * hNew_[i] + Bi[count + 1] * hNew_[j] + Bi[count + 2] * hNew_[k] + Bi[count + 3] * hNew_[l]) / Det;
		Vyy = (Ci[count] * hNew_[i] + Ci[count + 1] * hNew_[j] + Ci[count + 2] * hNew_[k] + Ci[count + 3] * hNew_[l]) / Det;
		Vzz = ((Di[count] * hNew_[i] + Di[count + 1] * hNew_[j] + Di[count + 2] * hNew_[k] + Di[count + 3] * hNew_[l]) / Det) + 1; // Kazz=1, det=6*Ve, see eq 4.25 manual, assuming no further anisotropy

		Vx[i] -= Vxx;
		Vx[j] -= Vxx;
		Vx[k] -= Vxx;
		Vx[l] -= Vxx;

		Vy[i] -= Vyy;
		Vy[j] -= Vyy;
		Vy[k] -= Vyy;
		Vy[l] -= Vyy;

		Vz[i] -= Vzz;
		Vz[j] -= Vzz;
		Vz[k] -= Vzz;
		Vz[l] -= Vzz;

		count += 4;
		++sub;
	}

	// listNE is number of sub elements the node is a corner of
	// con = Nodal values of the hydraulic conductivity at the new time level
	// basically differences in hydraulic head time conductivity averaged over the number of elements per node and taking above some anisotropy in the conductivity into account
	Vx *= Con_ / ListNE;
	Vy *= Con_ / ListNE;
	Vz *= Con_ / ListNE;

	return;
}


/**
 *  No Dirichlet BC can be set in this code: \n
 *  This is because third-type (von Neumann) conditions, in general,
 *	are physically more realistic and preserve solute mass in the simulated system \n
 *	(e.g., van Genuchten and Parker [ 1984]; Leij et al. [1991]).
 */
void Solute::cbound_condition(const double t, const double eps, const tVector & Q, tVector & B, double & cPrec) {
	//t is tnew
	//Q is Qnew.
	//conc is ConcOld
	const double alf = 1. - eps;

	// concentration in precipitation
	double prec;
	if (sPrec_) {
		sPrec_->get(t, prec);
		prec = std::fabs(prec);
		if (sinterception_) {
			double ic;
			sinterception_->get(t, ic);
			prec -= std::fabs(ic);
		}

	} else {
		prec = 0.;
	}

	if (scPrec_) {
		scPrec_->get(t, cPrec);
		cPrec = std::fabs(cPrec) * prec;
	} else {
		cPrec = 0.;
	}

	auto & width(soluteMesh.getWidth());

	//add precipitation
	for (auto i : soluteMesh.getTopBoundaryNodes()) {
		// evaporation does not cause a solute flux, transpiration has conc=0, so correct
		B[i] += Q[i] * (alf * Conc[i]);
		soluteMatrix.addValueUnsafely(i, i, -eps * Q[i]);
		if (cPrec) {
			msg::warning("Solute: fertigating, check this code first to see if signs are right, balance is correct etc");
			B[i] += width[i] * cPrec;
		}
	}

	return;
}

void Solute::interpolate(const tVector & source, tVector & target, const int factor) const {

	if (2 != factor) {
		msg::error("interpolate: factor != 2");
	}

	const tIndex & nxw(water_.getMesh().getnx());
	const tIndex & nyw(water_.getMesh().getny());

	tIndex NX = (2 * nxw - 1);
	tIndex NY = (2 * nyw - 1);
	tIndex numnp_2x = NX * NY * (water_.getMesh().getnz() * 2 - 1);
	// tIndex numnp_4x = (2*NX-1)*(2*NY-1)*(2*(water_.getMesh().getnz()*2-1)-1);

	// exception if source and target dimensions do not fit together
	if (NumNP != numnp_2x /*|| NumNP != numnp_4x */) {
		msg::error("interpolate: Fine Mesh is not a two potency multiple of coarse mesh");
	}

	tIndex i = 0;
	tIndex counter = 0;
	tIndex row = 1;
	tIndex layer = 1;
	while (counter < numnp_2x /*&& i < sourceNumNP*/) {
		if (i < row * nxw - 1) {
			target[counter] = source[i];
			target[counter + 1] = 0.5 * (source[i] + source[i + 1]);
			counter += 2;
		} else {
			target[counter] = source[i];
			counter += NX + 1;
			++row;
			if (i == layer * nxw * nyw - 1) {
				counter += (NX * (NY - 1));
				++layer;
			}
		}
		++i;
	}

	i = 0;
	counter = NX;
	row = 1;
	tIndex counter2 = 0;
	while (counter < numnp_2x) {
		if (i < row * nxw - 1) {
			target[counter] = 0.5 * (source[i] + source[i + nxw]);
			target[counter + 1] = 0.25 * (source[i] + source[i + 1] + source[i + nxw] + source[i + nxw + 1]);
			counter += 2;
		} else {
			target[counter] = 0.5 * (source[i] + source[i + nxw]);
			counter += NX + 1;
			++row;
			++counter2;
			if (counter2 == nyw - 1) {
				counter += (NX * NY + NX);
				i += nxw;
				++row; // then i is at the end of the layer
				counter2 = 0;
			}
		}
		++i;
	}

	i = 0;
	counter = NX * NY;
	row = 1;
	layer = 1;
	while (counter < numnp_2x) {
		if (i < row * nxw - 1) {
			target[counter] = 0.5 * (source[i] + source[i + nxw * nyw]);
			target[counter + 1] = 0.25 * (source[i] + source[i + 1] + source[i + nxw * nyw] + source[i + 1 + nxw * nyw]);
			counter += 2;
		} else {
			target[counter] = 0.5 * (source[i] + source[i + nxw * nyw]);
			counter += NX + 1;
			++row;
			if (i == layer * nxw * nyw - 1) {
				counter += (NX * (NY - 1));
				++layer;
			}
		}
		++i;
	}

	i = 0;
	counter = NX * NY + NX;
	row = 1;
	counter2 = 0;
	while (counter < numnp_2x) {
		if (i < row * nxw - 1) {
			target[counter] = 0.25 * (source[i + nxw] + source[i] + source[i + nyw * nxw] + source[i + nyw * nxw + nxw]);
			target[counter + 1] = 0.125
					* (source[i + nxw] + source[i] + source[i + nyw * nxw] + source[i + nyw * nxw + nxw] + source[i + nxw + 1] + source[i + 1]
							+ source[i + nyw * nxw + 1] + source[i + nyw * nxw + nxw + 1]);
			counter += 2;
		} else {
			target[counter] = 0.25 * (source[i + nxw] + source[i] + source[i + nyw * nxw] + source[i + nyw * nxw + nxw]);
			counter += NX + 1;
			++row;
			++counter2;
			if (counter2 == nyw - 1) {
				counter += (NX * NY + NX);
				i += nxw;
				++row; // then i is at the end of the layer
				counter2 = 0;
			}
		}
		++i;
	}

	return;

}

// todo right now this method thinks that meshes are the same, here only soluteMesh is used because it has water values anyway.
void Solute::reportTotSolidColumn(const Time & t, const tVector & Q, const double & cPrec) {

	double ConVolSolid(0.); 	/// Amount of solute in the entire flow domain [M](ConVol in Table 9.6).
	double ConVolSolute(0.); 	/// Amount of solute in the entire flow domain [M](ConVol in Table 9.6).

	tVector thNew_water;
	if (fine) {
		thNew_water.resize(NumNP);
		Mapping::interpolate(water_.getMesh(), water_.getthNew(), thNew_water, factor);
	}
	const tVector &thNew = fine ? thNew_water : water_.getthNew();

	tIndex it_subelem = 0;
	for (auto& it : soluteMesh.getSubelementlist()) { // Loop over sub-elements
		//index values of the nodes of this tetrahedral element
		const tIndex i(it[0]), j(it[1]), k(it[2]), l(it[3]);
		const double VE = soluteMesh.getSubelemetVolumes()[it_subelem];

		ConVolSolute += VE / 4. * (thNew[i] * Conc[i] + thNew[j] * Conc[j] + thNew[k] * Conc[k] + thNew[l] * Conc[l]);
		ConVolSolid += VE / 4.  * (bulkDens_times_adsorpCoeff[i] * Conc[i] + bulkDens_times_adsorpCoeff[j] * Conc[j]
								 + bulkDens_times_adsorpCoeff[k] * Conc[k] + bulkDens_times_adsorpCoeff[l] * Conc[l]);
		++it_subelem;
	}
	double ConVol = ConVolSolid + ConVolSolute; /// Amount of solute in the entire flow domain [M](ConVol in Table 9.6).

	//jouke: write metrics to simroot tables
	if (t < 1e-10) {
		ConVolI = ConVol; // set this here once to keep initial results
	}
	if (totSoluteInColumn)
		totSoluteInColumn->set(t, ConVol);
	if (totSoluteInColumnDissolved)
		totSoluteInColumnDissolved->set(t, ConVolSolute);
	if (totSoluteInColumnAbsorbed)
		totSoluteInColumnAbsorbed->set(t, ConVolSolid);
	if (totSoluteChange)
		totSoluteChange->set(t, ConVol - ConVolI);

	//d95 for solute in column
	//todo with a better interface, this could go into a separate class.
	if (d95Solute) {
		double tresh = 0.10 * ConVol;
		double tot(0.), d90(0.);
		for (auto it = soluteMesh.getSubelementlist().rbegin(); it != soluteMesh.getSubelementlist().rend(); ++it) { // Loop over sub-elements
			//index values of the nodes of this tetrahedal ellement
			it_subelem = 0;
			const tIndex i((*it)[0]), j((*it)[1]), k((*it)[2]), l((*it)[3]);
			const double VE = soluteMesh.getSubelemetVolumes()[it_subelem];
			// weights are 1/4th!
			tot += VE / 4. * (thNew[i] * Conc[i] + thNew[j] * Conc[j] + thNew[k] * Conc[k] + thNew[l] * Conc[l]);
			tot += VE / 4. * (bulkDens_times_adsorpCoeff[i] * Conc[i] + bulkDens_times_adsorpCoeff[j] * Conc[j]
							+ bulkDens_times_adsorpCoeff[k] * Conc[k] + bulkDens_times_adsorpCoeff[l] * Conc[l]);

			if (tot > tresh) {
				d90 = std::min(std::min(z[i], z[j]), std::min(z[k], z[l]));
				break;
			}
			++it_subelem;
		}
		d95Solute->set(t, d90); // set the value in the table
	}

	if (sumTopBoundaryFlux_) {
		double sumTopBoundaryFlux(0.);
		if (cPrec) {
			auto & width(soluteMesh.getWidth());
			for (auto i : soluteMesh.getTopBoundaryNodes()) {
				sumTopBoundaryFlux += width[i] * cPrec; //todo this needs checking. Is the sign right?
			}
		} else {
			// evaporation does not cause a solute flux, transpiration has conc=0, so correct
		}
		sumTopBoundaryFlux_->set(t, sumTopBoundaryFlux);
	}

	//report drainage of solute
	if (sumBottomBoundaryFlux_) {
		double cumBottomBoundaryFlux(0);
		for (auto i : soluteMesh.getBottomBoundaryNodes()) {
			cumBottomBoundaryFlux += Q[i] * Conc[i];//todo this should be Q[i] * alf *Conc[i]+Q[i] * eps *Conc_new[i]
		}
		sumBottomBoundaryFlux_->set(t, cumBottomBoundaryFlux);
	}

	return;
}

void Solute::setSoluteSink(const double & t0, const double & t1, tVector & sink) {
	if (t1 <= t0)
		return;
	sink = 0;
	double cSinkCheck(0);
	for (unsigned int i(0); i < femIndexes_.size(); ++i) {
		if (!nutrientUptake_[i]->evaluateTime(t0))
			continue;
		const std::vector<int> &ind = femIndexes_[i];
		if (ind[0] == -1) {
			// ROOTS OUTSIDE GRID no nutrients and no water
			continue;
		} else {
			double nuptake, nu1;
			nutrientUptake_[i]->get(t1, nuptake); // calls MichaelisMenten::MichaelisMenten(SimulaDynamic* pSD) the first time
			nutrientUptake_[i]->get(t0, nu1);
			nuptake -= nu1; //uptake by true differencing
			nuptake /= (t1 - t0); //uptake per day
			if (nuptake < 1e-30)
				continue;
			cSinkCheck += nuptake;

			std::vector<int> const * pind;
			std::vector<int> indexes;
			std::vector<double> weights;
			std::vector<double> const * pweights;
			double sum;
			//double * psum;
			if (ind[0] == -2) {
				//growthpoint, needs matching
				Coordinate pos;
				nutrientUptake_[i]->getParent(2)->get(t0, pos);
				const std::vector<double> &base = femWeights_[i];
				pos.x += base[0];
				pos.y += base[1];
				pos.z += base[2];
				soluteMesh.matchNodes(pos, indexes, weights, sum); //todo: somewhat ugly, depends on order ofthings in several other places
				pind = &indexes;
				pweights = &weights;
				//psum=&sum;
			} else {
				pind = &ind;
				pweights = &femWeights_[i];
				//psum=&femSumWeights_[i];
			}
			if (pind->size() > 1) {
				double sumweights(0);
				for (unsigned int j = 0; j < pind->size(); ++j) {
					int femi = pind->at(j);
					//if(Conc[femi]>0){
					sumweights += Conc[femi] * (*pweights)[j];
					//}
				}
				//std::cout<<std::endl<< nutrientUptake_[i]->getParent(3)->getName()<<" "<<nutrientUptake_[i]->getParent(5)->getName();
				//std::cout.precision(5);
				for (unsigned int j = 0; j < pind->size(); ++j) {
					int femi = pind->at(j);
					//if(Conc[femi]>0){
					sink[femi] += nuptake * Conc[femi] * (*pweights)[j] / sumweights;
					//}
					//std::cout<<std::endl<<nuptake<<" "<<femi<<" "<<conc[femi]<<" "<<Conc[femi]*(*pweights)[j]/sumweights;
				}
			} else {
				//push the first nodal values
				int femi = pind->at(0);
				sink[femi] += nuptake;
			}

		}

	}
	const double sinksum=sink.sum();
	if (std::abs(cSinkCheck) > 1.) {
		double check = 100 * std::abs((cSinkCheck - sinksum) / cSinkCheck);
		if (check > 0.01) {
			msg::error("Solute: sink differs at time=" + std::to_string(t0) + " from uptake by " + std::to_string(check) + " percent.");
		}
	}

	if (totSink_){
		totSink_->set((t0+t1)/2., -1*sinksum );
	};

	sink /= soluteMesh.getvol();

	//----------------mineralization--------------------------------
	if (mineral) {
		const double reduct = 1.; // this is a reduction coefficient for when net immobilization is limited by N - not used now
		tVector rndiss(mineral->getNumNodesWithMineralization());

		mineral->mineralisationYang1994(t1 - t0, reduct, water_.gethOld(), rndiss);
		// weights not used
		//todo check if this is true, negative values should be OK
		if (rndiss.max() > 0.) {
			msg::error("swms: yang`s model calculated net immobilization instead of mineralisation, but swms can not handle that.");
		}

		//mineralization added to the sink term.
		//only top numnl nodes have mineralization.
		for (unsigned int i(0); i < rndiss.size(); ++i) {
			sink[i] += rndiss[i] / 14.; // cSink += rndiss / 14.; // rndiss is in ug N/cm3 convert to uMol/cm3
		}

		//reporting of mineralizationrate
		double totminrate = (rndiss*soluteMesh.getvol()).sum() / 14.;
		if (totalMineralization_)
			totalMineralization_->set(t0, -1*totminrate);
		if (totminrate > 1e-5)
			msg::warning("Mineralization effective, but code wasn't thoroughly tested yet. Check balance etc.");
	}

}

void Solute::updateLists(const Time &t, const std::deque<SimulaBase *> & rootNodeQueue_) {
	unsigned int j=nutrientUptake_.size();
	for (auto & it : rootNodeQueue_) {
		SimulaBase* p = it->getChild(nutrient);
		SimulaBase* u = p->getChild("rootSegmentNutrientUptake");
		nutrientUptake_.push_back(u);

		u = p->existingChild("nutrientConcentrationAtTheRootSurface");
		SimulaTable<Time> *tbl;
		if (u) {
			tbl = dynamic_cast<SimulaTable<Time> *>(u);
			if (!tbl)
				msg::error("Solute::updateLists: concentrationAtRootSurface must be a time table");
			nutrientConcentration_.push_back(tbl);
		} else {
			nutrientConcentration_.push_back(nullptr);
			msg::warning("Solute::updateLists: nutrientConcentrationAtTheRootSurface not found");
		}

		//u=p->getChild("Cmin");

		if (it->getName() == "growthpoint") {
			Coordinate pos;
			it->getBase(t, pos);
			std::vector<double> w(3);
			w[0] = pos.x;
			w[1] = pos.y;
			w[2] = pos.z;
			femIndexes_.push_back(std::vector<int>(1, -2));
			femWeights_.push_back(w);
			//femSumWeights_.push_back(-1);
		} else {
			std::vector<int> indexes(27);
			std::vector<double> weights(27);
			double sum; // TODO unused?
			Coordinate pos;
			it->getAbsolute(t, pos);
			soluteMesh.matchNodes(pos, indexes, weights, sum);
			femIndexes_.push_back(indexes);
			femWeights_.push_back(weights);
			//femSumWeights_.push_back(sum);
		}

	}
	setrootsurfacevalues(t,j);
}

void Solute::setrootsurfacevalues(const Time & t1, const unsigned int jj) {
	//round off
	const double t=(double)ceil(t1/TIMEERROR)*TIMEERROR;//rounding of t, as very small rounding off differences can cause steep gradients in the interpolation tables, resulting in bad extrapolation results.

	//update list
	double concsur;
	for (unsigned int i(jj); i < femIndexes_.size(); ++i) {
		if (!nutrientUptake_[i]->evaluateTime(t))
			continue;
		std::vector<int> &ind = femIndexes_[i];
		if (ind[0] == -1) {
			//!ROOTS OUTSIDE GRID no nutrients
			concsur = 0.0;
		} else {
			std::vector<int> * pind;
			std::vector<int> indexes;
			std::vector<double> weights;
			std::vector<double> * pweights;
			double sum(0.0);
			//double * psum;
			if (ind[0] == -2) {

				//growthpoint, needs matching
				Coordinate pos;
				auto p=nutrientUptake_[i]->getParent(2);
				p->get(std::max(p->getStartTime(),t-MAXTIMESTEP) , pos);//TODO hack to make sure we do not enter predictor corrected loops here. I guess precision is not that great.
				std::vector<double> &base = femWeights_[i];
				pos.x += base[0];
				pos.y += base[1];
				pos.z += base[2];
				soluteMesh.matchNodes(pos, indexes, weights, sum);
				pind = &indexes;
				pweights = &weights;
				//psum=&sum;
			} else {
				pind = &ind;
				pweights = &femWeights_[i];
				//psum=&Simunek::femSumWeights_[i];
			}
			if (pind->size() > 1) {
				//determine weights based on distances
				concsur = 0;
				double tmpsc = 0;
				for (unsigned int j = 0; j < pind->size(); ++j) {
					int femi = pind->at(j);
					double w(pweights->at(j));
					//if(Conc[femi]>0){//ignore nodes with negative concentrations
					tmpsc += w;
					concsur += w * Conc[femi];
					//}
					//std::cout<<std::endl<<pweights->at(j)<<" * "<<Conc[femi]<<" x "<<x[femi]<<" y "<<y[femi]<<" z "<<z[femi];
				}
				//std::cout<<std::endl<<"sum:"<<concsur<<" / "<<*psum<<" / "<<tmps;
				if (tmpsc > 0)
					concsur /= tmpsc;
			} else {
				//push the first nodal values
				int femi = pind->at(0);
				if (femi < 0) {
					// growthpoint matching above returned -1 => point is outside of grid
					concsur = 0.0;
				} else {
					concsur = Conc[femi];
				}
			}
			//sometimes at high sinks (due too high rld) model migth have temporal small negative numbers for concentration
			concsur=std::max(concsur, 0.);

		}
//		std::cout.precision(5);
		if (nutrientConcentration_[i]){
			if(nutrientConcentration_[i]->size()==0 && t>nutrientConcentration_[i]->getStartTime()){//coming from nodes that are just added. Put data in based on their start time so they do not complain.
				double ts=nutrientConcentration_[i]->getStartTime();
				//todo the tables here get quite large, and are interpolated. So why not leave values out like the integration library does?
				nutrientConcentration_[i]->set(ts, concsur);
			}
			nutrientConcentration_[i]->set(t, concsur);
		}
	}

	/*for(int i(0); i<nutrientConcentration_.size(); ++i){
		if(nutrientConcentration_[i]->size()==0 && t>nutrientConcentration_[i]->getStartTime()){//coming from nodes that are just added. Put data in based on their start time so they do not complain.
			double ts=nutrientConcentration_[i]->getStartTime();
			nutrientConcentration_[i]->set(ts, concsur);
		}
		nutrientConcentration_[i]->set(t, concsur);
		if(nutrientConcentration_[i]->size()<2){
			if(t!=nutrientConcentration_[i]->getStartTime()){
				msg::warning("delete me 1");
			}else if((nutrientConcentration_[i]->size()<1)){
				msg::warning("delete me 2");
			}else{
				msg::warning("delete me 3");
			}
		}
		std::cout<<std::endl<<(void*)nutrientConcentration_[i]<<" size "<<nutrientConcentration_[i]->size();
	}*/

}

SoluteMassBalanceTest::SoluteMassBalanceTest(SimulaDynamic* pSD) :
		DerivativeBase(pSD), seedReserves(0.) {
	std::string nutrient(pSD->getParent()->getName());
	SimulaBase *p = ORIGIN->getChild("soil")->getChild(nutrient);
	SimulaBase *d;

	//todo come up with better names for these
	totSoluteChange = p->getChild("totalSoluteChange", "umol");
	sumTopBoundaryFlux_ = p->getChild("topBoundaryFlux", "umol");
	sumBottomBoundaryFlux_ = p->getChild("bottomBoundaryFlux", "umol");
	totalMineralization_ = p->existingChild("totalMineralization", "umol");
	totSink_ = p->getChild("totalSink", "umol");
	totalNutrientUptake_ = ORIGIN->existingPath("/plants/" + pSD->getParent()->getName() + "/plantNutrientUptake", "umol");
	//todo for backwards compatibility, simply insert this, instead of not calculating the balance
	if (totalNutrientUptake_)
		totalNutrientUptake_->get(0., seedReserves);					//todo these are a problem, when plants are not planted at time 0

	d = p->existingChild("massBalanceError");
	if (d)
		massBalanceError_ = dynamic_cast<SimulaTable<Time>*>(d);

	 p->getChild("totalSoluteInColumn", "umol")->get(0.,ref);
}

void SoluteMassBalanceTest::calculate(const Time & t, double &error) {
	double change(0);
	totSoluteChange->get(t, change);

	double tboundary(0);
	sumTopBoundaryFlux_->get(t, tboundary);

	double bboundary(0);
	sumBottomBoundaryFlux_->get(t, bboundary);

	double mineralization(0);
	if (totalMineralization_)
		totalMineralization_->get(t, mineralization);

	double sink(0);
	totSink_->get(t, sink);

	double uptake(0);
	if (totalNutrientUptake_) {
		totalNutrientUptake_->get(t, uptake);
//note that sink is negative when roots take up but uptake is positive
		if (std::fabs(sink) > 10 && std::fabs((sink + uptake - seedReserves)/sink) > 0.05){
			//std::cout<<std::endl<< uptake << " " << sink <<" "<<seedReserves;
			msg::warning(
					"SoluteMassBalanceTest: uptake may not equal sink for " + pSD->getParent()->getName()
							+ ". Message could be caused by seed reserves added to simulation after t=0, or roots outside the grid etc, but please check");
		}
		//std::cout<<std::endl<<"at time "<<t<<" sink is "<<sink <<" and uptake is "<<uptake - seedReserves;
	} else {
		msg::warning("SoluteMassBalanceTest: not including nutrient uptake as the table is missing");
	}

	//todo sink contains mineralization
	double sumFluxes(tboundary + bboundary + mineralization + sink);
	error = (change - sumFluxes) / ref;
	if (std::fabs(error) > 0.01)
		msg::warning("SoluteMassBalanceTest: mass balance is of by more than 1% of initial "+std::to_string(ref)+" umol in column.");

	if (massBalanceError_)
		massBalanceError_->set(t, change - sumFluxes);
}

std::string SoluteMassBalanceTest::getName() const {
	return "soluteMassBalanceTest";
}

DerivativeBase * newInstantiationSoluteMassBalanceTest(SimulaDynamic* const pSD) {
	return new SoluteMassBalanceTest(pSD);
}

//Register the module
class AutoRegisterSoluteFunctions {
public:
	AutoRegisterSoluteFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["soluteMassBalanceTest"] = newInstantiationSoluteMassBalanceTest;
	}
};

// our one instance of the proxy
static AutoRegisterSoluteFunctions l8r9h38hr9h9h9;

