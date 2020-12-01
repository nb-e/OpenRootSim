/*
Copyright © 2016, The Pennsylvania State University
All rights reserved.

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
#if _WIN32 || _WIN64
#include <algorithm>
#endif

///todo should look at:  Yanai, R. D. 1994. A steady state model of nutrient uptake accounting for newly grown roots. Soil Science Society of America Journal 58:1562-1571.

//calculate fluxdensity at any point
	/*Reference:
	 * Barber,S.A, 1995. Soil Nutrient Bioavailability, A mechanistic approach, second edition. JOHN WILEY & SONS, INC, New York, page 65
	 * S.A. Barber and J.H. Cushman, “Nitrogen uptake model for agronomic crops,” in Modeling Waste Water Renovation- Land Treatment, ed. I.K. Iskandar (Wiley Interscience, New York, 1981), 382-409.
	 */
double Barber_cushman_1981_nutrient_uptake::theta(-1);

Barber_cushman_1981_nutrient_uptake::Barber_cushman_1981_nutrient_uptake(SimulaDynamic* pSD) :
TotalBaseLabeled(pSD),mode(0),refTImax(0),refTKm(0)

{
	pSD->avoidPredictorCorrectedLoops();
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
	if(r0<=0)msg::error("BarberCushman: root diameter is <= 0");

	if(pSD->getUnit()=="umol"){
		rootLength=pSD->getParent(2)->getChild("rootSegmentLength", "cm");
	}else{
		rootLength=nullptr;
		if(pSD->getUnit()!="umol/cm") msg::error("BarberCushman: "+pSD->getPath()+" should have unit umol or umol/cm");
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
		if(!sDe) msg::error("BarberCushman: Neither saturatedDiffusionCoefficient nor diffusionCoefficient found for "+nutrient);
	}
	paramsoil->getChild("r1-r0", "cm")->get(lt,depth,r1);
	r1+=r0;

	//volumetric water content
	//note BarberCushman can not simulate variable water content.
	if(theta<0){
		theta=0.3; //default is one, which is not a good default, but does make the code backward compatible.
		SimulaBase* p(paramsoil->existingSibling("water"));
		if(p){
			p=p->existingChild("volumetricWaterContentInBarberCushman");
			if(p){
				p->get(lt,depth,theta);
			}else{
				msg::warning("BarberCushman: ../soil/water/volumetricWaterContentInBarberCushman not given, using 1 for backward compatibility reasons.");
			}
		}
	}

	paramsoil->getChild("concentration","umol/ml")->get(lt,depth,Ci);
	if (Ci < 0)msg::error("BarberCushman: initial concentration < 0");

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
	//if(k>100) msg::warning("BarberCushman: using large arrays: k>100");
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
								"BarberCushman: severe competition: k<4, you may want to use smaller delta r for "
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
						"BarberCushman: adjusting Ci to less than 10% of original Ci");
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
	Cn.resize(k,Ci);

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
			msg::error("BarberCushman: unknown unit for roothair density. Should be #/cm or #/cm2.");
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

	//try  to choose 'smartly' the right timestep.
	p=paramsoil->existingChild("increaseTimeStep");
	if(p) {
		p->get(lt,depth,increaseTimeStep);
		//guess initial timestep
		pSD->preferedTimeStep()=std::max(std::min(100*dr*pSD->preferedTimeStep(),pSD->maxTimeStep()),pSD->minTimeStep());
	}else{
		increaseTimeStep=1.05;
		msg::warning("BarberCushman:: increaseTimeStep not set, defaulting to 1.05 and not adjusting timestep for root radius");
	}

	msgcount=1;

	//method
	SimulaTimeDriven * std=dynamic_cast < SimulaTimeDriven * > (pSD);
	const auto & m=std->getIntegrationFunction();
	if(m!="BackwardEuler"){
		msg::error("BarberCusman: Integration method not supported use BackwardEuler");
	}

	//debugging files (additional output)
	bool writeFiles=false;
	p=param->existingChild("writeFiles");
	if(p) p->get(writeFiles);

	if(writeFiles){
		std::string filename("barber-cushman_"+pSD->getParent(2)->getName()+"_"+pSD->getParent(4)->getName()+".tab");
		os.open( filename.c_str() );
		if ( !os.is_open() ) msg::warning("generateTable: Failed to open "+filename);
		if ( os.is_open() ){
			os<<std::endl<<0.0;
			for(auto Cn_i: Cn){
				os<<"\t"<<Cn_i;
			}
		}
	}

}
Barber_cushman_1981_nutrient_uptake::~Barber_cushman_1981_nutrient_uptake(){
	if ( os.is_open() ) os.close();
}

