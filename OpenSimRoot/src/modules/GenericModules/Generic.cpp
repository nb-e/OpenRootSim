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
#include "Generic.hpp"
#include "../../engine/DataDefinitions/Units.hpp"
#include "../../cli/Messages.hpp"
#include "../../math/VectorMath.hpp"
#include "../PlantType.hpp"

//class integrates fluxdensity over the length of a root segment thus calculating nutrient uptake by a segment
IntegrateOverSegment::IntegrateOverSegment(SimulaDynamic* const pSV):
	TotalBase(pSV)
{
	//current and next datapoint
	current=pSD->getParent();
	//simulator name
	sName=pSD->getName();
	sName.erase(4,7);
	msg::warning("IntegrateOverSegment:: this is old, not maintained code used for "+sName);
	//unit we are looking for
	std::string unit;
	unit=pSD->getUnit().name;
	unit.insert(unit.size()-4,"/cm");
	//check units
	if(current->getChild(sName)->getUnit()!=unit ) msg::error("IntegrateOverSegment: Expecting unit "+pSD->getUnit().name+" for "+sName+" in DataPoints." );
}
void IntegrateOverSegment::calculate(const Time &t, double& rate){
	//This function integrated flux over root length between two nodes.
	//next point (with the same name as this!
	SimulaBase *next(getNext(t)->getParent());

	//get positions of datapoints
	Coordinate pos0;
	current->get(t,pos0);
	Coordinate pos1;
	next->get(t,pos1);

	//get rates at these datapoints
	//TODO with some metadata, possibly in the name of pSV, we could use this function for more than one nutrient. Same metadata should return in the name of nutrientInFlux
	double rateD0(0);
	current->getChild(sName,t)->get(t,rateD0);
	double rateD1(0);
	next->getChild(sName,t)->get(t,rateD1);

	//integration of the length of the root segment
	rate= (rateD0+rateD1)*vectorlength(pos0,pos1)/2;
}
std::string IntegrateOverSegment::getName()const{
	return "IntegrateOverSegment";
}

DerivativeBase * newInstantiationIntegrateOverSegment(SimulaDynamic* const pSV){
   return new IntegrateOverSegment(pSV);
}


UseDerivative::UseDerivative(SimulaDynamic* const pSD):
	DerivativeBase(pSD)
{
	//name of simulator
	SimulaBase *p=pSD->existingChild("path");
	if(p) {
		std::string name;
		p->get(name);
		//simulator
		derivativeSimulator=pSD->getParent()->getPath(name);
	}else{
		std::string name(pSD->getName());
		name.insert(name.size(),"Rate");
		//simulator
		derivativeSimulator=pSD->getSibling(name);
	}
}
void UseDerivative::calculate(const Time &t, double& d){
	derivativeSimulator->get(t,d);
}
void UseDerivative::calculate(const Time &t, Coordinate& d){
	derivativeSimulator->get(t,d);
}
std::string UseDerivative::getName()const{
	return "useDerivative";
}
DerivativeBase * newInstantiationUseDerivative(SimulaDynamic* const pSD){
   return new UseDerivative(pSD);
}

