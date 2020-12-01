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
#include "Photosynthesis.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include <math.h>

//lintul based shoot simulation
PhotosynthesisLintul::PhotosynthesisLintul(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//planting Time
	pSD->getParent(3)->getChild("plantingTime")->get(plantingTime);
	//simulators
	lightInterceptionSimulator=pSD->getSibling("lightInterception");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD)
	SimulaBase * param(ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot"));
	//get Light Use Efficiency note, this can be carbon or drymatter based, in other words in default lintul version respiration costs are taken care off in the lue, however in the orginal sucros model this was not the case
	lightUseEfficiencySimulator=param->getChild("lightUseEfficiency");
	//get area per plant
	areaSimulator=param->getChild("areaPerPlant");
	//unit checks
	// unit of CO2 (most likely g)
	if(pSD->getUnit().element(1)!=lightUseEfficiencySimulator->getUnit().element(1))
		msg::error("PhotosynthesisLintul: CO2 units differ: photosyntesis in "+pSD->getUnit().element(1)+" while LUE in "+lightUseEfficiencySimulator->getUnit().element(1));
	//energy unit Mj or the like
	if(lightUseEfficiencySimulator->getUnit().element(2)!=lightInterceptionSimulator->getUnit().element(1))
		msg::error("PhotosynthesisLintul: energy units differ: LUE in "+lightUseEfficiencySimulator->getUnit().element(2)+" while lightInterception in "+lightInterceptionSimulator->getUnit().element(1));
	//surface area unit (most likely cm2)
	if(areaSimulator->getUnit()!=lightInterceptionSimulator->getUnit().element(2))
		msg::error("PhotosynthesisLintul: area units differ: areaPerPlant in "+areaSimulator->getUnit().name+", while light interception in "+lightInterceptionSimulator->getUnit().element(2));

	//stress
	stress=pSD->getParent(3)->existingChild("stressFactor");
	if(stress) {
		adjust=param->getChild("photosynthesisStressResponse");
		msg::warning("PhotosynthesisLintul: Including stress factor.");
	}
	//RCA
	SimulaBase* contr(ORIGIN->getChild("simulationControls")->existingChild("aerenchyma"));
	if(contr) contr=contr->getChild("includePhotosynthesisEffects");
	bool flag(false);
	if(contr) contr->get(flag);
	if(flag){
		rca=param->existingChild("aerenchymaPhotosynthesisMitigation");
		if(rca) msg::warning("PhotosynthesisLintul: including RCA mitigation factor");
	}else{
		rca=nullptr;
	}

}
void PhotosynthesisLintul::calculate(const Time &t, double &photosynthesis){
	//localTime
	Time localTime=t-plantingTime;
	//area per plant
	double areaPerPlant;
	areaSimulator->get(localTime,areaPerPlant);
	//LUE
	double LUE;
	lightUseEfficiencySimulator->get(localTime,LUE);
	//calculate Intercepted Photosynthetic Available Radiation
	double PARINT;//Mj/cm2/day
	lightInterceptionSimulator->get(t,PARINT);
	//calculate drymatter production rate. note LUE may or may not have been compensated for respiration (g/J/cm2/day)
	photosynthesis= LUE*PARINT*areaPerPlant;//(g/MJ)*(MJ/cm2/day)*cm2 = g/day

	//stress
	if(stress){
		double s;
		stress->get(t,s);
		double sa;
		adjust->get(s,sa);
		if(rca){
			double a;
			rca->get(localTime,a);
			//1-((1-sa)*(1-a))=sa+a-sa*a
			sa+=(a*(1-sa));
		}
		photosynthesis*=sa;
	}

	//check
	if (photosynthesis<0) msg::error("PhotosynthesisLintul: photosynthesis<0");
}
std::string PhotosynthesisLintul::getName()const{
	return "photosynthesisLintul";
}

DerivativeBase * newInstantiationPhotosynthesisLintul(SimulaDynamic* const pSD){
   return new PhotosynthesisLintul(pSD);
}


