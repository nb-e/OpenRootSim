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
#include "../../engine/BaseClasses.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include "../../cli/Messages.hpp"

class LeafSenescence:public DerivativeBase {
public:
protected:
	SimulaBase *SLA, *senescenceRate, *CinDryWeight, *leafArea, *senescedLeafArea;
	Time lt,plantingTime;

	void calculate(const Time &t, double& r){
		senescenceRate->get(t-plantingTime,r);
		if(SLA){
			Time st(lt); //time at which the leaf was created, to get appropriate sla.
			double sl,sla;
			senescedLeafArea->get(t,sl);//current amount of senesced leafs
			//std::cout<<std::endl<<sl<<" "<<st<<" "<<lt<<" "<<t;
			leafArea->getTime(sl,st,lt,t);
			lt=st;
			SLA->get(st-plantingTime,sla);
			r*=sla;//convert from cm2 to g; cm2*g/cm2=g
		};
	};
public:
	LeafSenescence(SimulaDynamic* const pSV):DerivativeBase(pSV){
		//plant type
		std::string plantType;
		PLANTTYPE(plantType,pSD)
		//planting time
		pSD->getParent(3)->getChild("plantingTime")->get(plantingTime);
		//plant parameters
		SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));

		senescenceRate=parameters->getChild("leafAreaSenescenceRate","cm2/day");

		//check if unit given in input file agrees with this function
		if(pSD->getUnit()=="g") {
			//carbon conversion factor.
			SLA         = parameters->getChild("specificLeafArea","g/cm2");
			leafArea    = pSD->getSibling("leafArea","cm2");
			senescedLeafArea = pSD->getSibling("senescedLeafArea","cm2");
		}else if (pSD->getUnit()=="cm2"){
            //do nothing
			SLA=NULL;
			senescedLeafArea=pSD;
		}else{
			msg::error("LeafSenescence: Expecting unit cm2 or g but found "+pSD->getUnit().name);
		}

		//last time
		lt=pSD->getStartTime();
		plantingTime=lt;
	};
	std::string getName()const{
		return "leafSenescence";
	};
};


DerivativeBase * newInstantiationLeafSenescence(SimulaDynamic* const pSD){
   return new LeafSenescence(pSD);
}




//==================registration of the classes=================
static class AutoRegisterLeafSenescenceClassInstantiationFunctions {
public:
   AutoRegisterLeafSenescenceClassInstantiationFunctions() {
	  BaseClassesMap::getDerivativeBaseClasses()["leafSenescence"] = newInstantiationLeafSenescence;
   };
} p49fg3g9;