void Barber_cushman_1981_nutrient_uptake::calculate(const Time &t, double &fluxDensity) {
	//to clear predictions, as we will not redo this step. Predictions could come from root segment length.
	std::size_t nmbest=pSD->getPredictorSize();

	//check time step
	const Time dt(t-lt);
	if(dt<0){
		msg::error("BarberCushman: dt<0, wrong integration method?");
	}else if(dt<0.9*pSD->minTimeStep()){
	    //occurs when getRate is called. Just give the rate back. 
		fluxDensity=res;
		return; //return current flux
	}

	const Time bt(std::max(lt-pSD->maxTimeStep(), pSD->getStartTime()));
	const Time mt(lt+dt/2.);

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
								"BarberCushman: severe competition: k<4, you may want to use smaller delta r for "
										+ pSD->getParent()->getName());
					//note that Cn needs to be resized, but resize on a valarray invalidates the content, and there is not erase on a valarray
					auto Cntmp = std::move(Cn);
					Cn.resize(k);
					for (unsigned int i(0); i < k; ++i) {
						Cn[i] = Cntmp[i];
					}
				}
			}
		}
	}

	//current root radius
	double rdz;
	rootDiameter->get(bt,rdz);rdz/=2.;
	if(rdz>2.*r0 || Ci<=Cmin || k<4){
		//secondary growth destroyed cortex, assume uptake is zero
		// or
		//competition reduced uptake to 0
		fluxDensity=0.;
		res=fluxDensity;
		pSD->preferedTimeStep()=pSD->maxTimeStep();
		lt+=dt;
		//the depletion zone will take some time to disappear
		if (depletionZone) depletionZone->set(lt,rdz);
		return;
	}

	//root radius and the distance of all finite element nodes from the center of the root node
	Array r(0.,k);
	r[0] = r0;//todo could also use rdz here, but it would mean a moving inner boundary pushing everything out.
	for (unsigned int i(1); i < k; ++i) {
		r[i] = r[i - 1] + dr;
	}

	//set initial concentrations
	Array Cn1(Cn);

	//Get the different parameters
	double Imax,Km,v0(0.00864),De;
	sImax->get(mt-refTImax,Imax);
	sKm->get(mt-refTKm,Km);
	if(sv0) {
		sv0->getRate(t,v0);// cm3 water/cm root/day
		//divide v0 with the assumed constant volumetric water content so amount of water taken up is expressed in cm3 soil volume. This because the concentration is expressed in cm3 soil volume
		//We need to do this to make cm3 equal not refer to amount of water but an amount of water per cubic cm3 of soil.
		v0/=r[0];//cm3/cm/day / cm -> cm/day
	}
	sDe->get(mt,depth,De);
	if(mode==1){
		//De_barber = Dsimunek *theta / b
		De*=theta/b;
	}


	const double Imaxh(Imax);
	if(Imax<1e-8 && !depletionZone){//otherwise the depletion zone slowely dissappears, and we should run the model.
		//no uptake
		fluxDensity=0.;
		res=fluxDensity;
		pSD->preferedTimeStep()*=1.1;
		if(pSD->preferedTimeStep()>pSD->maxTimeStep()) pSD->preferedTimeStep()=pSD->maxTimeStep();
		lt+=dt;
		if (depletionZone) depletionZone->set(lt,r0);
		return;
	}



	//constants
	const double De_b=De*b;
	const double D1=(dr*dr*2./De/dt)+2.;
	const double D2=(dr*dr*2./De/dt)-2.;
	const double S3=dr*2./(De_b);

	//v0 dependent constants
	const double A1((2*dr*r0*v0)/(De_b*r1)); //A1 is used for case II boundary condition
	const double S=(dr/2)*(1.+((v0*r0)/(De_b)));
	const Array S1((S/r)-1.);
	const Array S2(S1+2.);

	//roothairs
	//TODO Imax and Km etc could be different for roothairs
	const double  Kmh(Km), Cminh(Cmin), D3(square(dr)/(De_b));

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
				S4[i]=((Imaxh*rh0)/(De_b))*log(rh1[i]/(1.6487212707*rh0));
				const double Ch=calcCh(Cn[i],Kmh,Cminh,S4[i]);
				//const double dCh=calcdCh(Cn[i],Kmh,Cminh,S4[i]);
				Ih[i]=calcIh(Ch,Imaxh,Kmh,Cminh,Ah[i]);
				//dIh[i]=calcdIh(Ch,dCh,Imaxh,Kmh,Cminh,Ah[i]);
			}
		}
	}

	//build tridiagonal jacobian matrix
	Array JaB(D1,k),Jb(S1),Jc(S2*-1.);

	//boundary conditions
	if(competition){
		JaB[k-1]=D1+S2[k-1]*A1 ;// case II only
		Jb[k-1]=-2. ;// case II only
	}else{
		JaB[k-1]=1.;
		Jb[k-1]=0.;
	}
	Jc[0]=-2;

	//Hypothetical Concentrations for inner and outer boundary conditions
	const double Cbit(Cn[0]-Cmin);
	const double Cbi=Cn[1]-S3*((Imax*Cbit/(Km+Cbit))-(v0*Cn[0]));
	const double Co(competition ? Cn[k-2]-A1*Cn[k-1] : 0 );

	//solve iteratively for Ce is estimate of Cn1[0]
	int count(0);
	const int maxcount(50);
	double err(1.e9);

	while (err>1e-7 && count<maxcount) {

		//inner root concentrations
		const double Ce(Cn1[0]-Cmin);
		const double Cbi1=Cn1[1]-S3*( ((Imax*Ce)/(Km+Ce))-(v0*Cn1[0]));

		//f
		Array f(k);
		f[0]=-1.*(   (S1[0]*Cbi1)+(D1*Cn1[0])-(S2[0]*Cn1[1])+(S1[0]*Cbi)-(D2*Cn[0])-(S2[0]*Cn[1])   ); // checked!
		for(unsigned int i(1);i<k-1;++i )
		{
			f[i]=-1.*((S1[i]*Cn1[i-1])+(D1*Cn1[i])-(S2[i]*Cn1[i+1]) +(S1[i]*Cn[i-1])-(D2*Cn[i])-(S2[i]*Cn[i+1]));
		}
		f[k-1] = competition ?  (-1.*(-2.*Cn1[k-2] + (D1+S2[k-1]*A1)*Cn1[k-1] +  S1[k-1]*Cn[k-2] - D2*Cn[k-1] - S2[k-1]*Co)) : (-1.*(Cn1[k-1]-Ci)) ;

		//estimate for inner boundary condition
		Array Ja(JaB);
		Ja[0]= -1.*S1[0]*S3*(((-Imax*Ce)/(square(Km+Ce)))+(Imax/(Km+Ce))-v0)+D1;

		//roothairs
		if(Ah[0]){
        	for (unsigned int i(0); i < k && Ah[i]>1e-9 ; ++i){
                const double Ch1=calcCh(Cn1[i],Kmh,Cminh,S4[i]);
                const double dCh1=calcdCh(Cn1[i],Kmh,Cminh,S4[i]);
                const double Ih1=calcIh(Ch1,Imaxh,Kmh,Cminh,Ah[i]);
                const double dIh1=calcdIh(Ch1,dCh1,Imaxh,Kmh,Cminh,Ah[i]);
    			f[i]-=D3*(Ih1+Ih[i]);
        		Ja[i]+=D3*(dIh1);
        	}
        }

		//solve tridiagonal matrix
		tridiagonalSolver(Ja, Jb, Jc, f); // this solver manipulates the right hand side f, which is set again in the next step anyway
		/*Array x(k);
		thomasBoundaryCondition(Ja, Jb, Jc, f, x);
		f=std::move(k);*/

		//error
		const double perr=err;
		err=(abs(f)).max();

		//converging?
		if(err>perr){
			//not converging try a different estimate for Cn1
			Cn1+=Cn;
			Cn1/=2.;
			err=1e8;
			//msg::warning("BarberCushman: not converging");
		}else{
			Cn1+=f;
		}

		//eternal loop protection
		count++;
	}

	if (count >= maxcount && err>Cmin){
		msg::warning("BarberCushman: could not solve within 50 iterations, increase dr or reduce timestep");// Ci"+std::to_string(Ci)+" dr "+std::to_string(dr)+" dt "+std::to_string(dt));
		if(err>Km){
			msg::warning("BarberCushman: large precision error. Check timestep and or dr, setting uptake to 0 for this segment ");
			for(unsigned int i(0); i<k;++i){
		        Cn1[i]=Cmin;
			}
		}
		++msgcount;
	}

	//calculate flux into the root using second order
	{
		const double Ca1(Cn1[0] - Cmin);
		const double Ca(Cn[0] - Cmin);
		fluxDensity = circ * Imax * (Ca1 / (Km + Ca1) + Ca / (Km + Ca));
		if (Ah[0]) {
			//include uptake by hairs
			for (unsigned int i(0); i < k && Ah[i] > 0; ++i) {
				double d1 = calcCh(Cn1[i], Km, Cmin, S4[i]);
				double d0 = calcCh(Cn[i], Km, Cmin, S4[i]);
				const double d3 = Ah[i] * vol[i];
				d1 = calcIh(d1, Imaxh, Km, Cmin, d3);
				d0 = calcIh(d0, Imaxh, Km, Cmin, d3);
				fluxDensity += (d1 + d0);
			}
			fluxDensity /= 2.; //average flux density
		}
	}

	//convert to per cm to per root segment length
	if(rootLength){
		double l;
		rootLength->get(mt,l);
		fluxDensity*=l;
	}

	//do some checks
	if(std::isnan(fluxDensity))msg::error("BarberCushman: Results is NaN");
	if(fluxDensity<0.) {
		msg::warning("BarberCushman: getting negative values for uptake, returning 0");
		fluxDensity=0.;
	}

	//save result
	res=fluxDensity;

	//increase localtime
	lt+=dt;

	//check for concentrations below cmin
	for (std::size_t i(0); i < k; ++i) {
		if(Cn1[i]<Cmin)Cn1[i]=Cmin;//msg::error("BarberCushman: soil concentration became negative");
	}

	//check for increasing concentrations. This happens when dr is too large
	if(Cn1[0]>1.1*Ci)
		msg::warning("BarberCushman: concentrations at root surface are increasing. massflow>uptake? or dr too large?");

	//prepare for next loop
	Cn = std::move(Cn1);//allow move

	//find edge of depletion zone
	if(depletionZone){
		std::size_t edgeDepletionZone(0);
		for (int i(k-1); i > -1; --i) {
			if (Cn[i]< 0.95*Ci){
				edgeDepletionZone=i;
				break;
			}
		}
		if(edgeDepletionZone==k-1){ //edge is at the boundary
			msg::warning("BarberCushman: competition effective. (edge depletion == r1)", 2);
		}
		rdz+=r[edgeDepletionZone]-r[0];
		depletionZone->set(t,rdz);
	}

	//allow time step to change slowly.
	if(lt-pSD->getStartTime()>pSD->maxTimeStep()*2){
		if(count<6 && pSD->preferedTimeStep()*increaseTimeStep<pSD->maxTimeStep()) pSD->preferedTimeStep()*=increaseTimeStep;
		if(count>12 && pSD->preferedTimeStep()*0.8>pSD->minTimeStep()) pSD->preferedTimeStep()*=0.80;
	}

	//we will not recalculate nutrient uptake, even if it is based on predictions.
	pSD->clearPredictions(nmbest);

	//write row into concentration matrix
	if ( os.is_open() ){
		os<<std::endl<<t;
		for(auto Cn_i: Cn){
			os<<"\t"<<Cn_i;
		}
	}
	return;

}
double Barber_cushman_1981_nutrient_uptake::calcCh(const double &C,const double &Km,const double &Cmin,const double &S4){
    return ( ( (   (C-Km+Cmin-S4)+sqrt(  square(-C+Km-Cmin+S4)-4*C*Cmin+4*S4*Cmin+4*C*Km) )/2) - Cmin);
}
double Barber_cushman_1981_nutrient_uptake::calcdCh(const double &C,const double &Km,const double &Cmin,const double &S4){
   return ( 0.5 + 0.5*(C + Km - Cmin - S4)/sqrt( square(-C+Km-Cmin+S4)-4*C*Cmin+4*S4*Cmin+4*C*Km )   );
}