//lintul based shoot simulation
PhotosynthesisLintulV2::PhotosynthesisLintulV2(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//planting Time
	pSD->getParent(3)->getChild("plantingTime")->get(plantingTime);
	//simulators
	lightInterceptionSimulator=pSD->getSibling("lightInterception");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD)
	SimulaBase * param(ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot"));
	//get Light Use Efficiency note, this can be carbon or drymatter based, in other words in default lintul version respiration costs are taken care off in the lue, however in the orginal sucros model this was not the case
	lightUseEfficiencySimulator=param->getChild("lightUseEfficiency");
	//get area per plant
	areaSimulator=param->getChild("areaPerPlant");
	//unit checks
	// unit of CO2 (most likely g)
	if(pSD->getUnit().element(1)!=lightUseEfficiencySimulator->getUnit().element(1))
		msg::error("PhotosynthesisLintulV2: CO2 units differ: photosyntesis in "+pSD->getUnit().element(1)+" while LUE in "+lightUseEfficiencySimulator->getUnit().element(1));
	//energy unit Mj or the like
	if(lightUseEfficiencySimulator->getUnit().element(2)!=lightInterceptionSimulator->getUnit().element(1))
		msg::error("PhotosynthesisLintulV2: energy units differ: LUE in "+lightUseEfficiencySimulator->getUnit().element(2)+" while lightInterception in "+lightInterceptionSimulator->getUnit().element(1));
	//surface area unit (most likely cm2)
	if(areaSimulator->getUnit()!=lightInterceptionSimulator->getUnit().element(2))
		msg::error("PhotosynthesisLintulV2: area units differ: areaPerPlant in "+areaSimulator->getUnit().name+", while light interception in "+lightInterceptionSimulator->getUnit().element(2));
	//stress
	stress=pSD->getParent(3)->existingChild("stressFactor:impactOn:photosynthesis");
	if(!stress)	msg::warning("PhotosynthesisLintulV2: no stress impact factor found");

}
void PhotosynthesisLintulV2::calculate(const Time &t, double &photosynthesis){
	//localTime
	Time localTime=t-plantingTime;
	//area per plant
	double areaPerPlant;
	areaSimulator->get(localTime,areaPerPlant);
	//LUE
	double LUE;
	lightUseEfficiencySimulator->get(localTime,LUE);
	//calculate Intercepted Photosynthetic Available Radiation
	double PARINT;//Mj/cm2/day
	lightInterceptionSimulator->get(t,PARINT);
	//calculate drymatter production rate. note LUE may or may not have been compensated for respiration (g/J/cm2/day)
	photosynthesis= LUE*PARINT*areaPerPlant;//(g/MJ)*(MJ/cm2/day)*cm2 = g/day

	//stress
	if(stress){
		double s;
		stress->get(t,s);
		photosynthesis*=s;
	}

	//check
	if (photosynthesis<0) msg::error("PhotosynthesisLintulV2: photosynthesis<0");
}
std::string PhotosynthesisLintulV2::getName()const{
	return "photosynthesisLintulV2";
}
DerivativeBase * newInstantiationPhotosynthesisLintulV2(SimulaDynamic* const pSD){
   return new PhotosynthesisLintulV2(pSD);
}




LightInterception::LightInterception(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//simulators
	irradiationSimulator=ORIGIN->getChild("environment")->getChild("atmosphere")->getChild("irradiation");
	leafAreaIndexSimulator=pSD->getSibling("leafAreaIndex");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD)
	//get extinction coefficient (KDF)
	KDF=GETSHOOTPARAMETERS(plantType)->getChild("extinctionCoefficient");
	//correction factor for converting RDD to PAR (NORMALLY 0.5)
	if(irradiationSimulator->existingSibling("PAR/RDD")){
		irradiationSimulator->getSibling("PAR/RDD")->get(RDDPAR);
	}else{
		RDDPAR=1;
	}
	//check if unit given in input file agrees with this function
	if(pSD->getUnit()!=irradiationSimulator->getUnit())
		msg::error("LightInterception: units differ - light inteception in "+pSD->getUnit().name+" while irradiation in "+irradiationSimulator->getUnit().name);
}
void LightInterception::calculate(const Time &t, double &PARINT){
	//get irradiation
	double RDD;
	irradiationSimulator->get(t,RDD);
	//calculate Photosynthetially avialable radiation (PAR), PAR is generally considered 1/2 RDD
	double PAR(RDDPAR*RDD);//KJ/cm2/day*1000=Mj/cm2/day
	//get leaf area index
	double LAI;
	leafAreaIndexSimulator->get(t,LAI);//cm2/cm2
	//calculate Intercepted Photosynthetic Available Radiation
	double k;
	KDF->get(t-pSD->getStartTime(),k);
	PARINT=PAR*(1-exp(-k*LAI));//Mj/cm2/day
}
std::string LightInterception::getName()const{
	return "lightInterception";
}

DerivativeBase * newInstantiationLightInterception(SimulaDynamic* const pSD){
   return new LightInterception(pSD);
}



//==================registration of the classes=================
class AutoRegisterPhotosynthesisInstantiationFunctions {
public:
   AutoRegisterPhotosynthesisInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["photosynthesisLintul"] = newInstantiationPhotosynthesisLintul;
		BaseClassesMap::getDerivativeBaseClasses()["photosynthesisLintulV2"] = newInstantiationPhotosynthesisLintulV2;
		BaseClassesMap::getDerivativeBaseClasses()["lightInterception"] = newInstantiationLightInterception;
   };
};



// our one instance of the proxy
static AutoRegisterPhotosynthesisInstantiationFunctions p;


