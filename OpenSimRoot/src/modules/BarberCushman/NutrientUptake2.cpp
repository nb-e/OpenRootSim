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
#include "NutrientUptake.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include <math.h>		// std::pow
#if _WIN32 || _WIN64
#include <algorithm>
#endif

/* Cash-Karp method
	this means two methods of orders 5 and 4

 Butcher Tableau:
 	0    |
	1/5  |	1/5
	3/10 |	3/40 	    9/40
	3/5  |  3/10 	   -9/10 	      6/5
    1    |-11/54 	    5/2 	    -70/27 	    35/27
    7/8  |1631/55296 	175/512 	575/13824 	44275/110592 	253/4096
 -------------------------------------
		 | 37/378 	    0 	250/621 	    125/594 	          0 	512/1771
		 | 2825/27648   0 	18575/48384 	13525/55296 	277/14336 	1/4
 */



Barber_cushman_1981_nutrient_uptake_explicit::Barber_cushman_1981_nutrient_uptake_explicit(SimulaDynamic* pSD) :TotalBaseLabeled(pSD){
}

void Barber_cushman_1981_nutrient_uptake_explicit::calculate(const Time &t, double &fluxDensity) {
}

//***************************************************************************/


std::string Barber_cushman_1981_nutrient_uptake_explicit::getName()const{
	return "barber_cushman_1981_nutrient_uptake_explicit";
}

DerivativeBase * newInstantiationBarber_cushman_1981_nutrient_uptake_explicit(SimulaDynamic* const pSD) {
	return new Barber_cushman_1981_nutrient_uptake_explicit(pSD);
}



//Register the module
static class AutoRegisterNutrientClassesInstantiationFunctionszj56 {
public:
	AutoRegisterNutrientClassesInstantiationFunctionszj56() {
		BaseClassesMap::getDerivativeBaseClasses()["barber_cushman_1981_nutrient_uptake_explicit"]	= newInstantiationBarber_cushman_1981_nutrient_uptake_explicit;
	}

}p7bg876h868h876h;


double BarberCushmanSolver::theta = -1;

const double BarberCushmanSolver::A11 = 0.2;    // 1/5;
const double BarberCushmanSolver::A21 = 0.075;  // 3/40;
const double BarberCushmanSolver::A22 = 0.225;  // 9/40;
const double BarberCushmanSolver::A31 = 0.3;    // 3/10;
const double BarberCushmanSolver::A32 = -0.9;   // -9/10;
const double BarberCushmanSolver::A33 = 1.2;    // 6/5 ;
const double BarberCushmanSolver::A41 = -11./54.;
const double BarberCushmanSolver::A42 = 2.5;    // 5/2;
const double BarberCushmanSolver::A43 = -70./27.;
const double BarberCushmanSolver::A44 = 35./27.;
const double BarberCushmanSolver::A51 = 1631./55296.;
const double BarberCushmanSolver::A52 = 175./512.;
const double BarberCushmanSolver::A53 = 575./13824.;
const double BarberCushmanSolver::A54 = 44275./110592.;
const double BarberCushmanSolver::A55 = 253./4096.;

/* order 5 */
const double BarberCushmanSolver::B11 = 37./378.;

const double BarberCushmanSolver::B13 = 250./621.;
const double BarberCushmanSolver::B14 = 125./594. ;

const double BarberCushmanSolver::B16 = 512./1771.;

/* order 4 */
const double BarberCushmanSolver::B21 = 2825./27648.;

const double BarberCushmanSolver::B23 = 18575./48384.;
const double BarberCushmanSolver::B24 = 13525./55296.;
const double BarberCushmanSolver::B25 = 277./14336.;
const double BarberCushmanSolver::B26 = 0.25;

const double BarberCushmanSolver::C1 = 0.2;
const double BarberCushmanSolver::C2 = 0.3;
const double BarberCushmanSolver::C3 = 0.6;
const double BarberCushmanSolver::C4 = 1;
const double BarberCushmanSolver::C5 = 0.875;

bool BarberCushmanSolver::MIXED = true;
bool BarberCushmanSolver::automatic_startstep = true;