double Barber_cushman_1981_nutrient_uptake::calcIh(const double &Ch,const double &Imax,const double &Km,const double &Cmin,const double &Ah){
    return  ((Ah*Imax*Ch)/(Km+Ch));
}
double Barber_cushman_1981_nutrient_uptake::calcdIh(const double &Ch,const double &dCh,const double &Imax,const double &Km,const double &Cmin,const double &Ah){
   return ( Ah*Imax*( (dCh/(Km+Ch)) - (Ch*dCh/(square(Km+Ch))) ) );
}


std::string Barber_cushman_1981_nutrient_uptake::getName()const{
	return "barber_cushman_1981_nutrient_uptake";
}

DerivativeBase * newInstantiationBarber_cushman_1981_nutrient_uptake(SimulaDynamic* const pSD) {
	return new Barber_cushman_1981_nutrient_uptake(pSD);
}

std::string BiologicalNitrogenFixation::getName()const{
	return "bnf.V1";
}

BiologicalNitrogenFixation::BiologicalNitrogenFixation(SimulaDynamic* const pSV):DerivativeBase(pSV){
	//std::string nutrient(pSD->getParent()->getName());
	optimal=pSD->getParent(1)->getChild("plantOptimalNutrientContent",pSD->getUnit());
	//plant parameters
	std::string plantType;
	pSD->getParent(2)->getChild("plantType")->get(plantType);
	SimulaBase* p=GETPLANTPARAMETERS(plantType);
	//ratio from parameter setting
	p=p->getChild("resources")->existingChild("relativeRelianceOnBNF");
	if(p){
		p->get(ratio);
	}else{
		ratio=0;//no biological N fixation is the default
	}
}
void BiologicalNitrogenFixation::calculate(const Time &t, double& bnf){
	//note that this is very simple - and not mechanistic at all. But it will let BNF float between ratio and about 2x ratio
	Time to=1; //first 5 days no fixation according to cropgro, but it's neglicable. Set to 1 day, so we do not get warnings about forward differentiation.
	if(t>pSD->getStartTime()+to){
		optimal->getRate(t,bnf);
	}else{
		bnf=0;
	}
	bnf*=ratio;
}
DerivativeBase * newInstantiationBiologicalNitrogenFixation(SimulaDynamic* const pSV) {
	return new BiologicalNitrogenFixation(pSV);
}

