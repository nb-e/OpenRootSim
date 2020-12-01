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
#include  "Transpiration.hpp"
#include  "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include "../../cli/Messages.hpp"

PotentialTranspirationCrop::PotentialTranspirationCrop(SimulaDynamic* pSD):DerivativeBase(pSD){
	pSD->checkUnit("cm3");//todo assumed to be integrated

	cropLeafAreaIndex=pSD->existingPath("/plants/meanLeafAreaIndex","cm2/cm2");
	if(cropLeafAreaIndex){
		leafArea=pSD->getSibling("leafArea","cm2");
		cropTranspiration=pSD->existingPath("/atmosphere/potentialTranspiration","cm");
	}else{
		//no crop level transpiration computed, use fall back to compute individual plant transpiration
		//relativePotentialTranspiration;
		std::string plantType;
		PLANTTYPE(plantType,pSD)
		relativePotentialTranspiration=GETSHOOTPARAMETERS(plantType);
		relativePotentialTranspiration=relativePotentialTranspiration->getChild("relativePotentialTranspiration");
		//leafArea
		if(relativePotentialTranspiration->getUnit()=="cm3/cm2/day"){
			mode=1;
			leafArea=pSD->getSibling("leafArea","cm2");
			leafSenescence = pSD->existingSibling("senescedLeafArea");
		}else if(relativePotentialTranspiration->getUnit()=="cm3/g"){
			mode=2;
			leafArea=pSD->getSibling("photosynthesis","g");
		}else{
			msg::error("PotentialTranspirationCrop: unsupported unit '"+relativePotentialTranspiration->getUnit().name+"' for relativePotentialTranspiration. Use cm3/cm2/day or cm3/g");
		}

	}
}
std::string PotentialTranspirationCrop::getName()const{
	return "potentialTranspirationCrop";
}
void PotentialTranspirationCrop::calculate(const Time &t,double &trans){
	if(cropLeafAreaIndex){
		double la,lai;
		cropLeafAreaIndex->get(t,lai);
		if(lai<=1e-6) {
			trans=0;
		}else{
			leafArea->get(t,la);
			cropTranspiration->getRate(t,trans);
			trans/=lai; //cm per unit soil to cm per unit leaf area.
			trans*=la;
		}
	}else{
		//get leaf area
		double la;
		if(mode==1){
			leafArea->get(t,la);
			if(leafSenescence){
				double s;
				leafSenescence->get(t,s);
				la-=s;
			}
		}else{
			leafArea->getRate(t,la);
		}
		double r;
		//get transpiration/leaf area
		relativePotentialTranspiration->get(t-pSD->getStartTime(),r);
		//multiply
		trans=la*r;
	}
}
DerivativeBase * newInstantiationPotentialTranspirationCrop(SimulaDynamic* const pSD){
   return new PotentialTranspirationCrop(pSD);
}


ActualTranspiration::ActualTranspiration(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//potential transpiration, and potential water uptake
	potentialTranspiration=pSD->getSibling("potentialTranspiration",pSD->getUnit());
	potentialWaterUptake=pSD->getParent(3)->getChild("rootPotentialWaterUptake",pSD->getUnit());
}
void ActualTranspiration::calculate(const Time &t,double &trans){
	std::size_t np=pSD->getPredictorSize();
	potentialTranspiration->getRate(t,trans);
	if(trans>0){
		double uptake;
		potentialWaterUptake->getRate(t,uptake);
		if(trans>uptake*1.01 && uptake>0){
			trans=uptake;
			if(np==pSD->getPredictorSize()) msg::warning("ActualTranspiration: simulating drought");
		}
	}
}
std::string ActualTranspiration::getName()const{
	return "actualTranspiration";
}

DerivativeBase * newInstantiationActualTranspiration(SimulaDynamic* const pSD){
   return new ActualTranspiration(pSD);
}



//Register the module
class AutoRegisterTranspirationInstantiationFunctions {
public:
   AutoRegisterTranspirationInstantiationFunctions() {
 		BaseClassesMap::getDerivativeBaseClasses()["simplePotentialTranspiration"] = newInstantiationPotentialTranspirationCrop;//for backward compatibility
 		BaseClassesMap::getDerivativeBaseClasses()["actualTranspiration"] = newInstantiationActualTranspiration;
		BaseClassesMap::getDerivativeBaseClasses()["potentialTranspirationCrop"] = newInstantiationPotentialTranspirationCrop;
  };
};

// our one instance of the proxy
static AutoRegisterTranspirationInstantiationFunctions l654645648435135753;

