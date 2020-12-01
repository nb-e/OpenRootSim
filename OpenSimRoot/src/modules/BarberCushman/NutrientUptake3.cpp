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

// Butcher Tableau:
// 	0   |
//	1/2 |	1/2
//	3/4 |	0       3/4
//	1 	|   2/9 	1/3 	4/9
//-------------------------------------
//		|   2/9 	1/3 	4/9 	0
//		|   7/24 	1/4 	1/3 	1/8

Barber_cushman_1981_nutrient_uptake_ode23::Barber_cushman_1981_nutrient_uptake_ode23(SimulaDynamic* pSD) :TotalBaseLabeled(pSD){
}

void Barber_cushman_1981_nutrient_uptake_ode23::calculate(const Time &t, double &fluxDensity) {
}

//***************************************************************************/


std::string Barber_cushman_1981_nutrient_uptake_ode23::getName()const{
	return "barber_cushman_1981_nutrient_uptake_ode23";
}

DerivativeBase * newInstantiationBarber_cushman_1981_nutrient_uptake_ode23(SimulaDynamic* const pSD) {
	return new Barber_cushman_1981_nutrient_uptake_ode23(pSD);
}



//Register the module
static class AutoRegisterNutrientClassesInstantiationFunctionszj5 {
public:
	AutoRegisterNutrientClassesInstantiationFunctionszj5() {
		BaseClassesMap::getDerivativeBaseClasses()["barber_cushman_1981_nutrient_uptake_ode23"]	= newInstantiationBarber_cushman_1981_nutrient_uptake_ode23;
	}

}p7bg876h868h876h1;


double BarberCushmanSolverOde23::theta = -1;



BarberCushmanSolverOde23::BarberCushmanSolverOde23():
IntegrationBase(),dr(0),b(0),Ci(0),r0(0),r1(0),circ(0),rootHairDensityConversionFactor(0),
Cmin(0),res(0),increaseTimeStep(0),
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
lt(-1),
k(0),
msgcount(0),
h(0),
h_new(0),
os(),
mode(0),
refTImax(0.),
refTKm(0.)
{
}

void BarberCushmanSolverOde23::doDelayedConstruction(){
	//set initial time
	lt=pSD->getStartTime();
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
	if(r0<=0)msg::error("BarberCushmanode23: root diameter is <= 0");

	if(pSD->getUnit()=="umol"){
		rootLength=pSD->getParent(2)->getChild("rootSegmentLength", "cm");
	}else{
		rootLength=nullptr;
		if(pSD->getUnit()!="umol/cm") msg::error("BarberCushmanode23: "+pSD->getPath()+" should have unit umol or umol/cm");
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
		if(!sDe) msg::error("BarberCushmanode23: Neither saturatedDiffusionCoefficient nor diffusionCoefficient found for "+nutrient);
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
				msg::warning("BarberCushmanSolverOde23: ../soil/water/volumetricWaterContentInBarberCushman not given, using 1 for backward compatibility reasons.");
			}
		}
	}

	paramsoil->getChild("concentration","umol/ml")->get(lt,depth,Ci);
	if (Ci < 0)msg::error("BarberCushmanExplicit: initial concentration < 0");

	//estimate dr automatically
	k=30;//from experience 30 is a bit small, 60 is better, but takes much memory and time
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
								"BarberCushmanSolverOde23: severe competition: k<4, you may want to use smaller delta r for "
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
						"BarberCushmanSolverOde23: adjusting Ci to less than 10% of original Ci");
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
			msg::error("BarberCushmanSolverOde23: unknown unit for roothair density. Should be #/cm or #/cm2.");
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
		std::string filename("barber_cushman_ode23.tab");
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


std::string BarberCushmanSolverOde23::getName()const{
	return "BarberCushmanSolverOde23";
}


void BarberCushmanSolverOde23::predict(const Time&,double &){
	msg::error("BarberCushmanSolverOde23: Predicting requested but no code for a prediction implemented");
}
void BarberCushmanSolverOde23::predictRate(const Time&,double &){
	msg::error("BarberCushmanSolverOde23: Predicting requested but no code for a prediction implemented");
}

