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

#include "CarbonCosts.hpp"
#include "../PlantType.hpp"
#include "../../math/InterpolationLibrary.hpp"
#include "../../tools/StringExtensions.hpp"

std::string CarbonCostOfExudates::getName()const{
	return ("carbonCostOfExudates");
}

CarbonCostOfExudates::CarbonCostOfExudates(SimulaDynamic* pSD): TotalBase(pSD){
	//check unit
	if(pSD->getUnit()!="g" && pSD->getUnit()!="g/day") msg::error("CarbonCostOfExudates: unit should be in g or g/day");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type parameters
	std::string rootType;
	pSD->getParent(3)->getChild("rootType")->get(rootType);
	SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
	//find simulators in database
	param=parameters->existingChild("relativeCarbonCostOfExudation");
	if(param){
		if(param->getUnit()=="g/g/day"){
			size=pSD->getSibling("rootSegmentDryWeight","g");
		}else if(param->getUnit()=="g/cm/day"){
			size=pSD->getSibling("rootSegmentLength","cm");
		}else if(param->getUnit()=="g/cm2/day"){
			size=pSD->getSibling("rootSegmentSurfaceArea","cm2");
		}else{
			msg::error("CarbonCostOfExudates: unsupported unit for parameter relativeCarbonCostOfExudation. Use: g/g/day or g/cm/day or g/cm2/day");
		}
	}else{
		msg::warning("CarbonCostOfExudates: parameter relativeCarbonCostOfExudation missing in root parameter section. No exudates simulated.");
	}
	//find nutrient specific multiplier
	multiplier=pSD;
	PLANTTOP(multiplier);
	multiplier=multiplier->existingChild("stressFactor:impactOn:rootSegmentCarbonCostOfExudates");
}
void CarbonCostOfExudates::calculate(const Time &t,double &var){
	if(!param){
		var=0;
		return;
	}
	//size
	size->get(t,var);
	//relative param based on age of rootsegment
	double m;
	param->get(t - pSD->getStartTime(),m);
	var*=m;
	//adustment (due to for example nutrient status of plant)
	if(multiplier){
		multiplier->get(t,m);
		var*=m;
	}
}

DerivativeBase * newInstantiationCarbonCostOfExudates(SimulaDynamic* const pSD){
   return new CarbonCostOfExudates(pSD);
}

//====
std::string CarbonCostOfBiologicalNitrogenFixation::getName()const{
	return ("carbonCostOfBiologicalNitrogenFixation");
}
CarbonCostOfBiologicalNitrogenFixation::CarbonCostOfBiologicalNitrogenFixation(SimulaDynamic* pSD): DerivativeBase(pSD){
	//nutrient uptake
	fixation=pSD->existingSibling("nitrate");
	if(fixation){
		fixation=fixation->existingChild("plantNutrientFixation");
	}
	//plant parameters
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	SimulaBase* p=GETPLANTPARAMETERS(plantType);
	//cost ratio from parameter setting
	factor=p->getChild("resources")->existingChild("carbonCostOfBiologicalNitrogenFixation");
	if(!factor)factor=p->getChild("resources")->existingChild("carbonCostOfBiologcialNitrogenFixation"); //backward compatibility which contained the spelling error

	if(!fixation && factor){
		msg::warning("CarbonCostOfBiologicalNitrogenFixation: cost not simulated for nitrogen fixation is not simulated");
		factor=nullptr;
	}
}
void CarbonCostOfBiologicalNitrogenFixation::calculate(const Time &t,double &cost){
	if(factor){
		///@todo these costs are not likely to be linear, but probably increase has N availability decreases? We could implement that with a simple stress multiplier.
		double f;
		fixation->getRate(t,cost);
		factor->get(t,f);
		cost*=f;
	}else{
		cost=0;
	}
}

DerivativeBase * newInstantiationCarbonCostOfBiologicalNitrogenFixation(SimulaDynamic* const pSD){
   return new CarbonCostOfBiologicalNitrogenFixation(pSD);
}