//class Imax scales Imax according to RCS
std::string Imax::getName()const{
	return "kineticParameters";
}

Imax::Imax(SimulaDynamic* const pSV):DerivativeBase(pSV),
		imax(nullptr),
		rcs(pSV->getParent(2)->existingChild("rootCorticalSenescenceStage")),
		rcseffect(nullptr){

	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//root type
	std::string rootType;
	pSD->getParent(4)->getChild("rootType")->get(rootType);
	//plant parameters
	SimulaBase *param(GETROOTPARAMETERS(plantType,rootType));
	//nutrient
	std::string nutrient(pSD->getParent()->getName());
	param=param->getChild(nutrient);
	imax=param->getChild(pSD->getName()); //Imax or Km
	if(rcs) rcseffect=param->getChild("reductionInImaxDueToCorticalSenescence");

}

void Imax::calculate(const Time &t, double& param){
	imax->get(t-pSD->getStartTime(),param);
	double c,e;
	if(rcs) {
		rcs->get(t,c);
		rcseffect->get(c,e);
		param*=(1-e);
	}
}

DerivativeBase * newInstantiationImax(SimulaDynamic* const p) {
	return new Imax(p);
}

//class integrates fluxdensity over the length of a root segment thus calculating nutrient uptake by a segment
/*SpatialIntegral::SpatialIntegral(SimulaDynamic* const pSV) :
	TotalBaseLabeled(pSV) {
	//check if unit given in input file agrees with this function
	pSD->getUnit().check("umol", "this", "SpatialIntegral");
	//length
	length=pSD->getParent(2)->getChild("rootSegmentLength");
	//current and next datapoint
	current = pSD->getParent(2)->getChild(pSD->getParent()->getName())->getChild("nutrientFluxDensity");
	//next1 is growthpoint
	next1 = pSD->getParent(4)->getChild("growthpoint")->getChild(pSD->getParent()->getName())->getChild("nutrientFluxDensity");
	//next2 is next datapoint
	next2 = nullptr;
	//initiate the chain
	chain1=pSD->getParent(4)->getChild("growthpoint")->getChild(pSD->getParent()->getName())->getChild(pSD->getName());
	chain2=nullptr;
}

void SpatialIntegral::calculate(const Time &t, double& flux) {
	//This function integrated flux over root length between two nodes.

	//check based on time what the next point is: growthpoint of datapoint
	SimulaBase * next(getNextRootNode(t)); //watch out, this is the next object with the same name

	//get positions of datapoints
	double l;
	length->get(t,l);

	//get flux density at these datapoints
	//TODO with some metadata, possibly in the name of pSV, we could use this function for more than one nutrient. Same metadata should return in the name of nutrientInFlux
	double fluxD0(0);
	current->getRate(t, fluxD0);
	double fluxD1(0);
	next->getRate(t, fluxD1);

	//euler integration of the length of the root segment
	flux =  l * (fluxD0 + fluxD1) / 2;
}
DerivativeBase * newInstantiationSpatialIntegral(SimulaDynamic* const pSV) {
	return new SpatialIntegral(pSV);
}
std::string SpatialIntegral::getName()const{
	return "spatialIntegral";
}
*/
SegmentMaxNutrientUptakeRate::SegmentMaxNutrientUptakeRate(SimulaDynamic* pSD) :
	TotalBaseLabeled(pSD) {
	//surface area simulator
	surfaceArea = pSD->getParent(2)->getChild("rootSegmentSurfaceArea");
	///@todo surface area should have unit that corresponds with Imax
}