//TOTAL CARBON CURRENTLY USED BY RESPIRATION (G/DAY)
//TODO maybe this should be rate state -> change the other code than as well.
PlantTotal::PlantTotal(SimulaDynamic* pSD):DerivativeBase(pSD),primaryRoot(nullptr),hypocotyl(nullptr),shootBornRoots(nullptr),primaryRoot2(nullptr), hypocotyl2(nullptr), shootBornRoots2(nullptr), stems(nullptr),leafs(nullptr),runmode(0)
{
	unsigned int mode(1);
	std::string group;
	//check if this is grouped under a different label
	if(pSD->getParent(2)->existingChild("plantingTime")){
		group=pSD->getParent()->getName();
		mode=2;
	}
	//simulator name
	std::string sim(pSD->getName());
	//are we doing whole plant, shoot only of root only
	if(sim.substr(0,5)=="plant") {
		primaryRoot=pSD->getParent(mode)->getChild("plantPosition")->existingChild("primaryRoot");
		if(primaryRoot){
			runmode=1;
		}else{
			//root is not simulated
			runmode=4;
		}
	}else if(sim.substr(0,5)=="shoot") {
		runmode=2;
	}else if(sim.substr(0,4)=="root") {
		runmode=3;
	}else{
		msg::error("PlantTotal: name should start with plant/shoot/root for "+sim);
	}

	//convert names
	if (runmode==3){
		sim.erase(0,4);
	}else{
		sim.erase(0,5);
	}
	std::string simRoot("rootSystem"+sim);
	std::string simRootSecondary("rootSystemSecondary"+sim);
	std::string simLeafs("leaf"+sim);
	std::string simStems("stem"+sim);

	if(runmode==1 || runmode==3){
		//sim of the primary rootsystem
		primaryRoot=pSD->getParent(mode)->getChild("plantPosition")->getChild("primaryRoot");
		if(mode==1) {
			primaryRoot=primaryRoot->getChild(simRoot,pSD->getUnit());
			primaryRoot2=primaryRoot->existingSibling(simRootSecondary,pSD->getUnit());
		}else{
			primaryRoot=primaryRoot->getChild(group)->getChild(simRoot,pSD->getUnit());
		}
		//sim of the hypocotyl based rootsystem
		hypocotyl=pSD->getParent(mode)->getChild("plantPosition")->getChild("hypocotyl");
		if(mode==1){
			hypocotyl=hypocotyl->getChild(simRoot,pSD->getUnit());
			hypocotyl2=hypocotyl->existingSibling(simRootSecondary,pSD->getUnit());
		}else{
			hypocotyl=hypocotyl->getChild(group)->getChild(simRoot,pSD->getUnit());
		}
		//sim for shoot born roots if existing
		shootBornRoots=pSD->getParent(mode)->getChild("plantPosition")->getChild("shoot");
		if(mode==1){
			shootBornRoots2=shootBornRoots;
			shootBornRoots=shootBornRoots->existingChild(simRoot,pSD->getUnit());
			shootBornRoots2=shootBornRoots2->existingChild(simRootSecondary,pSD->getUnit());
		}else{
			shootBornRoots=shootBornRoots->getChild(group)->existingChild(simRoot,pSD->getUnit());
		}
	}
	if(runmode==1 || runmode==2){
		//sim for the shoot
		stems=pSD->getParent(mode)->getChild("plantPosition")->getChild("shoot");
		leafs=stems;
		if (mode==1){
			leafs=leafs->getChild(simLeafs,pSD->getUnit());
			stems=stems->getChild(simStems,pSD->getUnit());
		}else{
			leafs=leafs->getChild(group)->getChild(simLeafs,pSD->getUnit());
			stems=stems->getChild(group)->getChild(simStems,pSD->getUnit());
		}
	}
	if(runmode==4){
		//just add up shoot+root
		leafs=pSD->getSibling("shoot"+sim,pSD->getUnit());
		stems=pSD->getSibling("root"+sim,pSD->getUnit());
	}
	//are we getting rates or not?
	if(pSD->getType()!="SimulaVariable") runmode*=-1;
	//we want to be able to set the initial value in the resource section
	std::string plantType;
	pSD->getParent(mode)->getChild("plantType")->get(plantType);
	SimulaBase * siv(GETRESOURCEPARAMETERS(plantType));
	if(mode==2)siv=siv->existingChild(pSD->getParent()->getName());
	if(siv) siv=siv->existingChild("initial"+sim);
	if(siv){
		msg::warning("PlantTotal: Using initial value for "+pSD->getName()+" from parameter section",1);
		double iv;
		siv->get(pSD->getStartTime(),iv);
		pSD->setInitialValue(iv);
	}
}
void PlantTotal::calculate(const Time &t,double &total){
	double add(0);
	total=0;
	if(runmode>0){//we are doing rates
		//sum up results from all simulators
		if(runmode==1 || runmode==3){
			primaryRoot->getRate(t,add);
			total+=add;
			hypocotyl->getRate(t,add);
			total+=add;
			if(shootBornRoots!=nullptr){
				shootBornRoots->getRate(t,add);
				total+=add;
			}
			if(primaryRoot2){
				primaryRoot2->getRate(t,add);
				total+=add;
			}
			if(hypocotyl2){
				hypocotyl2->getRate(t,add);
				total+=add;
			}
			if(shootBornRoots2){
				shootBornRoots2->getRate(t,add);
				total+=add;
			}
		}
		if(runmode==1 || runmode==2 || runmode==4){
			leafs->getRate(t,add);
			total+=add;
			stems->getRate(t,add);
			total+=add;
		}
	}else{
		//sum up results from all simulators
		if(runmode==-1 || runmode==-3){
			primaryRoot->get(t,add);
			total+=add;
			hypocotyl->get(t,add);
			total+=add;
			if(shootBornRoots!=nullptr){
				shootBornRoots->get(t,add);
				total+=add;
			}
			if(primaryRoot2){
				primaryRoot2->get(t,add);
				total+=add;
			}
			if(hypocotyl2){
				hypocotyl2->get(t,add);
				total+=add;
			}
			if(shootBornRoots2){
				shootBornRoots2->get(t,add);
				total+=add;
			}
		}
		if(runmode==-1 || runmode==-2){
			leafs->get(t,add);
			total+=add;
			stems->get(t,add);
			total+=add;
		}
	}
}
std::string PlantTotal::getName()const{
	return "plantTotal";
}
DerivativeBase * newInstantiationPlantTotal(SimulaDynamic* const pSD){
   return new PlantTotal(pSD);
}

