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
#if _WIN32 || _WIN64
#define _USE_MATH_DEFINES
#endif

#include "Watflow.hpp"

#include "../../engine/Origin.hpp"

//#include <fstream> // debug matrix - file output
#include "../../math/pcgSolve.hpp"
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

//*************************************************
Water::Water(SimulaDynamic* pSD){

}

void Water::calculate(const Time &t, double &theta){

}
std::string Water::getName() const {
	return "Water";
}

//DerivativeBase * newInstantiationWater(
//		SimulaDynamic* const pSD) {
//	return new Water(pSD);
//}
//*************************************************


Watflow::Watflow(SimulaDynamic* const pSD):
waterMesh(), /* empty mesh obj*/
AtmInF(true),
explic(false),
FreeD(true),
NumNP(waterMesh.getNumNP()), // copies
NumEl(waterMesh.getNumEl()),
NumBP(waterMesh.getNumBP()),
Q(0.,NumNP),   	 /* based on hOld */
SSM(nullptr),
par(VanGenuchten(waterMesh)),
thetaDry_(nullptr), thetaFieldCapacity_(nullptr), soilScalingExponent_(nullptr),
thetaR_(nullptr), thetaSat_(nullptr), depthOfTheta_(nullptr),
prec_(nullptr), interception_(nullptr), evaporation_(nullptr),
potentialEvaporation_(nullptr), evaporationEn_(nullptr),
x(waterMesh.getCordX()),
y(waterMesh.getCordY()),
z(waterMesh.getCordZ()),
hNew(0.,NumNP),
hOld(0.,NumNP),
hTemp(0.,NumNP),
matchmode(1),
B(0.,NumNP),
RHS(0.,NumNP),
DS(0.,NumNP),
Sink(NumNP),
Con(NumNP),
ConO(NumNP), /* Nodal values of the hydraulic conductivity at the old time level  */
Cap(NumNP),
thOld(NumNP),
thNew(NumNP),
Adiag_boundary_nodes_(0., NumNP),
TolTh(0.0001), // convergence criteria
TolH(0.1), // convergence criteria
rLen(waterMesh.getrLen()),
rTop(0.),
prec(0.),
rSoil(0.),
hCritS(0.),    /// (max. allowed pressure head at the soil surface)
/// hcrtiA = Minimum allowed pressure head at the soil surface [L] (Table 8.11).
hCritA(-1000000.),
// hCritA=-abs(hCritA)
// the water pressure cannot fall below a limit value (defined by the temperature and the relative humidity of the air)
// If this value is reached, the boundary condition is switched to a Dirichlet type with h = hCritA = rel. humidity of air
// and is maintained until the potential evaporation rate decreases or rainfall occurs. (see shift subroutine in watflow3.f)
VolumeOfDomain(waterMesh.getVolumeOfDomain()), Nsub(waterMesh.getNsubel()),
totWaterInColumn(nullptr),totWaterChange(nullptr), sumTopBoundaryFlux_(nullptr), sumBottomBoundaryFlux_(nullptr), totSink_(nullptr)
{

	// set soil parameter
	thetaR_ 	= pSD->getPath("/environment/soil/water/residualWaterContent", "100%");
	thetaSat_ 	= pSD->getPath("/environment/soil/water/saturatedWaterContent", "100%");
	thetaDry_ 	= pSD->existingPath("/environment/soil/water/waterContentAirDrySoil");
	thetaFieldCapacity_  = pSD->existingPath("/environment/soil/water/waterContentAtFieldCapacity");
	soilScalingExponent_ = pSD->existingPath("/environment/soil/water/soilScalingExponent");
	depthOfTheta_ 		 = pSD->existingPath("/environment/soil/water/depthOfTheta");
	// set atm parameter
	prec_ 		  	= pSD->getPath("/environment/atmosphere/precipitation", "cm/day");
	interception_ 	= pSD->existingPath("/atmosphere/interception", "cm/day");
	potentialEvaporation_ = pSD->existingPath("/atmosphere/potentialEvaporation", "cm/day");
	evaporation_ 	= pSD->existingPath("/atmosphere/evaporation", "cm/day");
	evaporationEn_ 	= pSD->existingPath("/environment/atmosphere/evaporation", "cm/day");
	if (potentialEvaporation_) {
		msg::warning("Watflow:: using potentialEvaporation from atmosphere section",1);
	} else if (evaporation_) {
		msg::warning("Watflow:: using evaporation from atmosphere section",1);
	} else if (evaporationEn_) {
		msg::warning("Watflow:: using evaporation from environment section, i.e. given, predefined.",1);
	} else {
		msg::error("Watflow:: No evaporation defined in input file.");
	}

	// initialize matrix
	SSM = new SparseSymmetricMatrix(NumNP);
	waterMesh.easy_init_matrix(*SSM);

	//bottom boundary condition is by default free drainage, but here we switch based on user input
	depthWaterTable_ = ORIGIN->existingPath("environment/soil/water/depthOfWaterTable");
	if(depthWaterTable_) {
		msg::warning("Watflow: Using Dirichlet bottom boundary based on specified depthOfWaterTable.",3);
		FreeD=false; ///todo enumerator would be better, so we can simply switch between various boundaries
	}else{
		msg::warning("Watflow: Using Free drainage bottom boundary.",3);
	}



	// set material!!!! set con and thold and thnew
	SimulaBase* p = ORIGIN->getPath("environment/soil/water/initialHydraulicHead");
	double Y;
	for (tIndex i = 0; i != NumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		hOld[i]  = Y;
		hNew[i]  = Y; // hNew = hOld;
		hTemp[i] = Y; // TODO locale variable later?
	}

	par.calc_fq(hOld, thOld);
	par.calc_fk(hOld, thOld, ConO);
	par.calc_fc(hOld, Cap);
	thNew = thOld;
	Con = ConO;

	// find out what method to use for matching roots to fem nodes
	p = ORIGIN->existingPath("environment/dimensions/rootMatchingMode");
	if (p) {
		std::string mode;
		p->get(mode);
		if (mode == "postma and lynch, 2011") {
			matchmode = 1;
		} else if (mode == "ignore root placement") {
			matchmode = 2;
			msg::warning("Watflow::Watflow: using rootMatchingMode: 'ignore root placement' as set in environment/dimensions/rootMatchingMode",1);
		} else {
			msg::error("Watflow::Watflow: unknown string for environment/dimensions/rootMatchingMode ");
		}
	}

	// Initialize atmosphere
	set_atmosphere(0); // thNew is thOld, so the first atmosphere coupling is good.
	neumann_boundary_condition(); //computes initial Q simply for the first output

	//output
	p = ORIGIN->getChild("soil")->existingChild("water");
	if(p){
		SimulaBase* d = p->existingChild("totalWaterInColumn", "cm3");
		if (d) totWaterInColumn = dynamic_cast<SimulaTable<Time>*>(d);
		d = p->existingChild("totalWaterChangeInColumn", "cm3");
		if (d) totWaterChange = dynamic_cast<SimulaTable<Time>*>(d);
		d = p->existingChild("topBoundaryFluxRate", "cm3/day");
		if (d) {
			sumTopBoundaryFlux_ = dynamic_cast<SimulaTable<Time>*>(d);
			sumTopBoundaryFlux_->setInterpolationMethod("step");
		}
		d = p->existingChild("bottomBoundaryFluxRate", "cm3/day");
		if (d) {
			sumBottomBoundaryFlux_ = dynamic_cast<SimulaTable<Time>*>(d);
			sumBottomBoundaryFlux_->setInterpolationMethod("step");
		}
		d = p->existingChild("totalSinkRate", "cm3/day");
		if (d) {
			totSink_ = dynamic_cast<SimulaTable<Time>*>(d);
			totSink_->setInterpolationMethod("step");
		}
	}

	reportTotWaterColumn(0,0);
	this->setrootsurfacevalues(0., 0.0001);
}