void SegmentMaxNutrientUptakeRate::calculate(const Time &t, double &var) {
	//check based on time what the next point is: growthpoint of datapoint
	SimulaBase * next=getNext(t);

	//average Imax
	double v1, v2;
	//TODO this could be little faster when update=false
	pSD->getSibling("Imax")->get(t, v1);
	next->getSibling("Imax")->get(t, v2);
	var = (v1 + v2) / 2;
	//multiply with segment surface area
    msg::warning("SegmentMaxNutrientUptakeRate: multiplication with root surface area missing, code not up to date?");
}

std::string SegmentMaxNutrientUptakeRate::getName()const{
	return "segmentMaxNutrientUptakeRate";
}

DerivativeBase * newInstantiationSegmentMaxNutrientUptakeRate(SimulaDynamic* const pSD) {
	return new SegmentMaxNutrientUptakeRate(pSD);
}


//class sums up the nutrient uptake for the whole root system (root plus branches)
RadiusDepletionZoneSimRoot4::RadiusDepletionZoneSimRoot4(SimulaDynamic* pSD) :
	DerivativeBase(pSD) {
	Time t=pSD->getStartTime();
	//check own unit
	pSD->getUnit().check("cm", "this", "RadiusDepletionZoneSimroot");
	//root diameter
	rootDiameterSimulator = pSD->getParent()->getSibling("rootDiameter");
	rootDiameterSimulator->getUnit().check("cm", "rootDiameter", "RadiusDepletionZoneSimroot");
	//depth
	Coordinate position;
	pSD->getParent()->getParent()->getAbsolute(pSD->getStartTime(), position);

	//nutrient
	std::string nutrient(pSD->getParent()->getName());
	//diffusion coefficient
	Coordinate depth;
	pSD->getAbsolute(pSD->getStartTime(), depth);
	ORIGIN->getChild("environment")->getChild("soil")->getChild(nutrient)->getChild(
			"diffusionCoefficient","cm2/day")->get(t,depth, diffusionCoefficient);
}