BarberCushmanSolver::BarberCushmanSolver():
IntegrationBase(),dr(0),b(0),Ci(0),r0(0),r1(0),circ(0),rootHairDensityConversionFactor(0),
Cmin(0),res(0),
sKm(nullptr),
sImax(nullptr),
sDe(nullptr),
sv0(nullptr),
competition(nullptr),
slh(nullptr),
sNh(nullptr),
srh0(nullptr),
rootDiameter(nullptr),
spatialRootDensity(nullptr),
rootLength(nullptr),
depletionZone(nullptr),
depth(0,0,0),
msgcount(0),
firststep(true),
os(),
mode(0),
refTImax(0.),
refTKm(0.)
{
}

void BarberCushmanSolver::doDelayedConstruction()
{
	pSD->avoidPredictorCorrectedLoops();
	//set initial time
	const double lt=pSD->getStartTime();
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//root type
	std::string rootType;
	pSD->getParent(4)->getChild("rootType")->get(rootType);
	//get nutrient
	const auto & nutrient(pSD->getParent()->getName());
	//position
	pSD->getAbsolute(pSD->getStartTime(), depth);

	//plant parameters
	SimulaBase *param(GETROOTPARAMETERS(plantType,rootType));
	double dummy;
	//TODO assuming diameter to be constant
	rootDiameter=pSD->getParent(2)->getChild("rootDiameter", "cm");
	rootDiameter->get(lt,r0);r0/=2.;
	if(r0<=0)msg::error("BarberCushmanExplicit: root diameter is <= 0");

	if(pSD->getUnit()=="umol"){
		rootLength=pSD->getParent(2)->getChild("rootSegmentLength", "cm");
	}else{
		rootLength=nullptr;
		if(pSD->getUnit()!="umol/cm") msg::error("BarberCushmanExplicit: "+pSD->getPath()+" should have unit umol or umol/cm");
	}

	param=param->getChild(nutrient);
	sImax=pSD->existingSibling("Imax", "umol/cm2/day");
	if(!sImax){
		sImax=param->getChild("Imax", "umol/cm2/day");
		refTImax=pSD->getStartTime();
	}
	sKm=pSD->existingSibling("Km", "umol/ml");
	if(!sKm) {
		sKm=param->getChild("Km", "umol/ml");
		refTKm=pSD->getStartTime();

	}
	param->getChild("Cmin", "umol/ml")->get(Cmin);
	competition=param->existingChild("competition");
	if(competition){
		bool cmp;
		competition->get(cmp);
		if(!cmp) competition=nullptr;
	}else{
		competition=pSD; //default is on
	}
	sv0=pSD->getParent(2)->existingChild("rootWaterUptake","cm3/cm");
	//soil parameters
	SimulaBase* paramsoil=ORIGIN->getChild("environment")->getChild("soil")->getChild(nutrient);

	sDe=paramsoil->existingChild("diffusionCoefficient", "cm2/day");
	paramsoil->getChild("bufferPower")->get(lt,depth,b);   // TODO get it from solute.getSoilBufferPower()
	if(!sDe){
		//see if other definition of De is present.
		//De_barber = Dsimunek *theta / b
		sDe=paramsoil->existingChild("saturatedDiffusionCoefficient", "cm2/day");
		mode=1;
		if(!sDe) msg::error("BarberCushmanExplicit: Neither saturatedDiffusionCoefficient nor diffusionCoefficient found for "+nutrient);
	}
	paramsoil->getChild("r1-r0", "cm")->get(lt,depth,r1);
	r1+=r0;

	//volumetric water content
	//note barber-cushman can not simulate variable water content.
	if(theta<0){
		theta=0.3; //default is one, which is not a good default, but does make the code backward compatible.
		SimulaBase* p(paramsoil->existingSibling("water"));
		if(p){
			p=p->existingChild("volumetricWaterContentInBarberCushman");
			if(p){
				p->get(lt,depth,theta);
			}else{
				msg::warning("BarberCushmanExplicit: ../soil/water/volumetricWaterContentInBarberCushman not given, using 1 for backward compatibility reasons.");
			}
		}
	}

	paramsoil->getChild("concentration","umol/ml")->get(lt,depth,Ci);
	if (Ci < 0)msg::error("BarberCushmanExplicit: initial concentration < 0");

	//estimate dr automatically
	unsigned int k=30;//from experience 30 is a bit small, 60 is better, but takes much memory and time
	dr=(r1-r0)/(double)k;
	const double mdr(2.*r0);
	if(dr>mdr) {
		dr=mdr;
		k=ceil(r1/mdr);
		if(k>90) k=90;
	};
	/*if(nutrient!="nitrate"){
		if(dr>0.9*r0) dr=0.9*r0;
		if(dr<0.3*r0){
			dr=0.3*r0;
		}
	}else{
		if(dr>0.9*r0) msg::warning("BarberCushman: running nitrate, but this model does not do well on nitrate. Using too large dr to avoid a crash");
	}*/

	//determine k
	//k=(int)ceil((r1-r0)/dr);
	//if(k>100) msg::warning("Barber-Cushman: using large arrays: k>100");
	r1=r0+(double)k*dr;
	++k;

	//root competition
	spatialRootDensity=pSD->getParent(2)->existingChild("spatialRootDensity","cm/cm3");
	if (spatialRootDensity) {
		double nr1;
		spatialRootDensity->get(lt, nr1);
		if (nr1 > 1.) {
			nr1 = sqrt(1. / nr1) / PI;
			if (nr1 < r1) {
				unsigned int nk = (unsigned int) ceil(
						std::max(0., nr1 - r0) / dr);
				if (nk < 3)
					nk = 3;
				if (k > nk) {
					r1 = r0 + (double) nk * dr;
					k = nk;
					if (k < 4)
						msg::warning(
								"BarberCushmanExplicit: severe competition: k<4, you may want to use smaller delta r for "
										+ pSD->getParent()->getName());
				}
			}

			Coordinate center;
			pSD->getAbsolute(pSD->getStartTime(), center);

			SimulaBase::Positions list, list2;
			double radius(1.);
			pSD->getPositionsWithinRadius(pSD->getStartTime(), center,
					radius, list);
			//eliminate nodes from same root
			SimulaBase *root1(pSD->getParent(3)), *root2;
			for (auto &it : list) {
				root2 = it.second->getParent(2);
				//if(root1==root2) list.erase(it--);//note this must be post decrement of iterator, it will decrement the iterator and than return old copy to erase.
				if (root1 != root2)
					list2.insert(it);
			}
			//total uptake in umol
			double uptake(0), u;
			for (auto & it : list2) {
				SimulaBase *p(
						it.second->getChild(nutrient)->getChild(
								"rootSegmentNutrientUptake"));
				if (p != pSD) { //do not include selve.
					p->get(lt, u);
					uptake += u;
				};
			}
			//adjust Ci by subtracting uptake.
			double volume = radius * radius * radius * (4 / 3) * PI; // volume of the sphere in which uptake took place in cm3
			double CiOri(Ci);
			Ci -= ((uptake / volume) / b);
			if (Ci < 0.1 * CiOri)
				msg::warning(
						"BarberCushmanExplicit: adjusting Ci to less than 10% of original Ci");
			if (Ci < 0)
				Ci = 0;
		}
	}

	//exudates factor
	SimulaBase* p=param->existingChild("exudatesFactor");
	if(p){//found factor for exudates
		p->get(lt,depth,dummy);
		Ci*=dummy;
		b/=dummy;
	}

	//circumference
	circ=r0*2*PI;

	//resize arrays and set initial concentrations
	X_old.resize(k,Ci);

	//roothair parameter
	slh = pSD->getParent(2)->existingChild("rootHairLength","cm");
	if(slh){
		srh0 = pSD->getParent(2)->getChild("rootHairDiameter","cm");
		sNh = pSD->getParent(2)->getChild("rootHairDensity");
	}else{
		sNh=nullptr;
	}
	if(sNh){
		if(sNh->getUnit()=="#/cm2"){
			rootHairDensityConversionFactor=r0*2*PI;
		}else if(sNh->getUnit()=="#/cm"){
			rootHairDensityConversionFactor=1;
		}else{
			msg::error("BarberCushmanExplicit: unknown unit for root hair density. Should be #/cm or #/cm2.");
		}
	}

	//initial flux density at time 0
	double Imax,Km;
	sImax->get(lt-refTImax,Imax);
	sKm->get(lt-refTKm,Km);
	res=circ*(Imax*Cmin/(Km+Cmin));

	//depletion zone
	SimulaBase* dz(pSD->existingSibling("radiusDepletionZone"));
	if(dz){
		depletionZone= dynamic_cast < SimulaTable<Time> * > (dz);
	}else{
		depletionZone=nullptr;
	}
	if(depletionZone) {
		depletionZone->set(pSD->getStartTime(),r0);
	}else{
		msg::warning("Depletion zone not found");
	}

	msgcount=1;

	//debugging files (additional output)
	bool writeFiles=false;
	p=param->existingChild("writeFiles");
	if(p) p->get(writeFiles);

	if(writeFiles){
		std::string filename("barber_cushman_explicit.tab");
		os.open( filename.c_str() );
		if ( !os.is_open() ) msg::warning("generateTable: Failed to open "+filename);
		if ( os.is_open() ){
			os<<std::endl<<0.0;
			for(auto Cn_i: X_old){
				os<<"\t"<<Cn_i;
			}
		}
	}

} // end constructor


