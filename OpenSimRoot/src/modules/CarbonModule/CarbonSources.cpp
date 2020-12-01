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
#include "CarbonSources.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
//#include "Constants.hpp"

//TOTAL CARBON AVAILABLE FROM PHOTOSYNTHESIS AND SEEDRESERVES (G/DAY)
PlantCarbonIncomeRate::PlantCarbonIncomeRate(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//initiation time
	pSD->getSibling("plantingTime")->get(plantingTime);
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//photosythesis simulator
	photosynthesisSimulator=pSD->getSibling("plantPosition")->getChild("shoot")->getChild("photosynthesis","g");
	//seedreserves simulator
	seedreservesSimulator=pSD->getSibling("reserves","g");
	//carbon conversion factor.
	CinDryWeight=pSD->getSibling("carbonToDryWeightRatio","100%");
}
void PlantCarbonIncomeRate::calculate(const Time &t,double &rate){
	//see if crop is already planted
	if (t<plantingTime) {rate=0;return;}
	//get rate of photosynthesis (in g carbon)
	photosynthesisSimulator->getRate(t,rate);
	//add rate of carbon release from seed reserves (in g dryweight - assume carbon content of 0.54)
	double r(0);
	seedreservesSimulator->getRate(t,r);
	//C in dry weight
	double cdw;
	CinDryWeight->get(t,cdw);
	//rate calculation, note rate is negative, for seed reserves are depleted
	rate-=r*cdw;
	if(rate<0) {
		//photosynthesisSimulator->getRate(t,rate);
		msg::warning("PlantCarbonIncomeRate: rate<0");

	}
}
std::string PlantCarbonIncomeRate::getName()const{
	return "plantCarbonIncomeRate";
}
DerivativeBase * newInstantiationPlantCarbonIncomeRate(SimulaDynamic* const pSD){
   return new PlantCarbonIncomeRate(pSD);
}

CarbonAvailableForGrowth::CarbonAvailableForGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//carbonIncome
	carbonIncome=pSD->getSibling("plantCarbonIncome","g");
	//respiration simulator
	respirationSimulator=pSD->existingSibling("plantRespiration","g"); //rate = g/day
	if(!respirationSimulator) msg::warning("CarbonAvailableForGrowth: Respiration is not simulated - assuming it is 0");
	//Subtract exudates and other costs
	carbonCostOfExudates=pSD->existingSibling("rootCarbonCosts","g");
	if(!carbonCostOfExudates) carbonCostOfExudates=pSD->existingSibling("rootCarbonCostOfExudates","g"); //rate = g/day
	if(!carbonCostOfExudates) msg::warning("CarbonAvailableForGrowth: plantCarbonCost(OfExudates) not found. Not including the costs. ");
	//set limit if storage/reserves is available
	storage=pSD->existingSibling("carbonReserves","g"); //rate = g/day
}
void CarbonAvailableForGrowth::calculate(const Time &t,double &c){
	//get total carbon available (g/day)
	carbonIncome->getRate(t,c);
	//Subtract respiration
	double r1(0),r2(0),r3(0);
	if(respirationSimulator){
		respirationSimulator->getRate(t,r1);
	}
	//Subtract carbon costs because of exudates
	if(carbonCostOfExudates){
		carbonCostOfExudates->getRate(t,r2);
	}
	//carbon going to reserves
	if(storage){
		storage->getRate(t,r3);
	}
    c-=(r1+r2+r3);

    //check if respiration is higher than income rate
	if(c< 0)	{
		if(t>pSD->getStartTime()+1.){
			//ignore this message for the first day, as we are running on seed reserves anyway and the estimates are not good because carbon Income is 0.
			msg::warning("CarbonAvailableForGrowth:  negative values: carbon production lower than compulsory cost of respiration/exudates? Setting growth to 0, this will cause balance errors");
		}
		c=0;
	}
}
std::string CarbonAvailableForGrowth::getName()const{
	return "carbonAvailableForGrowth";
}
DerivativeBase * newInstantiationCarbonAvailableForGrowth(SimulaDynamic* const pSD){
   return new CarbonAvailableForGrowth(pSD);
}

//registration of classes
class AutoRegisterCarbonSources{
public:
   AutoRegisterCarbonSources() {
		BaseClassesMap::getDerivativeBaseClasses()["carbonAvailableForGrowth"] = newInstantiationCarbonAvailableForGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["plantCarbonIncomeRate"] = newInstantiationPlantCarbonIncomeRate;
   }
} static p;