//
std::string CarbonCostOfNutrientUptake::getName()const{
	return ("carbonCostOfNutrientUptake");
}
CarbonCostOfNutrientUptake::CarbonCostOfNutrientUptake(SimulaDynamic* pSD): DerivativeBase(pSD){
	//nutrient uptake
	uptake=pSD->existingSibling("nitrate");
	if(uptake){
		SimulaBase *n=uptake;
		uptake=n->existingChild("plantNutrientUptake");
		max=n->existingChild("plantOptimalNutrientContent");
	}else{
		max=nullptr;
	}
	if(max) {
		//plant parameters
		std::string plantType;
		pSD->getSibling("plantType")->get(plantType);
		SimulaBase* p=GETPLANTPARAMETERS(plantType);
		//cost ratio from parameter setting
		factor=p->getChild("resources")->getChild("carbonCostOfNitrateUptake",pSD->getUnit()/max->getUnit());
	}else{
		msg::warning("CarbonCostOfNutrientUptake: cost not simulated for nitrate uptake is not simulated");
	}
}
void CarbonCostOfNutrientUptake::calculate(const Time &t,double &cost){
	if(max){
		///@todo these costs are not likely to be linear, but probably increase as N availability decreases? We could implement that with a simple stress multiplier.
		///@todo high N avail. does currently not feed back towards less N uptake. So costs can be too large causing plant to crash. For now, we set a max to the cost
		double f;
		if(uptake){
			uptake->getRate(t,cost);
			max->getRate(t,f);
			if(f<cost)cost=f;
		}else{
			max->getRate(t,cost);
		}
		factor->get(t,f);
		cost*=f;
	}else{
		cost=0;
	}
}

DerivativeBase * newInstantiationCarbonCostOfNutrientUptake(SimulaDynamic* const pSD){
   return new CarbonCostOfNutrientUptake(pSD);
}

//====
std::string SumCarbonCosts::getName()const{
	return ("sum");
}
SumCarbonCosts::SumCarbonCosts(SimulaDynamic* pSD): DerivativeBase(pSD){
	//path of things to sum up
	std::string paths;
	pSD->getChild("paths")->get(paths);
	std::string::size_type pos(0);
	while (pos<paths.size()){
		std::string path(nextWord(paths,pos,';'));
		SimulaBase* p=pSD->getParent()->existingPath(path);
		if(p){
			list.push_back(p);
		}else{
			msg::warning("SumCarbonCosts: Did not find "+path+". And thus not including it in the costs.");
		}
	}
	//rates
	if(pSD->getType()=="SimulaVariable"){
		rates=true;
	}else{
		rates=false;
	}
}
void SumCarbonCosts::calculate(const Time &t,double &tot){
	tot=0;
	double add;
	if(rates){
		for(std::vector<SimulaBase*>::iterator it(list.begin());it!=list.end();++it){
			(*it)->getRate(t,add);
			tot+=add;
		}
	}else{
		for(std::vector<SimulaBase*>::iterator it(list.begin());it!=list.end();++it){
			(*it)->get(t,add);
			tot+=add;
		}
	}
}

DerivativeBase * newInstantiationSumCarbonCosts(SimulaDynamic* const pSD){
   return new SumCarbonCosts(pSD);
}


//Register the module
class AutoRegisterExudatesInstantiationFunctions {
public:
  AutoRegisterExudatesInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["carbonCostOfExudates"] = newInstantiationCarbonCostOfExudates;
		BaseClassesMap::getDerivativeBaseClasses()["carbonCostOfBiologicalNitrogenFixation"] = newInstantiationCarbonCostOfBiologicalNitrogenFixation;
		BaseClassesMap::getDerivativeBaseClasses()["carbonCostOfNutrientUptake"] = newInstantiationCarbonCostOfNutrientUptake;
		BaseClassesMap::getDerivativeBaseClasses()["sum"] = newInstantiationSumCarbonCosts;
 }
};


// our one instance of the proxy
static AutoRegisterExudatesInstantiationFunctions p8734897ch398cn7;