void RadiusDepletionZoneSimRoot4::calculate(const Time &t, double &radius) {
	//current root diameter
	double d;
	rootDiameterSimulator->get(t, d);
	//root age
	Time rootAge(t - pSD->getStartTime());
	//radius
	radius = (d / 2) + sqrt(diffusionCoefficient * rootAge) * 2;
}

std::string RadiusDepletionZoneSimRoot4::getName()const{
	return "radiusDepletionZoneSimRoot4";
}

DerivativeBase * newInstantiationRadiusDepletionZoneSimRoot4(SimulaDynamic* const pSD) {
	return new RadiusDepletionZoneSimRoot4(pSD);
}



//Calls BarberCushman model for the radius of the depletionzone, determined by cut of of 95%(?) in concentration change
RadiusDepletionZoneBarberCushman::RadiusDepletionZoneBarberCushman(SimulaDynamic* pSD) :
	DerivativeBase(pSD) {
	//check own unit
	pSD->getUnit().check("cm", "this", "RadiusDepletionZoneSimroot");
	//root diameter
	link = pSD->existingSibling("nutrientFluxDensity");
	if(!link) link = pSD->existingSibling("rootSegmentNutrientUptakeBC");
	if(!link) link = pSD->getSibling("rootSegmentNutrientUptake");
}
void RadiusDepletionZoneBarberCushman::calculate(const Time &t, double &radius) {
	//update
	link->get(t,radius);
}
std::string RadiusDepletionZoneBarberCushman::getName()const{
	return "radiusDepletionZoneBarberCushman";
}
DerivativeBase * newInstantiationRadiusDepletionZoneBarberCushman(SimulaDynamic* const pSD) {
	return new RadiusDepletionZoneBarberCushman(pSD);
}