void Watflow::calculate(tIndex & ItCum, const tIndex & MaxIt, double & dt, const double & dtMin,  const double & dtOld,
		const double & tOld, const tIndex & TLevel, tIndex & Iter) {

	this->set_atmosphere(tOld+dt/2.); // midpoint for evaporation/precipitation

	//if (TLevel != 1) { // TLevel is the current time-step number
	//extrapolation as first guess seems to mostly cause convergence errors.
		//hTemp = hNew; //old hnew
		//new estimate for hnew
		/*for (tIndex i = 0; i < NumNP; ++i) {
			if (waterMesh.getKode()[i] < 1) {
				hNew[i] += (hNew[i] - hOld[i]) * dt / dtOld;
			}
		}*/
		//update hold to old hnew
		//hOld  = hTemp; //update hOld todo use swap here.
	    hOld = hNew;
		thOld = thNew;
    if(hOld.max()>0) {
    	//auto n=0;
    	/*for(auto i:hOld){
    		if(i>0.) std::cout<<std::endl<<n<<" is "<<i<<" at "<<z[n];
    		++n;
    	}*/
    	msg::error("Watflow:: h="+convertToString(hOld.max())+" at "+convertToString(tOld));
    }
    //msg::warning("Watflow:: h="+convertToString(hOld.max())+" at "+convertToString(tOld));
		//hTemp = hNew;
	//}
	ConO = Con; // gives ConO an initial value

	Iter   = 0; // debug iter has to be handed to swms_3d
	explic = false;

	//bool ItCrit;
	for (;;) {
		// --- Beginning of iteration loop --------------------------------------

		// setmatd calculates Con und Cap (conductivities) and thnew (theta at next time)
		// local

		// convert these functions to array

		if(explic){//todo when explic is hnew not hold ? is this not anyway the case, and I think we should get rid of explic completely.
			par.calc_fk(hOld, thOld, Con); //todo original htemp, but is the same?
			par.calc_fc(hOld, Cap);
			par.calc_fq(hOld, thNew);
		}else{
			par.calc_fk(hTemp*0.1+hNew*0.9, thNew, Con);//mix of previous iteration and current.
			par.calc_fc(hNew, Cap);
			par.calc_fq(hNew, thNew);
		}

		/// sets values in SSM and B as a function of hNew
		hTemp = hNew; // hTemp is updated so it is the last hNew (htemp is basicallz hnew previous)

		reset_system(tOld,dt, Iter);


		// debug lines
//			std::ofstream ssm;
//			ssm.open("ssm.txt");
//			SSM->print_sparse(ssm);
//			ssm.close();
//			exit(0);

		Pcg::solve(*SSM, RHS, hNew, 1.e-10, NumNP, SYM_GAUSS_SEIDEL);
		// Paramterlist: A, b, x. b is rhs, x is solution.

		++Iter;
		++ItCum;

		if (explic) {//explic only does one forward step and is used in implicit method fails to converge.
			break; // exit iterative loop if non-iterative procedure
		}
		// the next part is for implicit (and the the first) iteration

		//  Test for convergence
//		ItCrit = true;
		double EpsTh(0), EpsH(0);
		for (tIndex i = 0; i < NumNP; ++i) {
			if (hTemp[i] < par.gethSat()[i] && hNew[i] < par.gethSat()[i]) {
				//double Th = thNew[i] + Cap[i] * (hNew[i] - hTemp[i]) / (thSat[i] - thR[i]);
				//	EpsTh = std::fabs(thNew[i] - Th);
				EpsTh = std::max(EpsTh,std::fabs(Cap[i] * (hNew[i] - hTemp[i]) / (par.getthSat()[i] - par.getthR()[i])));
			} else {
				EpsH = std::max(EpsH,std::fabs(hNew[i] - hTemp[i]));
			}
		}
//		std::cout << std::endl << Iter << " eps " << EpsTh*10000 << " EpsH " << EpsH*10000;
		if (EpsTh > TolTh || EpsH > TolH) {
//			ItCrit = false;
			if (Iter < MaxIt) {
				// do the next iteration
					// continue;
			} else {
				if (dt < 2 * dtMin) {
					explic = true;
					msg::warning("Watflow::calculate: timestep is less than dtMin, switching to explicit solving");
					hTemp = hOld;
					hNew  = hOld;
					// do the next iteration
						// continue;
				} else {
					// restart solving with a smaller timestep?
					// note that we also restart with a different hNew, which is not extrapolated from the previous timestep.
					hNew  = hOld;
					hTemp = hOld; // or could also be the extrapolated htemp
					dt = std::max(dt / 3., dtMin);
					msg::warning("Watflow::calculate: Did not converge fast enough, reducing timestep by a factor 3 and trying again.");
					// reset
					Iter = 0;
					explic = false;
					this->set_atmosphere(tOld+dt/2.); // midpoint for evaporation/precipitation
					// do the next iteration
					// continue;
				}
			}
		}else{ // ItCrit is true, so we can leave the for loop
			if (Iter > MaxIt/2 ) {
				msg::warning("Watflow::calculate: needing more than "+std::to_string(MaxIt/2)+" iterations to solve problem",1);
			}
			break;
		}

	} // --- End of iteration loop --------------------------------------------


	// OUTPUTS
	//   P-Level information
	computeQBackwards(dt); //to report values for Q.
	reportTotWaterColumn(tOld,dt);
	// set current Conc & hhead at root surface
	double t = tOld + dt;
	this->setrootsurfacevalues(t, dtMin);

} // end method