std::string BarberCushmanSolver::getName()const{
	return "BarberCushmanSolver";
}


void BarberCushmanSolver::predict(const Time&,double &){
	msg::error("BarberCushmanExplicit: Predicting requested but no code for a prediction implemented");
}
void BarberCushmanSolver::predictRate(const Time&,double &){
	msg::error("BarberCushmanExplicit: Predicting requested but no code for a prediction implemented");
}

BarberCushmanSolver::~BarberCushmanSolver(){
	if ( os.is_open() ) os.close();
}


void BarberCushmanSolver::integrate(SimulaVariable::Table & data, DerivativeBase & rateCalculator){

	if (firststep){
		doDelayedConstruction();
	}

	//time
	double st=data.rbegin()->first, rt;
	const Time bt(std::max(st-pSD->maxTimeStep(), pSD->getStartTime()));
	double h;//timestep (dt)
	timeStep(st, rt, h); // synchroniziation
	predictor = new SimplePredictor(data, h);
	Time mt=firststep?st:st+0.5*h;
	const double h_ori=h;

	//adjust k if competition increased
	auto k=X_old.size();
	if (spatialRootDensity) {
		double nr1;
		spatialRootDensity->get(bt, nr1);
		if (nr1 > 1.) {
			nr1 = sqrt(1. / nr1) / PI;
			if (nr1 < r1) {
				unsigned int nk = (unsigned int) ceil(
						std::max(0., nr1 - r0) / dr);
				if (nk < 3)
					nk = 3;
				if (k > nk) {
					r1 = r0 + (double) nk * dr;
					k = nk;
					if (k < 4)
						msg::warning(
								"BarberCushmanExplicit: severe competition: k<4, you may want to use smaller delta r for "
										+ pSD->getParent()->getName());
					//note that Cn needs to be resized, but resize on a valarray invalidates the content, and there is not erase on a valarray
					auto Cntmp = std::move(X_old);
					X_old.resize(k);
					for (unsigned int i(0); i < k; ++i) {
						X_old[i] = Cntmp[i];
					}
				}
			}
		}
	}

	//current root radius
	double rdz;
	rootDiameter->get(bt,rdz);rdz/=2.;

	//check the soil concentration
	if(rdz>2.*r0 || Ci<=Cmin || k<4){
		//secondary growth destroyed cortex, assume uptake is zero
		// or
		//competition reduced uptake to 0
		const double fluxDensity=0.;
		//the depletion zone will take some time to disappear
		const Time newTime(st+pSD->maxTimeStep());
		if (depletionZone) depletionZone->set(st+0.5,rdz);
		//integration
		firststep = false;
		StateRate & oldValue=data.rbegin()->second;
		StateRate newValue;
		newValue.state=oldValue.state+h*(oldValue.rate+fluxDensity)/2; // trapezoidal sum rule
		newValue.rate=fluxDensity;
		bool r=pSD->keepEstimate(newTime); //true if the values depend on predictors from objects earlier in the calling sequence
		if(r) msg::warning("BarberCushmanExplicit:: ignoring estimate as no restart function as been build in. ",2);
		data.insert(std::pair<Time,StateRate>(newTime,newValue));
		delete predictor;
		predictor=nullptr;
		return;
	}

	//root radius and the distance of all finite element nodes from the center of the root node
	//can go into constructor
	//the only part does goes here is that k is changed, so the array might have to be shortened.
	Array r(0.,k);
	r[0] = r0;
	for (unsigned int i(1); i < k; ++i) {
		r[i] = r[i - 1] + dr;
	}

	//Get the different parameters
	///@todo this may be numerically not completely valid - barber assumes Imax, Km and E V0 De constant over time.
	//Now it is not, but we assume them constant during the timestep
	double Imax,Km,v0(0.00864),De;
	sImax->get(mt-refTImax,Imax);
	sKm->get(mt-refTKm,Km);
	if(sv0) {
		sv0->getRate(st,v0);// cm3 water/cm root/day
		//todo check this: divide v0 with the assumed constant volumetric water content so amount of water taken up is expressed in cm3 soil volume. This because the concentration is expressed in cm3 soil volume
		//We need to do this to make cm3 equal not refer to amount of water but an amount of water per cubic cm3 of soil.
		v0/=r[0];//cm3/cm/day / cm -> cm/day
	}
	sDe->get(mt,depth,De);
	if(mode==1){
		//De_barber = Dsimunek *theta / b
		De*=theta/b;
	}

	//*************************************
	//constants

	//roothairs
	//TODO Imax and Km etc could be different for roothairs
	const double Imaxh(Imax);

	const double Kmh(Km), Cminh(Cmin);
	const Array vol((PI*square(r+dr))-(PI*square(r)));
	Array S4(0.,k),Ah(0.,k),Ih(0.,k);
	if(slh){
		double Nh;
		sNh->get(mt,Nh);
		if(Nh>1.){
			Nh*=rootHairDensityConversionFactor;
			double lh(0.);
			slh->get(mt,lh);
			double rh0;
			srh0->get(mt,rh0);rh0/=2.;
			const Array rh1(sqrt(r*PI/(2*Nh)));
			unsigned int krh(0);
			for (unsigned int i(0); i < k; ++i){
				Ah[i]=(Nh*std::min(dr,std::max(0.,(lh+r0)-r[i]))*2*PI*rh0)/vol[i];
				if(Ah[i]<1e-9) {
					krh=i;
					break;
				}
			}
			for (unsigned int i(0); i < krh; ++i){
				//log is taking 7% of the bc code computational time just here
				//if Nh, rh0, r0, and k are constant, we would not have to repeat this.
				S4[i]=((Imaxh*rh0)/(De*b))*log(rh1[i]/(1.6487212707*rh0));
				const double Ch=calcCh(X_old[i],Km,Cmin,S4[i]);
				//const double dCh=calcdCh(Cn[i],Kmh,Cminh,S4[i]);
				Ih[i]=calcIh(Ch,Imaxh,Kmh,Cminh,Ah[i]);
				//dIh[i]=calcdIh(Ch,dCh,Imaxh,Kmh,Cminh,Ah[i]);
			}
		}
	}

	// RK solving procedure ...

	Array Pe(0.,k);
	double vmax(0.);
	for(std::size_t i=0; i < k; ++i) {
		double v = fabs( 1/r[i]*(De+v0*r0/b) ); // theoretically v0 can be negative
		Pe[i] = v*dr/De ; // <= 2
		if(Pe[i] > 2) msg::warning("BarberCushmanExplicit: Peclet criteria not fulfilled. Upwind used in compartment.",2);
		vmax = vmax > v ? vmax : v;
	}


	Array w1(0.,k);
	Array w2(0.,k);
	Array w3(0.,k);
	Array w4(0.,k);
	const double De_dr2 = De/(dr*dr);
	const double adv=(De+v0*r0/b) /dr;
	for(size_t i=0; i < k; ++i){
		const double advection_term =  adv/r[i];
		if(advection_term > 0){ //  upwind in root direction
			if(MIXED && Pe[i]<2.){
				if(Pe[i]>1.8){
					w1[i] = advection_term*(-0.5*0.5)     + De_dr2;
					w2[i] = advection_term*(-1.5*0.5)    -2.*De_dr2;
					w3[i] = advection_term*(2.5*0.5)      + De_dr2;
					w4[i] = advection_term*(-0.5*0.5);
				}else{
					// central
					w1[i] = advection_term*(-0.5)       +De_dr2;
					w2[i] =                           -2.*De_dr2;
					w3[i] = advection_term*(0.5)        +De_dr2;
					//w4[i] = 0;
				}
			}else{
				// upwind
				w1[i] =                                De_dr2;
				w2[i] = advection_term*(-1.5)       -2.*De_dr2;
				w3[i] = advection_term*2.0            +De_dr2;
				w4[i] = advection_term*(-0.5);
			}
		}
		else{ // downwind
			// use central differences as workaround !
			w1[i] = advection_term*(-0.5)       +De_dr2;
			w2[i] =                           -2.*De_dr2;
			w3[i] = advection_term*(0.5)        +De_dr2;
			//w4[i] = 0.;
			msg::warning("BarberCushmanExplicit: downwind! but central differences are used for advection.",2);
		}
	}

	const double TOL =  1e-4; // 1e-5; // relative error tolerance of RK45


	// first time step size guess (ref.: Hairer I)
	if (firststep && automatic_startstep) {

		// FUNCTION EVALUATION AT THE INITIAL POINT
		Array IhRK(0.,k);
		Array X(Ci,k);
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);

		Array k1(0.,k);
		rk_step(k1, X, w1, w2, w3, w4, De, b, Cmin, r0, v0, Km, Imax, dr, IhRK, k, r);

		double d0 = L2Norm(X);
		double d1 = L2Norm(k1);
		double h0;
		if (d0 < 1e-5 || d1 < 1e-5) {
			h0 = 1e-6;
		} else {
			h0 =  d0 / d1; // norm(X)/norm(k1);
			if(h0>pSD->maxTimeStep()) h0=pSD->maxTimeStep();
			h0*=0.01;
		}

		// PERFORM EXPICIT EULER STEP y1= y0+ h0*k1(x0,y0)
		X = X + h0 * k1;

		// COMPUTE f(x0+h0, y1) , where y1 := X
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);

		Array k2(0.,k);
		rk_step(k2, X, w1, w2, w3, w4, De, b, Cmin, r0, v0, Km, Imax, dr, IhRK, k, r);

		double d2 = L2Norm(k2 - k1) / h0; // as an estimate of the second derivative of the solution

		const double h1 =
				(std::max(d1, d2) < 1e-15) ?
						std::max(1e-6, h0 * 1e-3) :
						std::pow(0.01 / std::max(d1, d2), 0.2); // q+1=5 is order of local error

		//todo don't want h to be larger than hmax
		pSD->preferedTimeStep() = std::min(100 * h0, h1);
		timeStep(st, rt, h); // synchroniziation
		predictor->resetTimeStep(h);
		mt=st+0.5*h;
	}
	firststep = false;


	bool rejection=true;
	double facmax = 2.0; // usually between 1.5 and 5
	const double safety = 0.9; // or (0.25)^(0.25); // given safety factor


	// ********* loop for re-calculating am h-step *************************
	while(rejection){
		//X_0 = X_old;

		// step 1
		// X = X_old;
		Array IhRK(0.,k);
		calcIhRK(IhRK, X_old, S4, Imaxh, Ah, Kmh, Cminh, k);
		// F( t , X_0 )
		Array k1(0.,k);
		rk_step(k1, X_old, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);
		// step 2
		Array X = X_old + h*A11 *k1;
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);
		// F( t+h*C1 , X_0+h*A11 *k1 )
		Array k2(0.,k);
		rk_step(k2, X, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);
		// step 3
		X = X_old + h*(A21*k1 + A22*k2);
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);
		// F( t+ C2*h , X_0 + h*(A21*k1 + A22*k2) )
		Array k3(0.,k);
		rk_step(k3, X, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);
		//  step 4
		X = X_old + h*(A31*k1 +A32*k2 + A33*k3);
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);
		// F( t+C3*h , X_0 + h*(A31*k1 +A32*k2 + A33*k3) )
		Array k4(0.,k);
		rk_step(k4, X, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);
		//  step 5
		X = X_old + h*(A41*k1 +A42*k2 +A43*k3 + A44*k4);
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);
		// F( t+C4*h , X_0+ h*(A41*k1 +A42*k2 +A43*k3 + A44*k4) )
		Array k5(0.,k);
		rk_step(k5, X, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);
		// k5 is needed for the lower order solution and is computed with a
		// full h-step, see C4:=1.
		//  step 6
		X = X_old + h*(A51*k1 + A52*k2 + A53*k3 + A54*k4 + A55*k5);
		calcIhRK(IhRK, X, S4, Imaxh, Ah, Kmh, Cminh, k);
		// F( t+C5*h , X_0+ h*(A51*k1 + A52*k2 + A53*k3 + A54*k4 + A55*k5) )
		Array k6(0.,k);
		rk_step(k6, X, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);

		X = h*(B11*k1+ B13*k3 + B14*k4          + B16*k6); // 5th
		const Array Xo45 = X - h*(B21*k1+ B23*k3 + B24*k4 + B25*k5 + B26*k6); // 4th order solution
		X+= X_old;//solution

		const double local_error = L2Norm(Xo45)/L2Norm(X);

		// step size control algorithm:
		// ----------------------------
		const double h_min =  pSD->minTimeStep();
		if (local_error <= TOL){    // time step solved
			//determine time step for next round
			{
				double h_new(h);
				//if we used the original timestep, and not a reduced one, see if we can increase it base on local_error estimate
				if(h+TIMEERROR>h_ori){
					//this should only happen when we did not use a small h because of synchronization of the timestep.
					double h0 = 1 + safety* (std::pow((TOL/local_error),0.2)-1);
					h_new = h*std::min(h0, facmax); // prevents the code from too large steps
				}
				//reduce time step to meet the CFL criteria
				if(fabs( vmax*(h_new/dr) ) > 1. ){
					h_new = dr/vmax;
					if(h_new<pSD->minTimeStep()) msg::warning("BarberCushmanExplicit: min timestep does not allow CFL criteria to be met for at least one compartment. A smaller min dt is needed or a larger dr."); // <=1, CFL condition, same as courant number
				}
				// set dt for global stepping precedure
				pSD->preferedTimeStep() = h_new;
			}
			X_old = std::move(X);//update x_old to new solution
			rejection = false; //exit loop
			//std::cout<<std::endl<<this<<"h "<<std::scientific<<h<<" hnew "<<h_new<<" local error "<<local_error<<" Csurf "<<X_old[1]<<"K"<<k;
		}else if(h<h_min+TIMEERROR){
			//do not change the time step h for it reached its minimum
			msg::warning("BarberCushmanExplicit: using miniminum timestep and not meeting tolerance criteria. Results might be instable");
			X_old = std::move(X);//update x_old to new solution
			rejection = false;//exit loop
		}else{  // error is large  redo  with a smaller time step h
			const double h0 =  safety* std::pow((TOL/local_error),0.25);//how (100%) much to decrease the step size
			h *=  std::max(h0, 0.2); // maximum reduction is 0.2
			pSD->preferedTimeStep() = h; //used in timeStep();
			timeStep(st, rt, h); // synchronization
			predictor->resetTimeStep(h);
			mt=st+0.5*h;  //update t+0.5dt (used to get root segment length below)
			rejection = true;//redo with a smaller timestep
			facmax=1.;
			msg::warning("BarberCushmanExplicit: retrying with a smaller timestep",3);
		}
	} // **** end while loop ***********************

	//calculate flux into the root using second order (r0 is currently circumference - see above)
	double Ca(X_old[0]-Cmin);
	double fluxDensity=circ*(Imax*Ca/(Km+Ca));
	if(Ah[0]){
		//include uptake by hairs
		for(unsigned int i(0); i<k && Ah[i]>0 ; ++i){
			double d1=calcCh(X_old[i],Kmh,Cminh,S4[i]);
			double d3=Ah[i]*vol[i];
			d1=calcIh(d1,Imaxh,Kmh,Cminh,d3);
			fluxDensity+=(d1);
		}
	}

	if ( os.is_open() ){
		os<<std::endl<<st;
		for(auto Cn_i: X_old){
			os<<"\t"<<Cn_i;
		}
	}

	//convert to per cm to per root segment length
	if(rootLength){
		double l;
		rootLength->get(mt,l);
		fluxDensity*=l;
	}

	//do some checks
	if(std::isnan(fluxDensity))msg::error("BarberCushmanExplicit: Results is NaN");
	if(fluxDensity<0.) {
		msg::warning("BarberCushmanExplicit: getting negative values for uptake, returning 0");
		fluxDensity=0.;
	}

	//check for increasing concentrations. This happens when dr is too large
	if(X_old[0]>1.1*Ci)
		msg::warning("BarberCushmanExplicit: concentrations at root surface are increasing. massflow>uptake? or dr too large?");

	//find edge of depletion zone
	if(depletionZone){
		std::size_t edgeDepletionZone(0);
		for (int i(k-1); i >= -1; --i) {
			if (X_old[i]< 0.95*Ci){
				edgeDepletionZone=i;
				break;
			}
		}
		if(edgeDepletionZone>=k-1){ //edge is not found and competition is effective
			msg::warning("BarberCushmanExplicit: competition effective. (edge depletion == r1)", 2);
		}
		rdz+=r[edgeDepletionZone]-r[0];
		depletionZone->set(rt,rdz);
	}

	StateRate & oldValue=data.rbegin()->second;
	const StateRate newValue(oldValue.state+h*(oldValue.rate+fluxDensity)/2,fluxDensity);
	bool er=pSD->keepEstimate(rt); //true if the values depend on predictors from objects earlier in the calling sequence
	if(er) msg::warning("BarberCushmanExplicit:: ignoring estimate as no restart function as been build in. ",2);

	data.insert(std::pair<Time,StateRate>(rt,newValue));

	delete predictor;
	predictor = nullptr;
	return;
}


