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
#include "ShootDryWeight.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"


LeafDryWeight::LeafDryWeight(SimulaDynamic* const pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//planting time
	pSD->getParent(2)->getSibling("plantingTime")->get(plantingTime);
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD)
	//SLA
	SLASimulator=ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot")->getChild("specificLeafArea","g/cm2");
	if(SLASimulator->getType()!="SimulaConstant<double>") msg::error("LeafDryWeight: this assumes a constant SLA use leafDryWeight.v2 with SimulaVariable when sla is not constant.");
	//leaf area
	leafAreaSimulator=pSD->getSibling("leafArea","cm2");
}
void LeafDryWeight::calculate(const Time &t, double &dw){
	//note that the original version starts with an exponential growth curve instead of using photosynthesis
	//local time
	Time localTime(t - plantingTime);
	//get specific leaf area SLA
	double SLA;
	SLASimulator->get(localTime,SLA);
	//get portion of shoot allocated carbon that is used for leafs.
	double la;
	leafAreaSimulator->get(t,la);
	//multiply and return the result
	dw=la*SLA;
}
std::string LeafDryWeight::getName()const{
	return "leafDryWeight";
}
DerivativeBase * newInstantiationLeafDryWeight(SimulaDynamic* const pSD){
   return new LeafDryWeight(pSD);
}


LeafDryWeight2::LeafDryWeight2(SimulaDynamic* const pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//planting time
	pSD->getParent(3)->getChild("plantingTime")->get(plantingTime);
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD)
	//SLA
	SLASimulator=ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot")->getChild("specificLeafArea","g/cm2");
	//leaf area
	leafAreaSimulator=pSD->getSibling("leafArea","cm2");
}
void LeafDryWeight2::calculate(const Time &t, double &dw){
	//note that the original version starts with an exponential growth curve instead of using photosynthesis
	//local time
	Time localTime(t - plantingTime);
	//get specific leaf area SLA
	double SLA;
	SLASimulator->get(localTime,SLA);
	//get portion of shoot allocated carbon that is used for leafs.
	double la;
	leafAreaSimulator->getRate(t,la);
	//multiply and return the result
	dw=la*SLA;
}
std::string LeafDryWeight2::getName()const{
	return "leafDryWeight.v2";
}
DerivativeBase * newInstantiationLeafDryWeight2(SimulaDynamic* const pSD){
   return new LeafDryWeight2(pSD);
}

StemDryWeight::StemDryWeight(SimulaDynamic* const pSV):
	DerivativeBase(pSV),c2sSimulator(nullptr), CinDryWeight(nullptr), leafAllocSimulator(nullptr), leafDW(nullptr), stemDW(nullptr)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plant parameters
	c2sSimulator=pSD->existingSibling("carbonAllocation2Stems","g");
	//carbon conversion factor.
	CinDryWeight=pSD->getParent(3)->existingChild("carbonToDryWeightRatio","100%");
	//fallback in case carbon module does not run.
	if(!c2sSimulator){
		//plant type
		std::string plantType;
		pSD->getParent(3)->getChild("plantType")->get(plantType);
		//plant parameters
		SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
		stemDW=parameters->getChild("shoot")->existingChild("stemDryWeightAccumulation","g/day");

		if(!stemDW){
			//carbon allocation simulator (1-root)
			SimulaBase * p=parameters->existingChild("resources");
			if(p)
				leafAllocSimulator=p->existingChild("carbonAllocation2LeafsFactor","100%");
			if(leafAllocSimulator){
				leafDW=pSD->getSibling("leafDryWeight","g");
			}else{
				msg::warning("StemDryWeight: have no basis to simulate the stem dryweight, setting this to 0.");
			}
		}

	}
}
void StemDryWeight::calculate(const Time &t, double& cal){
	if(c2sSimulator){//carbon module is running
		//get portion of stem allocated carbon
		c2sSimulator->getRate(t,cal);
		//C in dry weight
		double cdw;
		if(CinDryWeight){ ///todo instead of existing, we could create a database::get("param",number) where is it is missing number will be inserted as a simulaconstant.
			CinDryWeight->get(t,cdw);
		}else{
			cdw=0.45;//backwards compatible default
		}
		cal/=cdw;
	}else{//no carbon module so no stems currently simulated.
		if(stemDW){
			stemDW->get(t-pSD->getStartTime(),cal);
		}else if(leafDW){
			//backwards compatible fall back.
			leafDW->getRate(t,cal);
			double r;
			leafAllocSimulator->get(t-pSD->getStartTime(),r);
			//cal/=r;//to get totalrate
			//cal*=(1.-r);//to get
			cal*=(1./r)-1.;
		}else{
			cal=0;
		}
	}
}


std::string StemDryWeight::getName()const{
	return "stemDryWeight";
}
DerivativeBase * newInstantiationStemDryWeight(SimulaDynamic* const pSV){
   return new StemDryWeight(pSV);
}


ShootDryWeight::ShootDryWeight(SimulaDynamic* const pSV):
	DerivativeBase(pSV)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plant parameters
	c2sSimulator=pSD->getParent(3)->getChild("carbonAllocation2ShootRate","g/day");
	//carbon conversion factor.
	CinDryWeight=pSD->getParent(3)->getChild("carbonToDryWeightRatio","100%");
}
void ShootDryWeight::calculate(const Time &t, double& cal){
	//get portion of shoot allocated carbon
	c2sSimulator->get(t,cal);
	//C in dry weight
	double cdw;
	CinDryWeight->get(t,cdw);
	//multiply and return the result - assume that carbon to drymatter ratio is 0.54
	cal/=cdw;
}

std::string ShootDryWeight::getName()const{
	return "shootDryWeight";
}

DerivativeBase * newInstantiationShootDryWeight(SimulaDynamic* const pSD){
   return new ShootDryWeight(pSD);
}






//==================registration of the classes=================
class AutoRegisterRateClassInstantiationFunctionsDryWeight {
public:
   AutoRegisterRateClassInstantiationFunctionsDryWeight() {
 	  BaseClassesMap::getDerivativeBaseClasses()["leafDryWeight"] = newInstantiationLeafDryWeight;
 	  BaseClassesMap::getDerivativeBaseClasses()["leafDryWeight.v2"] = newInstantiationLeafDryWeight2;
	  BaseClassesMap::getDerivativeBaseClasses()["shootDryWeight"] = newInstantiationShootDryWeight;
	  BaseClassesMap::getDerivativeBaseClasses()["stemDryWeight"] = newInstantiationStemDryWeight;
   };
};



// our one instance of the proxy
static AutoRegisterRateClassInstantiationFunctionsDryWeight p465461065;