void Watflow::reset_system(const double & tOld, const double & dt, const tIndex & Iter) {

	// (Re-)Initialization
	if (0 == Iter) {
		DS 	   = 0.;
	}
	B = 0.;
	SSM->resetToZero(); // to reset the matrix here it is essential to have a matrix with neighboring entries first!


	double VE;
	double AMul;

	tIndex i(0), j(0), k(0), l(0);
	tIndex iG;
	tIndex jG;
	double temp_value;
	double sumConE;

	tIndex count(0), selcount(0);
	for (auto & subit : waterMesh.getSubelementlist() ) {
			i = subit[0];
			j = subit[1];
			k = subit[2];
			l = subit[3];

			// sumConE is conductivity sum per sub-element
			sumConE = ( Con[i]  + Con[j]  + Con[k]  + Con[l] )/ 24.; //(1./4. * 1./6. = 1./24.)

			B[i] += waterMesh.getDi()[count] * sumConE; // see eq 4.5; Di from the determinant the linear distances in certain direction
			B[j] += waterMesh.getDi()[count + 1] * sumConE;
			B[k] += waterMesh.getDi()[count + 2] * sumConE;
			B[l] += waterMesh.getDi()[count + 3] * sumConE;

			AMul = sumConE / 6. / waterMesh.getSubelemetVolumes()[selcount];
			++selcount;  // go through each subelement
			// for the 36 and 6 see equation 4.5 and 4.6 on page 18 of the manual,
			// they come from from the 6 directions x,y,z in positive and negative and all combinations of directions

			for (tIndex ii = 0; ii < 4; ++ii) { // all neighboring combinations go into the matrix
				iG = subit[ii];
				for (tIndex jj = ii; jj < 4; ++jj) {
					jG = subit[jj];

					temp_value = AMul * (waterMesh.getBi()[count + ii] * waterMesh.getBi()[count + jj] + waterMesh.getCi()[count + ii] * waterMesh.getCi()[count + jj] + waterMesh.getDi()[count + ii] * waterMesh.getDi()[count + jj]);
//					if(std::isnan(temp_value) || isinf(temp_value)){msg::error("bad numbers");}
					SSM->addValueUnsafely(iG, jG, temp_value);
				}
			}
			count += 4; // go through each node of each subelement
	}

	if (0 == Iter) {
		tIndex it_subelem = 0;
		setWaterSink(tOld, tOld+dt);
		for (auto & it : waterMesh.getSubelementlist() ) {
			i = it[0];
			j = it[1];
			k = it[2];
			l = it[3];
			VE = waterMesh.getSubelemetVolumes()[it_subelem];
			++it_subelem;
			// if(VE <= 0) msg::error("negative volume");

			//  here we go back to nodal values
			DS[i] += VE * Sink[i] * 0.25;
			DS[j] += VE * Sink[j] * 0.25;
			DS[k] += VE * Sink[k] * 0.25;
			DS[l] += VE * Sink[l] * 0.25;
		}
	}



/// Determine boundary fluxes
// this does normally nothing, but if the given boundary condition cause the soil to be too wet or too dry,
// then shift will change the boundary condition

	neumann_boundary_condition(); //computes Q

	auto &F(waterMesh.getvol());

	RHS = - B + F*( Cap*hNew - thNew + thOld )/dt + Q - DS;

	dirichlet_boundary_condition(tOld+dt/2.);


	for (tIndex i = 0; i < NumNP; ++i) {
		temp_value = waterMesh.getvol()[i] * Cap[i] / (dt);
		SSM->addValueUnsafely(i, i, temp_value);
	}
	return;
}

