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

#include  "WaterUptakeByRoots.hpp"
#include "../PlantType.hpp"

WaterUptakeFromHopmans::WaterUptakeFromHopmans(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//Check the units
	if(pSD->getUnit()=="cm3") {
		mode=1;
	}else if (pSD->getUnit()=="cm3/cm"){
		mode=0;
	}else{
		msg::error("WaterUptakeFromHopmans: expected unit cm3");
	}
	//pointer to total transpiration of this plant
	SimulaBase *plant(pSD);
	PLANTTOP(plant)
	potentialTranspiration=plant->getChild("plantPosition")->getChild("shoot")->getChild("potentialTranspiration","cm3");
	//pointer to segment length
	segmentLength=pSD->getSibling("rootSegmentLength","cm");
	//check position of the root
	Coordinate pos;
	pSD->getAbsolute(pSD->getStartTime(),pos);
	if(pos.y>=0) mode=-1;

	//pointer to total root length
	totalRootLength=plant->existingChild("rootLengthBelowTheSurface",segmentLength->getUnit());
	if(!totalRootLength){
		totalRootLength=plant->getChild("rootLongitudinalGrowth",segmentLength->getUnit());
		if(mode<0) msg::warning("WaterUptakeFromHopmans: rootsegment is above ground, but averaging potential transpiration over total root length. Consider adding rootLenthBelowTheSurface to your plant template.");
	}

}
void WaterUptakeFromHopmans::calculate(const Time &t,double &total){
	if(mode==-1){
		//root is above ground, set uptake to 0
		total=0;
		return;
	}
	//get the total transpiration
	double tr;
	potentialTranspiration->getRate(t,tr);
	//get fraction
	double tl;
	totalRootLength->get(t,tl);
	if(tl!=0){
		if(mode){
			double sl;
			segmentLength->get(t,sl);
			total=tr*(sl/tl);
		}else{
			total=tr/tl;
		}
	}else{
		if (tr!=0)msg::warning("WaterUptakeFromHopmans: Transpiration but zero root length.");
		total=0;
	}
}
std::string WaterUptakeFromHopmans::getName()const{
	return "waterUptakeFromHopmans";
}

DerivativeBase * newInstantiationWaterUptakeFromHopmans(SimulaDynamic* const pSD){
   return new WaterUptakeFromHopmans(pSD);
}


ScaledWaterUptake::ScaledWaterUptake(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//Check the units
	if(pSD->getUnit()=="cm3") {
		length=nullptr;
	}else if (pSD->getUnit()=="cm3/cm"){
		length=pSD->getSibling("rootSegmentLength","cm");;
	}else{
		msg::error("ScaledWaterUptake: expected unit cm3");
	}
	//pointer to total transpiration of this plant
	SimulaBase *plant(pSD);
	PLANTTOP(plant)
	actualTranspiration=plant->getChild("plantPosition")->getChild("shoot")->getChild("actualTranspiration","cm3");;
	total=plant->getChild("rootPotentialWaterUptake","cm3");
	fraction=pSD->getSibling("rootSegmentPotentialWaterUptake","cm3");
}
void ScaledWaterUptake::calculate(const Time &t,double &uptake){
	//get the total transpiration
	actualTranspiration->getRate(t,uptake);
	//get fraction
	double tl;
	total->getRate(t,tl);
	if(tl!=0){
		double sl;
		fraction->getRate(t,sl);
		uptake*=(sl/tl);
		if(length){
			length->get(t,sl);
			if(sl>0){
				uptake/=sl;
			}else{
				uptake=0;
			}
		}
	}else{
		uptake=0;
	}
	if(std::isnan(uptake))
		msg::error("ScaledWaterUptake: value is nan");
}
std::string ScaledWaterUptake::getName()const{
	return "scaledWaterUptake";
}

DerivativeBase * newInstantiationScaledWaterUptake(SimulaDynamic* const pSD){
   return new ScaledWaterUptake(pSD);
}





GetValuesFromPlantWaterUptake::GetValuesFromPlantWaterUptake(SimulaDynamic* pSD):DerivativeBase(pSD){
	SimulaBase * top=pSD->getParent();
	if(!top->existingChild("plantPosition") ) {
		PLANTTOP(top);
	}
	wu=top->getChild("rootWaterUptakeCheck");
}
std::string GetValuesFromPlantWaterUptake::getName()const{
	return "getValuesFromPlantWaterUptake";
}
void GetValuesFromPlantWaterUptake::calculate(const Time &t,double &var){
	wu->get(t,var);
}
DerivativeBase * newInstantiationGetValuesFromPlantWaterUptake(SimulaDynamic* const pSD) {
	return new GetValuesFromPlantWaterUptake(pSD);
}



//Register the module
class AutoRegisterWaterUptakeInstantiationFunctions {
public:
   AutoRegisterWaterUptakeInstantiationFunctions() {
 		BaseClassesMap::getDerivativeBaseClasses()["waterUptakeFromHopmans"] = newInstantiationWaterUptakeFromHopmans;
		BaseClassesMap::getDerivativeBaseClasses()["scaledWaterUptake"] = newInstantiationScaledWaterUptake;
 		BaseClassesMap::getDerivativeBaseClasses()["getValuesFromPlantWaterUptake"] = newInstantiationGetValuesFromPlantWaterUptake;
   }
};

// our one instance of the proxy
static AutoRegisterWaterUptakeInstantiationFunctions l654645648435135753;