void BarberCushmanSolver::rk_step(Array &k_, const Array &X,const Array &w1,const Array &w2,const Array &w3,const Array &w4,const double &De, const double &b, const double &Cmin, const double &r0, const double &v0, const double &Km,const double &Imax ,const double &dr, const Array &IhRK,const std::size_t &k,const Array &r)
{
	const double B1=2 * dr / (De * b);
	{
		const double Cea = X[0] - Cmin;
		const double C_0 = X[1] - B1 * (Imax * (Cea) / (Km + Cea) - v0 * X[0]);

		k_[0] = w1[0] * C_0 + w2[0] * X[0] + w3[0] * X[1] + w4[0] * X[2];
		for (std::size_t i = 1; i < k - 2; ++i) { // because k-1 is the end of the vectors
			k_[i] = w1[i] * X[i - 1] + w2[i] * X[i] + w3[i] * X[i + 1] + w4[i] * X[i + 2];
		}
	}
	{
		const double B2=B1 * r0 * v0;
		const double C_outer1 = X[k - 2] - B2 / r[k - 1] * X[k - 1];
		const double C_outer2 = X[k - 1] - B2 / r[k - 1] * C_outer1; // 2bd GHOST POINT --> workaround for last inner upwind point (central differences for ghost)

		k_[k - 2] = w1[k - 2] * X[k - 3] + w2[k - 2] * X[k - 2] + w3[k - 2] * X[k - 1] + w4[k - 2] * C_outer1;
		k_[k - 1] = w1[k - 1] * X[k - 2] + w2[k - 1] * X[k - 1] + w3[k - 1] * C_outer1 + w4[k - 1] * C_outer2;
	}
	//roothairs
	k_-=IhRK / b;
	return ;
}