///computes Q
void Watflow::neumann_boundary_condition() {
	Q=0.;
	//set Q for the top boundary nodes.
	for (auto n : waterMesh.getTopBoundaryNodes()) {
		// set scaling
		double scaling(0.);
		getscaling(thNew[n], scaling);
		if (scaling > 1.) {
			scaling = 1.;
			msg::warning("Watflow::set_atmosphere: Scaling of evaporation was greater than one, check code. Simulation run scaled with 1.0.");
		}

		// post scaling procedure
		if (prec > rSoil * scaling) {
			if (prec > rSoil) {
				scaling = 1.;
			} else {
				scaling = prec / rSoil;
					}
					}

		Q[n] = - waterMesh.getWidth()[n] * (rSoil * scaling - prec); // L²*L = L³, cm³

				}

	if (FreeD) {
		//neumann at bottom boundary
		//todo for this to work, getWidth need to have indexes numnp, and not numbp
		for (auto i : waterMesh.getBottomBoundaryNodes()) {
			Q[i] = -waterMesh.getWidth()[i] * ConO[i]; // TODO discuss why this flux is calculated with ConO (old Con)
			if(std::isnan(Q[i])) {
				/*std::valarray<double> tcon(0., NumNP);
				par.calc_fk(hOld, tcon);
				std::cout<<std::endl<<waterMesh.getWidth()[i]<<" "<<ConO[i]<<" "<<hOld[i]<<" "<<tcon[i]<<std::endl;*/
				msg::error("WatFlow: Q[i] is NAN");
			}
		}
	}
	//msg::warning("Watflow:: Q is max "+convertToString(Q.max())+" and min "+convertToString(Q.min()));
	return;
}