//use parameter from parameter section

UseParameterFromParameterSection::UseParameterFromParameterSection(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//get name of table
	std::string name(pSD->getName());
	//get name of group
	std::string group(pSD->getParent()->getName());
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);

	//determine if we are doing this for the roots or the shoot
	//determine if we are doing the parameter is listed under a group
	if(pSD->getParent()->getName()=="shoot"){
		//shoot without group
		SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));
		//set pointer to table
		sim=parameters->getChild(name);
	}else if(pSD->getParent(2)->getName()=="shoot"){
		//shoot with group
		SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));
		//set pointer to table
		sim=parameters->getChild(group)->getChild(name);
	}else if(pSD->getParent()->getName()=="growthpoint"){
		//growthpoint without group
		//get the root type parameters
		std::string rootType;
		pSD->getParent(3)->getChild("rootType")->get(rootType);
		SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
		//set pointer to table
		sim=parameters->getChild(name);
	}else if(pSD->getParent(2)->getName()=="growthpoint"){
		//growthpoint with group
		//get the root type parameters
		std::string rootType;
		pSD->getParent(3)->getChild("rootType")->get(rootType);
		SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
		//set pointer to table
		sim=parameters->getChild(group)->getChild(name);
	}else if(pSD->getParent(3)->existingChild("rootType")){
		//datapoint without group
		//get the root type parameters
		std::string rootType;
		pSD->getParent(3)->getChild("rootType")->get(rootType);
		SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
		//set pointer to table
		sim=parameters->getChild(name);
	}else{
		//datapoint with group
		//get the root type parameters
		std::string rootType;
		pSD->getParent(4)->getChild("rootType")->get(rootType);
		SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
		//set pointer to table
		sim=parameters->getChild(group)->getChild(name);
	}

}
void UseParameterFromParameterSection::calculate(const Time &t,double &result){
	Time localTime(t-pSD->getStartTime());
	sim->get(localTime,result);
}
std::string UseParameterFromParameterSection::getName()const{
	return "useParameterFromParameterSection";
}

DerivativeBase * newInstantiationUseParameterFromParameterSection(SimulaDynamic* const pSD){
   return new UseParameterFromParameterSection(pSD);
}

//++++++++++++++++++++++
PointSensor::PointSensor(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	pSD->getChild("location")->get(location);
	std::string path;
	pSD->getChild("path")->get(path);
	sim=ORIGIN->getPath(path,pSD->getUnit());
}
void PointSensor::calculate(const Time &t,double &result){
	sim->get(t,location,result);
}
std::string PointSensor::getName()const{
	return "pointSensor";
}

DerivativeBase * newInstantiationPointSensor(SimulaDynamic* const pSD){
   return new PointSensor(pSD);
}

//Register the module
class AutoRegisterIntegrateOverSegmentInstantiationFunctions {
public:
   AutoRegisterIntegrateOverSegmentInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["integrateOverSegment"] = newInstantiationIntegrateOverSegment;
		BaseClassesMap::getDerivativeBaseClasses()["useDerivative"] = newInstantiationUseDerivative;
		BaseClassesMap::getDerivativeBaseClasses()["usePath"] = newInstantiationUseDerivative;
		BaseClassesMap::getDerivativeBaseClasses()["useName+Rate"] = newInstantiationUseDerivative;
		BaseClassesMap::getDerivativeBaseClasses()["useParameterFromParameterSection"] = newInstantiationUseParameterFromParameterSection;
		//legacy entry, do not remove.
		BaseClassesMap::getDerivativeBaseClasses()["useRootClassSpecificTable"] = newInstantiationUseParameterFromParameterSection;

		BaseClassesMap::getDerivativeBaseClasses()["plantTotal"] = newInstantiationPlantTotal;
 		BaseClassesMap::getDerivativeBaseClasses()["plantTotalForNutrients"] = newInstantiationPlantTotal;
		BaseClassesMap::getDerivativeBaseClasses()["plantTotalRootFraction"] = newInstantiationPlantTotal;
 		BaseClassesMap::getDerivativeBaseClasses()["plantTotalShootFraction"] = newInstantiationPlantTotal;
		BaseClassesMap::getDerivativeBaseClasses()["plantTotalRates"] = newInstantiationPlantTotal;
		BaseClassesMap::getDerivativeBaseClasses()["plantTotalRatesRootFraction"] = newInstantiationPlantTotal;
 		BaseClassesMap::getDerivativeBaseClasses()["plantTotalRatesShootFraction"] = newInstantiationPlantTotal;
		BaseClassesMap::getDerivativeBaseClasses()["pointSensor"] = newInstantiationPointSensor;
   }
};

// our one instance of the proxy
static AutoRegisterIntegrateOverSegmentInstantiationFunctions p;