//class sums up the nutrient uptake for the whole root system (root plus branches)
RootSegmentNutrientDepletionVolume::RootSegmentNutrientDepletionVolume(SimulaDynamic* pSD) :
		TotalBaseLabeled(pSD),volSim(nullptr),lengthSim(nullptr),current(nullptr),next1(nullptr),next2(nullptr) {
	//check own unit
	pSD->getUnit().check("cm3", "this", "RootSegmentNutrientDepletionVolume");
	lengthSim=pSD->getParent(2)->getChild("rootSegmentLength");
	volSim=pSD->getParent(2)->getChild("rootSegmentVolume");
	current=pSD->getSibling("radiusDepletionZone");
	next1=getNext(pSD->getStartTime())->getSibling("radiusDepletionZone");
}

//TODO this can be more efficient - look at optimal nutrient content
void RootSegmentNutrientDepletionVolume::calculate(const Time &t, double &volume) {
	//check based on time what the next point is: growthpoint of datapoint
	SimulaBase * next(next1);
	if(!next2){
			next=getNext(t)->getSibling("radiusDepletionZone");
			if (next!=next1) next2=next;
	}else if(t>next2->getStartTime()){
		next=next2;
	}

	//get the radius at these datapoints
	double r0(0);
	current->get(t, r0);
	double r1(0);
	next->getSibling("radiusDepletionZone")->get(t, r1);

	//calculate surface area
	r0 = surfaceAreaCircle(r0);
	r1 = surfaceAreaCircle(r1);

	//double distance between these datapoints
	double length;
	lengthSim->get(t, length);

	//current root volume
	double rootVolume;
	volSim->get(t, rootVolume);

	//volume
	volume = (length * (r0 + r1) / 2) - rootVolume;
}

std::string RootSegmentNutrientDepletionVolume::getName()const{
	return "rootSegmentNutrientDepletionVolume";
}

DerivativeBase * newInstantiationRootSegmentNutrientDepletionVolume(SimulaDynamic* const pSD) {
	return new RootSegmentNutrientDepletionVolume(pSD);
}


//Register the module
class AutoRegisterNutrientClassesInstantiationFunctions {
public:
	AutoRegisterNutrientClassesInstantiationFunctions() {
		//BaseClassesMap::getDerivativeBaseClasses()["spatialIntegral"] = newInstantiationSpatialIntegral;
		BaseClassesMap::getDerivativeBaseClasses()["barber_cushman_1981_nutrient_uptake"]	= newInstantiationBarber_cushman_1981_nutrient_uptake;
		BaseClassesMap::getDerivativeBaseClasses()["bnf.V1"] = newInstantiationBiologicalNitrogenFixation;
		BaseClassesMap::getDerivativeBaseClasses()["kineticParameters"] = newInstantiationImax;
		BaseClassesMap::getDerivativeBaseClasses()["segmentMaxNutrientUptakeRate"] = newInstantiationSegmentMaxNutrientUptakeRate;
		BaseClassesMap::getDerivativeBaseClasses()["radiusDepletionZoneSimRoot4"] = newInstantiationRadiusDepletionZoneSimRoot4;
		BaseClassesMap::getDerivativeBaseClasses()["radiusDepletionZoneBarberCushman"] = newInstantiationRadiusDepletionZoneBarberCushman;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentNutrientDepletionVolume"]= newInstantiationRootSegmentNutrientDepletionVolume;
	}

};

// our one instance of the proxy
static AutoRegisterNutrientClassesInstantiationFunctions p;