void Watflow::computeQBackwards(double dt){
	//This code computes what Q should have been when using neumann to get the same hnew result as we just computed with dirichlet.

	//the values on diagonal of A were overwritten when doing dirichlet, we restore them first
	for (auto n : waterMesh.getTopBoundaryNodes()) {
		if(fabs(RHS[n]/hNew[n]) > 10.e20) SSM->insert(n,n,Adiag_boundary_nodes_[n]);
			}
	for (auto n : waterMesh.getBottomBoundaryNodes()) {
		if(fabs(RHS[n]/hNew[n]) > 10.e20) SSM->insert(n,n,Adiag_boundary_nodes_[n]);
		}
	//auto Qcheck=Q;


	//compute back the right hand side
	tVector temp_A_times_hNew(NumNP);
	SSM->vectorMultiply(hNew, temp_A_times_hNew);

	//compute Q from the right hand side (formula 4.4)
	auto &F(waterMesh.getvol());
	//for (tIndex n = 0; n < NumNP; ++n) {
	for (auto n : waterMesh.getTopBoundaryNodes()) {
		//if (waterMesh.getKode()[n] < 1) {
		//	continue;
		//} else {
		if(fabs(RHS[n]/hNew[n]) > 10.e20){
			//Dirichlet boundary condition.
			//	RHS = - B + F*( Cap*hNew - thNew + thOld )/dt + Q - DS;

			Q[n] = temp_A_times_hNew[n] + B[n] + DS[n] - F[n] * (thNew[n] - thOld[n]) / (dt); //eq 4.4 of the manual
			if(std::isnan(Q[n])) msg::error("WatFlow: Q[n] is NAN");
	}
//			}
	}
	for (auto n : waterMesh.getBottomBoundaryNodes()) {
		//if (waterMesh.getKode()[n] < 1) {
		//	continue;
		//} else {
		if(fabs(RHS[n]/hNew[n]) > 10.e20){
			//Dirichlet boundary condition.
			Q[n] = temp_A_times_hNew[n] + B[n] + DS[n] - F[n] * (thNew[n] - thOld[n]) / (dt); //eq 4.4 of the manual
			if(std::isnan(Q[n])) msg::error("WatFlow: Q[n] is NAN (2)");

		}
//		}
	}

	//Qcheck-=Q;
	//std::cout<<std::endl<<Q.max()<<" "<<Q.min()<<" "<<Qcheck.max()<<" "<<Qcheck.min()<<" ";

}

void Watflow::dirichlet_boundary_condition(const Time &t) {
	//The top boundary is assumed to have a predescribed flux coming
	//from precipitation and or evaporation. This flux is already set in Q by
	//setAtmosphere.
	//here we just do not except that flux if it leads to too dry soil or too wet soil, in which case we fix hnew
	for (auto n : waterMesh.getTopBoundaryNodes()) {
		if (hNew[n] <= hCritA && rTop>0.) { // Evaporation capacity is exceeded
			msg::warning("Watflow: top soil too dry, in order to have the evaporation. ");
			hNew[n] = hCritA;
			Adiag_boundary_nodes_[n]=(*SSM)(n,n);
			SSM->insert(n, n, 10.e30); // fill diagonal
			RHS[n] = 10.e30 * hNew[n];  // rhs
			Q[n] = 0.;  //Computed later
			//continue;
		} else if (hNew[n] >= hCritS && rTop<0.) { // Infiltration capacity is exceeded
			msg::warning("Watflow: topsoil too wet, and can not take up precipitation, assuming runoff.");
			hNew[n] = hCritS;
			Adiag_boundary_nodes_[n]=(*SSM)(n,n);
			SSM->insert(n, n, 10.e30); // fill diagonal
			RHS[n] = 10.e30 * hNew[n];  // rhs
			Q[n] = 0.; //Computed later
		}
	}

	/// Free Drainage
	if(depthWaterTable_){//(assumes FreeD is false)
		//dirichlet at the bottom boundary.
		//todo implement code to get values for h (in case of changing groundwatertable over time
		double hbot=-300;
		depthWaterTable_->get(t,hbot);
		//msg::warning("Watflow::dirichlet: Dirichlet boundary set active");
		for (auto i : waterMesh.getBottomBoundaryNodes()) {
			hNew[i]=hbot-z[i];
			Adiag_boundary_nodes_[i]=(*SSM)(i,i);
			SSM->insert(i, i, 10.e30); // fill diagonal
			RHS[i] = 10.e30 * hNew[i];  // rhs
		}
	}

	return;
}