BarberCushmanSolverOde23::~BarberCushmanSolverOde23(){
	if ( os.is_open() ) os.close();
}


void BarberCushmanSolverOde23::integrate(SimulaVariable::Table & data, DerivativeBase & rateCalculator){

	if (lt<0.){
		doDelayedConstruction();
	}

	//time
	double h=pSD->preferedTimeStep();//timestep (dt)
	double st=data.rbegin()->first;
	double rt=st+h;
	timeStep(st, rt, h); // synchroniziation
	const Time bt(std::max(st-pSD->maxTimeStep(), pSD->getStartTime()));
	predictor = new SimplePredictor(data, h);
	Time mt=st+0.5*h; //firststep?st:st+0.5*h;
	const double h_ori=h;

	//adjust k if competition increased
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
								"BarberCushmanSolverOde23: severe competition: k<4, you may want to use smaller delta r for "
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
		StateRate & oldValue=data.rbegin()->second;
		StateRate newValue;
		newValue.state=oldValue.state+h*(oldValue.rate+fluxDensity)/2; // trapezoidal sum rule
		newValue.rate=fluxDensity;
		bool r=pSD->keepEstimate(newTime); //true if the values depend on predictors from objects earlier in the calling sequence
		if(r) msg::warning("BarberCushmanSolverOde23: ignoring estimate as no restart function as been build in. ",2);
		data.insert(std::pair<Time,StateRate>(newTime,newValue));
		delete predictor;
		predictor=nullptr;
		return;
	}

	//root radius and the distance of all finite element nodes from the center of the root node
	//can go into constructor
	//the only part does goes here is that k is changed, so the array might have to be shortened.
	std::valarray<double> r(0.,k);
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
	double Imaxh(Imax);

	//double Kmh(Km), Cminh(Cmin);
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
				Ih[i]=calcIh(Ch,Imax,Km,Cmin,Ah[i]);
				//dIh[i]=calcdIh(Ch,dCh,Imaxh,Kmh,Cminh,Ah[i]);
			}
		}
	}

	// RK solving procedure ...

