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
#include  "TotalsForAllPlants.hpp"
#include  "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include "../../engine/SimulaVariable.hpp"
#include "../../cli/Messages.hpp"


SumOverPlantsShoot::SumOverPlantsShoot(SimulaDynamic* pSD):DerivativeBase(pSD), mean(false)
{
	//name we are looking for
	SimulaBase *p=pSD->existingChild("name");
	if(p){
		p->get(name);
	}else{
		name=pSD->getName();
	};
	if(name.substr(0,4)=="mean"){
		name.erase(0,4);
		name[0]=tolower(name[0]);
		mean=true;
	}
	rates=false;
	SimulaVariable* v(dynamic_cast<SimulaVariable* >(pSD));
	if(v){
		rates=true;
	}else{
		rates=false;
	}

}

void SumOverPlantsShoot::calculate(const Time &t,double &total){

	//loop over plants
	total=0;

	//find all plants //this is assuming plants are planted immediately, but possibly with different dates.
	SimulaBase::List	plants;
	pSD->getParent()->getAllChildren(plants,t);

	//get leaf area

	//sum up leaf area of all plants
	double count=0.;
	for(auto & it:plants){
		SimulaBase*	k=(it)->existingChild("plantingTime");
		if(k) {
			Time pt;
			k->get(pt);
			if(t>=pt){
				k=(it)->getChild("plantPosition",t)->getChild("shoot",t)->getChild(name,t);
				if(k->getUnit()!=pSD->getUnit()) msg::error("SumOverPlantShoot: Unit checking failed for \'"+pSD->getName()+"\' with unit "+pSD->getUnit().name+" while plant \'"+(it)->getName()+"\' has unit "+k->getUnit().name);
				double add;
				if(rates){
					k->getRate(t,add);
				}else{
					k->get(t,add);
				}
				total+=add;
				count+=1;
			}//else this plant did not exist yet, ignore
		}//else ignore, this is not a plant
	}
	if(mean) total/=count;
}

std::string SumOverPlantsShoot::getName()const{
	return "sumOverAllPlantShoots";
}

DerivativeBase * newInstantiationSumOverPlantsShoot(SimulaDynamic* const pSD){
   return new SumOverPlantsShoot(pSD);
}




SumOverPlants::SumOverPlants(SimulaDynamic* pSD):DerivativeBase(pSD), mean(false),n(1)
{
	//name we are looking for
	SimulaBase *p=pSD->existingChild("name");
	if(p){
		p->get(name);
	}else{
		name=pSD->getName();
	};
	if(name.substr(0,4)=="mean"){
		name.erase(0,4);
		name[0]=tolower(name[0]);
		mean=true;
	}
	rates=false;
	SimulaVariable* v(dynamic_cast<SimulaVariable* >(pSD));
	if(v){
		rates=true;
	}else{
		rates=false;
	}

	if(pSD->getParent(2)->getName()=="plants") {
		container=pSD->getParent()->getName();
		n=2;
	}

}

void SumOverPlants::calculate(const Time &t,double &total){

	//loop over plants
	total=0;

	//find all plants //this is assuming plants are planted immediately, but possibly with different dates.
	SimulaBase::List	plants;
	pSD->getParent(n)->getAllChildren(plants,t);

	//get leaf area

	//sum up leaf area of all plants
	double count=0.;
	for(auto & it:plants){
		SimulaBase*	k=(it)->existingChild("plantingTime");
		if(k) {
			Time pt;
			k->get(pt);
			if(t>=pt){
				k=(it);
				if(container.size()) k=k->getChild(container,t);
				k=k->getChild(name,t);
				if(k->getUnit()!=pSD->getUnit()) msg::error("SumOverPlants: Unit checking failed for \'"+pSD->getName()+"\' with unit "+pSD->getUnit().name+" while plant \'"+(it)->getName()+"\' has unit "+k->getUnit().name);
				double add;
				if(rates){
					k->getRate(t,add);
				}else{
					k->get(t,add);
				}
				total+=add;
				count+=1;
			}//else this plant did not exist yet, ignore
		}//else ignore, this is not a plant
	}
	if(mean) total/=count;
}

std::string SumOverPlants::getName()const{
	return "sumOverAllPlants";
}

DerivativeBase * newInstantiationSumOverPlants(SimulaDynamic* const pSD){
   return new SumOverPlants(pSD);
}



//Register the module
class AutoRegisterTotalsFoorAllPlantsInstantiationFunctions {
public:
   AutoRegisterTotalsFoorAllPlantsInstantiationFunctions() {
 		BaseClassesMap::getDerivativeBaseClasses()["sumOverAllPlantShoots"] = newInstantiationSumOverPlantsShoot;
 		BaseClassesMap::getDerivativeBaseClasses()["meanOverAllPlantShoots"] = newInstantiationSumOverPlantsShoot;
 		BaseClassesMap::getDerivativeBaseClasses()["sumOverAllPlantsNutrients"] = newInstantiationSumOverPlants;
 		BaseClassesMap::getDerivativeBaseClasses()["meanOverAllPlantsNutrients"] = newInstantiationSumOverPlants;
 		BaseClassesMap::getDerivativeBaseClasses()["sumOverAllPlants"] = newInstantiationSumOverPlants;
 		BaseClassesMap::getDerivativeBaseClasses()["meanOverAllPlants"] = newInstantiationSumOverPlants;
   };
};

// our one instance of the proxy
static AutoRegisterTotalsFoorAllPlantsInstantiationFunctions l465484357357651435447;