//***************************************************************************/
void BarberCushmanSolver::calcIhRK(Array &Ih, const Array &X, const Array &S4, const double &Imaxh, const Array &Ah, const double &Km, const double  &Cmin, const std::size_t &k){
	for(std::size_t i = 0; i < k && Ah[i]>1e-8; ++i){
		double p = X[i]-Km+Cmin-S4[i];
		double z = (p*p)/4 + X[i]*(Km-Cmin) + S4[i]*Cmin ;
		if(z < 0){
			msg::error("calcClrV sqrt of negative value.");
		}
		double Clr   = 0.5*(p)+ sqrt( z ) - Cmin;;
		Ih[i] = Imaxh*Clr/(Km+Clr)*Ah[i];
	}
	return ;
}

//***************************************************************************/
double BarberCushmanSolver::L2Norm(const Array &a) {
	if (a.size()){
		return (sqrt((a*a).sum()));
	}else{
		return 0.;
	}
}
//***************************************************************************/

double BarberCushmanSolver::calcCh(const double &C,const double &Km,const double &Cmin,const double &S4){
	return ( ( (  (C-Km+Cmin-S4)+sqrt(  square(-C+Km-Cmin+S4)-4*C*Cmin+4*S4*Cmin+4*C*Km) )/2) - Cmin);
}

double BarberCushmanSolver::calcIh(const double &Ch,const double &Imaxh,const double &Km,const double &Cmin,const double &Ah){
	return  ((Ah*Imaxh*Ch)/(Km+Ch));
}




IntegrationBase * newInstantiationBarberCushmanSolver(){
	return new BarberCushmanSolver();
}

//Register the module
static class AutoRegisterIntegrationFunctionsbcs {
public:
	AutoRegisterIntegrationFunctionsbcs() {
		BaseClassesMap::getIntegrationClasses()["BarberCushmanSolver"] = newInstantiationBarberCushmanSolver;
	}
}p44608510843654654046454385;