//	std::valarray<double> Pe(0.,k);
	double vmax(0.);
	for(std::size_t i=0; i < k; ++i) {
		double v = fabs( 1/r[i]*(De+v0*r0/b) ); // theoretically v0 can be negative
//		Pe[i] = v*dr/De ; // <= 2
//		if(Pe[i] > 2) msg::warning("BarberCushmanode23: Peclet criteria not fulfilled. Upwind used in compartment.",2);
		vmax = vmax > v ? vmax : v;
	}


	std::valarray<double> w1(0.,k);
	std::valarray<double> w2(0.,k);
	std::valarray<double> w3(0.,k);
	std::valarray<double> w4(0.,k);
	const double dr2 = dr*dr;
	for(size_t i=0; i < k; ++i){
		double advection_term =  1.0/r[i]*(De+v0*r0/b) /dr;
				// upwind (v0 in root direction, v0 > 0)
				w1[i] =                                De/dr2;
				w2[i] = advection_term*(-1.5)       -2.*De/dr2;
				w3[i] = advection_term*2.0            +De/dr2;
				w4[i] = advection_term*(-0.5);
	}

	// initialization of RK coeff's
	std::valarray<double> k1(0.,k);
	std::valarray<double> k2(0.,k);
	std::valarray<double> k3(0.,k);
	std::valarray<double> k4(0.,k);

	const double TOL =  1e-4; // 1e-5; // relative error tolerance of RK45

	std::valarray<double> IhRK(0.,k);

	bool rejection=true;
	double facmax = 2.0; // usually between 1.5 and 5
	const double safety = 0.9; // or (0.25)^(0.25); // given safety factor


	// ********* loop for re-calculating am h-step *************************
	while(rejection){
//		if (firststep) { // TODO FSAL needs work
			// FUNCTION EVALUATION AT THE INITIAL POINT
			calcIhRK(IhRK, X_old, S4, Imaxh, Ah, Km, Cmin, k);
			rk_step(k1, X_old, w1, w2, w3, w4, De, b, Cmin, r0, v0, Km, Imax, dr, IhRK, k, r);
//		else{
//			// FSAL—first same as last—property
//			k1 = k4;
//		}
		// step 2
		const std::valarray<double>  Xo0 = X_old + h*0.5 *k1;
		calcIhRK(IhRK, Xo0, S4, Imaxh, Ah, Km, Cmin, k);
		rk_step(k2, Xo0, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);
		// step 3
		const std::valarray<double>  Xo1 = X_old + h*0.75*k2;
		calcIhRK(IhRK, Xo1, S4, Imaxh, Ah, Km, Cmin, k);
		rk_step(k3, Xo1, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);

		std::valarray<double>  Xo3 = X_old + h*( 2./9.*k1 + 1./3.*k2 + 4./9.*k3); //3rd order solution
		//  step 4
		calcIhRK(IhRK, Xo3, S4, Imaxh, Ah, Km, Cmin, k);
		rk_step(k4, Xo3, w1,w2,w3,w4,De,b,Cmin,r0,v0,Km,Imax ,dr, IhRK, k, r);

		const std::valarray<double>  Xo2 = X_old + h*(7./24.*k1+0.25*k2+1./3.*k3+0.125*k4);//2nd order solution

		const double local_error = L2Norm(Xo3 - Xo2)/L2Norm(Xo3);
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
					double h0 = 1 + safety* (std::pow((TOL/local_error),0.25)-1);
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
			X_old = std::move(Xo3);//update x_old to new solution
			rejection = false; //exit loop
			//std::cout<<std::endl<<this<<"h "<<std::scientific<<h<<" hnew "<<h_new<<" local error "<<local_error<<" Csurf "<<X_old[1]<<"K"<<k;
		}else if(h<h_min+TIMEERROR){
			//do not change the time step h for it reached its minimum
			msg::warning("BarberCushmanExplicit: using miniminum timestep and not meeting tolerance criteria. Results might be instable");
			X_old = std::move(Xo3);//update x_old to new solution
			rejection = false;//exit loop
		}else{  // error is large  redo  with a smaller time step h
			const double h0 =  safety* std::pow((TOL/local_error),0.333);//how (100%) much to decrease the step size
			h *=  std::max(h0, 0.2); // maximum reduction is 0.2
			pSD->preferedTimeStep() = h; //used in timeStep();
			timeStep(st, rt, h); // synchronization
			predictor->resetTimeStep(h);
			mt=st+0.5*h;  //update t+0.5dt (used to get root segment length below)
			rejection = true;//redo with a smaller timestep
			facmax=1.;
			msg::warning("BarberCushmanSolverOde23: retrying with a smaller timestep",3);
		}
	} // **** end while loop ***********************

	if(fabs( vmax*(h_new/dr) ) > 1. ){
		h_new = dr/vmax; //meet the CFL criteria
		if(h_new<pSD->minTimeStep()) msg::warning("BarberCushmanSolverOde23: min timestep does not allow CFL criteria to be met for at least one compartment. A smaller min dt is needed or a larger dr."); // <=1, CFL condition, same as courant number
	}
	pSD->preferedTimeStep() = h_new; // set dt for global stepping precedure


	//calculate flux into the root using second order (r0 is currently circumference - see above)
	double Ca(X_old[0]-Cmin);
	double fluxDensity=circ*(Imax*Ca/(Km+Ca));
	if(Ah[0]){
		//include uptake by hairs
		for(unsigned int i(0); i<k && Ah[i]>0 ; ++i){
			double d1=calcCh(X_old[i],Km,Cmin,S4[i]);
			double d3=Ah[i]*vol[i];
			d1=calcIh(d1,Imaxh,Km,Cmin,d3);
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
	if(std::isnan(fluxDensity))msg::error("BarberCushmanSolverOde23: Results is NaN");
	if(fluxDensity<0) {
		msg::warning("BarberCushmanSolverOde23: getting negative values for uptake, returning 0");
		fluxDensity=0.;
	}

	//check for increasing concentrations. This happens when dr is too large
	if(X_old[0]>1.1*Ci)
		msg::warning("BarberCushmanSolverOde23: concentrations at root surface are increasing. massflow>uptake? or dr too large?");

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
			msg::warning("BarberCushmanSolverOde23: competition effective.");
		}
		rdz+=r[edgeDepletionZone]-r[0];
		depletionZone->set(rt,rdz);
	}

	StateRate & oldValue=data.rbegin()->second;
	StateRate newValue;
	newValue.state=oldValue.state+h*(oldValue.rate+fluxDensity)/2; // trapezoidal sum rule
	newValue.rate=fluxDensity;
	bool er=pSD->keepEstimate(rt); //true if the values depend on predictors from objects earlier in the calling sequence
	if(er) msg::warning("BarberCushmanSolverOde23:: ignoring estimate as no restart function as been build in. ",2);

	data.insert(std::pair<Time,StateRate>(rt,newValue));

	delete predictor;
	predictor = nullptr;
	return;
}