// **********************************************************************************
// scaling function for getting the actual evaporation from the potential evaporation
void Watflow::getscaling(const double& theta, double& scaling) {
	// z is the depth of the theta at evaporation front
	double z;
	if (depthOfTheta_) {
		depthOfTheta_->get(z);
	} else {
		z = 0.0; // top soil, default
	}
	// get theta dry air from xml
	double thSat, thR, thDry, thFC;
	if (thetaDry_) {
		thetaDry_->get(z, thDry);
	} else {
		thetaSat_->get(z, thSat);
		thetaR_->get(z, thR);
		thDry = thR + 0.1 * (thSat - thR);
	}
	// get theta at field capacity from xml
	if (thetaFieldCapacity_) {
		thetaFieldCapacity_->get(z, thFC);
	} else {
		thetaSat_->get(z, thSat);
		thFC = 0.9 * thSat;
	}
	// get exponent, depending on soil type, if not set, default is 2
	double exponent;
	if (soilScalingExponent_) {
		soilScalingExponent_->get(exponent);
	} else {
		exponent = 2.0;
	}
	double thRel = (theta - thDry) / (thFC - thDry);
	if (theta <= thDry) {
		scaling = 0.0;
	} else if (theta <= thFC) {
		scaling = pow((0.5 - 0.5 * cos(M_PI * thRel)), exponent);
	} else {
		scaling = 1.0;
	}

}

void Watflow::set_atmosphere(const Time & t) {
	// local
	double  interception(0.);

	// precipitation
	prec_->get(t, prec);
	prec = std::fabs(prec);

	// interception
	if (prec > 1.e-6 && interception_) {
			interception_->get(t, interception);
			prec = prec - std::fabs(interception);
			msg::warning("Watflow::set_atmosphere: using interception from atmosphere section",1);
		}

	// evaporation
	if (potentialEvaporation_) {
		potentialEvaporation_->get(t, rSoil);
	} else if (evaporation_) {
		evaporation_->get(t, rSoil);
	} else {
		evaporationEn_->get(t, rSoil);
	}
	rSoil = std::fabs(rSoil);

	rTop = rSoil - prec; // rTop:  potential fluid flux across the soil surface

	return;
} // end set_atmosphere method

void Watflow::setWaterSink(const double & t0, const double & t1) {
	if (t1 <= t0)
		return;
	Sink = 0.;
	for (unsigned int i(0); i < femIndexes_.size(); ++i) {
		if (!waterUptake_[i]->evaluateTime(t0))
			continue;
		std::vector<int> &ind = femIndexes_[i];
		if (ind[0] == -1) {
			//!ROOTS OUTSIDE GRID no nutrients and no water
			continue;
		} else {
			double wuptake, wu1;
			waterUptake_[i]->get(t1, wuptake);
			waterUptake_[i]->get(t0, wu1);
			wuptake -= wu1;
			wuptake /= (t1 - t0);
			if (wuptake < 1e-15)
				continue;

			std::vector<int> * pind;
			std::vector<int> indexes;
			std::vector<double> weights;
			std::vector<double> * pweights;
			double sum;
			//double * psum;
			if (ind[0] == -2) {
				//growthpoint, needs matching
				Coordinate pos;
				waterUptake_[i]->getParent(2)->get(t0, pos);
				std::vector<double> &base = femWeights_[i];
				pos.x += base[0];
				pos.y += base[1];
				pos.z += base[2];
				waterMesh.matchNodes(pos, indexes, weights, sum); //todo: somewhat ugly, depends on order of things in several other places
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
					sumweights += (*pweights)[j];
				}
				for (unsigned int j = 0; j < pind->size(); ++j) {
					int femi = pind->at(j);
					Sink[femi] += wuptake * (*pweights)[j] / sumweights;
				}
			} else {
				//push the first nodal values
				int femi = pind->at(0);
				Sink[femi] += wuptake;
			}

		}

	}
	Sink /= waterMesh.getvol();
}

