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
#include "SeedReserves.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include <math.h>
//#include "./Constants.hpp"
//#include "../../DataDefinitions/Units.hpp"



Reserves::Reserves(SimulaDynamic* const pSV):
	DerivativeBase(pSV)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plantingTime
	plantingTime=pSD->getStartTime();
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//get rate from table / constant
	rrSimulator=GETRESOURCEPARAMETERS(plantType)->getChild("reserveAllocationRate");
	//get seed size
	double seedsize;
	GETRESOURCEPARAMETERS(plantType)->getChild("seedSize","g")->get(seedsize);
	//seedsize*=CinDRYWEIGHT;
	pSD->setInitialValue(seedsize);
}
void Reserves::calculate(const Time &t, double& reserve){
	//current reserve
	pSD->get(t,reserve);
	if(reserve==0.) {
		pSD->minTimeStep()= pSD->maxTimeStep();
	}else if (reserve<0.00001) {//drop to zero
		pSD->preferedTimeStep()=pSD->maxTimeStep();
		reserve/=pSD->maxTimeStep();
	}else{ //calculate
		double rate;
		rrSimulator->get((Time)(t-plantingTime),rate);
		reserve*= rate*-1;
	}
	if(std::isnan(reserve)) {
		msg::error("Reserves: Reserves is NaN");
	}

}

std::string Reserves::getName()const{
	return "reserves";
}

DerivativeBase * newInstantiationReserves(SimulaDynamic* const pSD){
   return new Reserves(pSD);
}


ReservesSinkBased::ReservesSinkBased(SimulaDynamic* const pSV):
	DerivativeBase(pSV),mr(-1)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//get rate from table / constant
	sink=pSD->getSibling("plantPotentialCarbonSinkForGrowth","g");
	respiration=pSD->existingSibling("plantRespiration","g");
	photosynthesis=pSD->getSibling("plantPosition")->getChild("shoot")->getChild("photosynthesis","g");
	exudates=pSD->existingSibling("rootCarbonCosts","g");
	if(!exudates) exudates=pSD->existingSibling("rootCarbonCostOfExudates","g");
	//make sure we do not do timesteps larger than 1 day
	pSD->maxTimeStep()=1;
	//get seed size
	double seedsize;
	SimulaBase* param(GETRESOURCEPARAMETERS(plantType));
	param->getChild("seedSize","g")->get(seedsize);
	pSD->setInitialValue(seedsize);
	//time after which seed reserves have expired
	mt=12;
	SimulaBase *p(param->existingChild("seedReserveDuration","day"));
	if(p) p->get(mt);
	//carbon conversion factor.
	CinDryWeight=pSD->getSibling("carbonToDryWeightRatio","100%");
}
void ReservesSinkBased::calculate(const Time &t, double& reserve){
	if(!sink){
		reserve=0;
		return;
	}
	if(t-pSD->getStartTime()>mt){
		pSD->get(t,reserve);
		///@todo hard coded value here. this could be relative to initial reserves, but not if garbage collector was on
		if(reserve>0.05*mr) msg::warning("ReservesSinkBased: Expiring seed reserves at "+std::to_string(mt)+" d.a.g. with "+std::to_string(100*reserve/mr)+"% of the initial "+std::to_string(mr)+"g left.");
		reserve=0;
		sink=nullptr;
	}else{
		//max reserve
		pSD->get(t,reserve);
		if(mr<reserve) mr=reserve;
		//std::cout<<"seed re "<<t<<'t'<<reserve<<'\t';
		if(reserve<=1e-7){
			if(reserve<-1e-3)msg::error("ReservesSinkBased: Seed reserves got smaller than zero");
			//small error:fix it
			reserve=0;
			sink=nullptr;
			return;
		}
		if (reserve<1e-5) {//half time of a day
			reserve/=-1;
		}else{ //calculate
			double s,p;
			sink->getRate(t,s);
			photosynthesis->getRate(t,p);
			s-=p;
			if(respiration) respiration->getRate(t,p);
			s+=p;
			if(exudates) exudates->getRate(t,p);
			s+=p;
			//C in dry weight
			double cdw;
			CinDryWeight->get(t,cdw);
			//convert carbon to dryweight
			s/=cdw;
			if(s<0){
				//carbon not needed
				reserve=0;
			}else{
				//s is allocation rate - choose sensible timestep
				if(s>reserve){
					//we can not meet demand for one day fall back to zero but do it slowly to avoid abrupt changes in carbon availability
					reserve/=-1;
				}else{
					//there is enough carbon, supply what is needed
					reserve= s*-1;
				}
			}
		}
	}
}

std::string ReservesSinkBased::getName()const{
	return "reservesSinkBased";
}

DerivativeBase * newInstantiationReservesSinkBased(SimulaDynamic* const pSD){
   return new ReservesSinkBased(pSD);
}

//Scaling factor for root growth (0-1). All roots are scaled equally
CarbonReserves::CarbonReserves(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//potential growth
	potentialGrowth=pSD->getSibling("plantPotentialCarbonSinkForGrowth","g");
	//carbon from photosynthesis and seed reserves
	available=pSD->getSibling("plantCarbonIncome","g");
	//respiration & exudates
	respiration=pSD->existingSibling("plantRespiration","g");
	exudates=pSD->existingSibling("rootCarbonCosts","g");
	if(!exudates) exudates=pSD->existingSibling("rootCarbonCostOfExudates","g");
	//sugar regulation of growth.

}
void CarbonReserves::calculate(const Time &t,double &v){
	//available carbon from other sources
	double v1;
	available->getRate(t,v1);
	v=v1;
	//substract what is needed for respiration, exudates and potential growth
	double s,s1,s2;
	potentialGrowth->getRate(t,s);
	v-=s;
	if(respiration) {
		respiration->getRate(t,s1);
		v-=s1;
	}
	if(exudates){
		exudates->getRate(t,s2);
		v-=s2;
	}
	//limit rate
	if(v<0){
		double s,l;
		pSD->get(t,s);
		l=-0.5;//do not deplete more than 70 percent per day ! note that sugars are probably growth regulators, scaling back growth when their levels go down.
		if(v<s*l){//comparing negative numbers here
			v=s*l;
		}
	}
	if(std::isnan(v)) {
		msg::error("CarbonReserves: Reserve is NaN");
	}

}


std::string CarbonReserves::getName()const{
	return "carbonReserves";
}

DerivativeBase * newInstantiationCarbonReserves(SimulaDynamic* const pSD){
   return new CarbonReserves(pSD);
}






//==================registration of the classes=================
class AutoRegisterSeedReservesInstantiationFunctions {
public:
   AutoRegisterSeedReservesInstantiationFunctions() {
	  BaseClassesMap::getDerivativeBaseClasses()["reserves"] = newInstantiationReserves;
	  BaseClassesMap::getDerivativeBaseClasses()["reservesSinkBased"] = newInstantiationReservesSinkBased;
	  BaseClassesMap::getDerivativeBaseClasses()["carbonReserves"] = newInstantiationCarbonReserves;
   }
};

// our one instance of the proxy
static AutoRegisterSeedReservesInstantiationFunctions p;