void BarberCushmanSolverOde23::rk_step(std::valarray<double> &k_, const std::valarray<double> &X,const std::valarray<double> &w1,const std::valarray<double> &w2,const std::valarray<double> &w3,const std::valarray<double> &w4,const double &De, const double &b, const double &Cmin, const double &r0, const double &v0, const double &Km,const double &Imax ,const double &dr, const std::valarray<double> &IhRK,const std::size_t &k,const std::valarray<double> &r)
{
	double Cea = X[0]-Cmin ;
	double C_0 = X[1] -2*dr/(De*b) *( Imax*(Cea)/(Km+Cea) -v0*X[0]  );

	k_[0] = w1[0]*C_0+w2[0]*X[0]+w3[0]*X[1]+w4[0]*X[2] - IhRK[0]/b;
	for( std::size_t i=1; i < k-2; ++i) { // because k-1 is the end of the vectors
		k_[i] = w1[i]*X[i-1]+w2[i]*X[i]+w3[i]*X[i+1]+w4[i]*X[i+2] - IhRK[i]/b;
	}
	double C_outer1 = X[k-2] - (2*dr)/(De*b)*r0*v0/r[k-1]*X[k-1];
	double C_outer2 = X[k-1] - (2*dr)/(De*b)*r0*v0/r[k-1]*C_outer1;  // 2bd GHOST POINT --> workaround for last inner upwind point (central differences for ghost)

	k_[k-2] =   w1[k-2]*X[k-3] + w2[k-2]*X[k-2]+w3[k-2]*X[k-1]  +w4[k-2]*C_outer1  - IhRK[k-2]/b;
	k_[k-1] =   w1[k-1]*X[k-2] + w2[k-1]*X[k-1]+w3[k-1]*C_outer1+w4[k-1]*C_outer2  - IhRK[k-1]/b;
	return ;
}

//***************************************************************************/
void BarberCushmanSolverOde23::calcIhRK(std::valarray<double> &Ih, const std::valarray<double> &X, const std::valarray<double> &S4, const double &Imaxh, const std::valarray<double> &Ah, const double &Km, const double  &Cmin, const std::size_t &k){
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
double BarberCushmanSolverOde23::L2Norm(const std::valarray<double> &a) {
	double sum;
	std::size_t n = a.size();
	if (n == 0){
		return 0.;
	}
	sum = a[0] * a[0];

	for (std::size_t i = 1; i < n; ++i){
		sum += a[i] * a[i];
	}
	sum = sqrt(sum);
	return sum;
}
//***************************************************************************/

double BarberCushmanSolverOde23::calcCh(const double &C,const double &Km,const double &Cmin,const double &S4){
	return ( ( (  (C-Km+Cmin-S4)+sqrt(  square(-C+Km-Cmin+S4)-4*C*Cmin+4*S4*Cmin+4*C*Km) )/2) - Cmin);
}

double BarberCushmanSolverOde23::calcIh(const double &Ch,const double &Imaxh,const double &Km,const double &Cmin,const double &Ah){
	return  ((Ah*Imaxh*Ch)/(Km+Ch));
}




IntegrationBase * newInstantiationBarberCushmanSolverOde23(){
	return new BarberCushmanSolverOde23();
}

//Register the module
static class AutoRegisterIntegrationFunctionsbcs2 {
public:
	AutoRegisterIntegrationFunctionsbcs2() {
		BaseClassesMap::getIntegrationClasses()["BarberCushmanSolverOde23"] = newInstantiationBarberCushmanSolverOde23;
	}
}p44608510843654654046454386;