void Watflow::updateLists(const Time &t, const std::deque<SimulaBase *> & rootNodeQueue_) {
	unsigned int j=waterUptake_.size();
	for (auto & it : rootNodeQueue_) {
		SimulaBase* u = it->getChild("rootSegmentWaterUptake");
		waterUptake_.push_back(u);

		u = it->existingChild("hydraulicHeadAtRootSurface");
		SimulaTable<Time> *tbl;
		if (u) {
			tbl = dynamic_cast<SimulaTable<Time> *>(u);
			if (!tbl)
				msg::error("Watflow::updateLists: hydraulicheadAtRootSurface must be a time table");
			hydraulicHead_.push_back(tbl);
		} else {
			hydraulicHead_.push_back(nullptr);
			msg::warning("Watflow::updateLists: hydraulicheadAtRootSurface not found",2);
		}

		u = it->existingChild("volumetricWaterContentAtTheRootSurface");
		if (u) {
			tbl = dynamic_cast<SimulaTable<Time> *>(u);
			if (!tbl)
				msg::error("Watflow::updateLists: volumetricWaterContentAtRootSurface must be a time table");
			volumetricWaterContent_.push_back(tbl);
		} else {
			volumetricWaterContent_.push_back(nullptr);
			msg::warning("Watflow::updateLists: volumetricWaterContentAtRootSurface not found",2);
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
			double sum;
			Coordinate pos;
			it->getAbsolute(t, pos);

			waterMesh.matchNodes(pos, indexes, weights, sum);
			femIndexes_.push_back(indexes);
			femWeights_.push_back(weights);
			//femSumWeights_.push_back(sum);
		}

	}
	setrootsurfacevalues(t, TIMEERROR, j);//set root values for added root segments.

}

void Watflow::setrootsurfacevalues(const Time & t1, double dtMin, unsigned int j) {
	const double t=(double)ceil(t1/TIMEERROR)*TIMEERROR;//rounding of t, as very small rounding off differences can cause steep gradients in the interpolation tables, resulting in bad extrapolation results.
	// make sure lists are up to date
	double hhsur, thsur;
	for (unsigned int i(j); i < femIndexes_.size(); ++i) {
		if (!waterUptake_[i]->evaluateTime(t))
			continue;
		std::vector<int> &ind = femIndexes_[i];
		if (ind[0] == -1) {
			//!ROOTS OUTSIDE GRID no water
			hhsur = -1e8;
			thsur = 0.0;
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
				auto p=waterUptake_[i]->getParent();
				p->get(std::max(p->getStartTime(),t-MAXTIMESTEP) , pos);//TODO hack to make sure we do not enter predictor corrected loops here. I guess precision is not that great.
				std::vector<double> &base = femWeights_[i];
				pos.x += base[0];
				pos.y += base[1];
				pos.z += base[2];
				waterMesh.matchNodes(pos, indexes, weights, sum);
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
				hhsur = 0;
				thsur = 0;
				//std::cout<<std::endl<<"weigts";
				double tmps = 0;
				for (unsigned int j = 0; j < pind->size(); ++j) {
					int femi = pind->at(j);
					double w(pweights->at(j));
					tmps  += w;
					hhsur += w * hNew[femi];
					thsur += w * thNew[femi];
				}
				hhsur /= tmps;
				thsur /= tmps;
			} else {
				//push the first nodal values
				int femi = pind->at(0);
				if (femi < 0) {
					// growthpoint matching above returned -1 => point is outside of grid
					hhsur = -1e8;
					thsur = 0.0;
				} else {
					hhsur = hNew[femi];
					thsur = thNew[femi];
				}
			}

		}
//			std::cout.precision(5);
		if (hydraulicHead_[i]){
			if(hydraulicHead_[i]->size()==0 && t>hydraulicHead_[i]->getStartTime()){//coming from nodes that are just added. Put data in based on their start time so they do not complain.
				double ts=hydraulicHead_[i]->getStartTime();
				hydraulicHead_[i]->set(ts, hhsur);
			}
			hydraulicHead_[i]->set(t, hhsur);
		}
		if (volumetricWaterContent_[i]){
			if(volumetricWaterContent_[i]->size()==0 && t>volumetricWaterContent_[i]->getStartTime()){//coming from nodes that are just added. Put data in based on their start time so they do not complain.
				double ts=volumetricWaterContent_[i]->getStartTime();
				volumetricWaterContent_[i]->set(ts, thsur);
			}
			volumetricWaterContent_[i]->set(t, thsur);
		}


	}
}


void Watflow::reportTotWaterColumn(const Time&t,const double & dt)const{

	const double tnew=t+dt;

	double thTot=waterMesh.integrateOverMesh(thNew);
	if (totWaterInColumn)
		totWaterInColumn->set(tnew, thTot);

	static double initThTot(0.);
	if(dt==0.)initThTot=thTot;
	if (totWaterInColumn)
		totWaterChange->set(tnew, thTot-initThTot);


	//double thChange(0);
	//if(dt>0) thChange = (thTot-waterMesh.integrateOverMesh(thOld))/dt;

	//double hMean = waterMesh.integrateOverMesh(hNew)/VolumeOfDomain;

	double topBoundaryFlux(0);
	for (auto i : waterMesh.getTopBoundaryNodes()) {
		topBoundaryFlux += Q[i];
	}
	if(std::isnan(topBoundaryFlux)) msg::error("WatFlow: Top boundary flux is NAN");
	if(sumTopBoundaryFlux_)sumTopBoundaryFlux_->set(t,topBoundaryFlux);

	if(dt==0. && msg::getVerboseLevel()>1){
		double surf(0.);
		for (auto i : waterMesh.getTopBoundaryNodes()) {
			surf += waterMesh.getWidth()[i];
		}
		msg::warning("Watflow::Surface area water mesh is "+convertToString(surf));
	};

	double cumBottomBoundaryFlux(0);
	for (auto i : waterMesh.getBottomBoundaryNodes()) {
		cumBottomBoundaryFlux += Q[i];
	}
	if(std::isnan(cumBottomBoundaryFlux)) msg::error("WatFlow: Bottom boundary flux is NAN");
	if(sumBottomBoundaryFlux_)sumBottomBoundaryFlux_->set(t,cumBottomBoundaryFlux);

	double vMeanR = waterMesh.integrateOverMesh(Sink);//root uptake in per day
	if(totSink_) totSink_->set(t,-vMeanR);



}


WaterMassBalanceTest::WaterMassBalanceTest(SimulaDynamic* pSD) :
		DerivativeBase(pSD), relMassBalanceError_(nullptr) {

	//todo come up with better names for these
	totWaterChange = pSD->getSibling("totalWaterChangeInColumn", "cm3");
	sumTopBoundaryFlux_ = pSD->getSibling("topBoundaryFlux", "cm3");
	sumBottomBoundaryFlux_ = pSD->getSibling("bottomBoundaryFlux", "cm3");
	totSink_ = pSD->getSibling("totalSink", "cm3");
	totalWaterUptake_ = ORIGIN->existingPath("/plants/rootWaterUptake", "cm3");
	if (!totalWaterUptake_) {
		msg::warning("WaterMassBalanceTest: not including water uptake as the table \"/plants/rootWaterUptake\" is missing");
	}

	SimulaBase *d = pSD->existingSibling("relativeMassBalanceError");
	if (d)
		relMassBalanceError_ = dynamic_cast<SimulaTable<Time>*>(d);

	pSD->getSibling("totalWaterInColumn", "cm3")->get(0,ref);
}

void WaterMassBalanceTest::calculate(const Time & t, double &error) {
	double change(0);
	totWaterChange->get(t, change);

	double tboundary(0);
	sumTopBoundaryFlux_->get(t, tboundary);

	double bboundary(0);
	sumBottomBoundaryFlux_->get(t, bboundary);

	double sink(0);
	totSink_->get(t, sink);

	double uptake(0);
	if (totalWaterUptake_) {
		totalWaterUptake_->get(t, uptake);
	}

	//todo sink contains mineralization
	double sumFluxes(tboundary + bboundary + sink);
	error = (change - sumFluxes) / ref;
	if (std::fabs(error) > 0.01)
		msg::warning("WaterMassBalanceTest: mass balance is off by more than 1% of "+std::to_string(ref)+" ml initial water in column");

	if(relMassBalanceError_) relMassBalanceError_->set(t, error);

	//abs error returned
	error = change - sumFluxes;
}

std::string WaterMassBalanceTest::getName() const {
	return "waterMassBalanceTest";
}

DerivativeBase * newInstantiationWaterMassBalanceTest(SimulaDynamic* const pSD) {
	return new WaterMassBalanceTest(pSD);
}

//Register the module
class AutoRegisterWaterFunctions {
public:
	AutoRegisterWaterFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["waterMassBalanceTest"] = newInstantiationWaterMassBalanceTest;
	};
};

// our one instance of the proxy
static AutoRegisterWaterFunctions l8r9h38hr9h9h9;



