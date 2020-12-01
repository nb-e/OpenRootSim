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
#if _WIN32 || _WIN64
#define _USE_MATH_DEFINES
#endif
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#include "CarbonSinks.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include <math.h>

//returns potential growth of the root in g/day CARBON
//this is the old function that only does longitudinal growth and works for the whole root
//replace with the node one and use for the whole root the total function if you want to do secondary growth
RootPotentialCarbonSinkForGrowth::RootPotentialCarbonSinkForGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//warning message since radial growth is not implemented yet
	msg::warning("RootPotentialCarbonSinkForGrowth: Assuming no secondary growth for roots, use other module if you do want secondary growth");
	//longitudinal growth simulator
	lgSimulator=pSD->getSibling("growthpoint")->getChild("rootPotentialLongitudinalGrowth");
	//root diameter simulator
	rdSimulator=lgSimulator->getSibling("rootDiameter","cm");
	//pointer to plant
	SimulaBase * plantTop(pSD);
	PLANTTOP(plantTop)
	//plantType
	std::string plantType;
	plantTop->getChild("plantType")->get(plantType);
	//get the root type
	std::string rootType;
	pSD->getSibling("rootType")->get(rootType);
	//pointer to object that gives root type specific density
	densitySimulator=ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild(rootType)->getChild("density","g/cm3");
	//carbon conversion factor.
	CinDryWeight=plantTop->getChild("carbonToDryWeightRatio","100%");}
void RootPotentialCarbonSinkForGrowth::calculate(const Time &t,double &carbon){
	//get density in g/cm3
	double density;
	densitySimulator->get(density);
	//get current longitudinal growth rate in cm/day
	double li(0);
	lgSimulator->getRate(t,li);

	//get current radius of the growth point in cm
	double r(0);
	rdSimulator->get(t,r);
	r/=2;

	//calculate potential volumetric increase from longitudinal growth (cm3)
	li*=r*r*M_PI; // cm3/day

	//get current radial growth
	///@todo, this has to be implemented. For now, warning message is given in initiation
	//get current length of the root
	//calculate potential volumetric increase from radial growth
	double ri(0);//cm3/day

	///@todo currently this does not take into account the branges that may be created. we can solve this by creating branches earlier with a near zero growthrate - would be more realistic too.

	//C in dry weight
	double cdw;
	CinDryWeight->get(t,cdw);
	//calculate carbon needed for volumetric increase
	carbon=(ri+li)*density*cdw;//cm3/day*g/cm3=g/day
	if(carbon<0) {
		msg::error("RootPotentialCarbonSinkForGrowth: negative growth");
	}
}
std::string RootPotentialCarbonSinkForGrowth::getName()const{
	return "rootPotentialCarbonSinkForGrowth";
}
DerivativeBase * newInstantiationRootPotentialCarbonSinkForGrowth(SimulaDynamic* const pSD){
   return new RootPotentialCarbonSinkForGrowth(pSD);
}

//this one does both secondary and longitudinal growth.
RootNodePotentialCarbonSinkForGrowth::RootNodePotentialCarbonSinkForGrowth(SimulaDynamic* pSD):DerivativeBase(pSD),
	growth(nullptr), 
	surfaceArea(nullptr), 
	density(nullptr), 
	diameter(nullptr), 
	CinDryWeight(nullptr)
{
	//root diameter simulator
	growth=pSD->existingSibling("rootPotentialLongitudinalGrowth","cm");
	if(growth){
		//we are doing longitudinal growth
		diameter=pSD->getSibling("rootDiameter","cm");
		surfaceArea=nullptr;
	}else{
		//we are doing secondary growth
		growth=pSD->getSibling("rootPotentialSecondaryGrowth","cm");
		surfaceArea=pSD->getSibling("rootSegmentSurfaceArea","cm2");
	}
	//pointer to plant
	SimulaBase * plantTop(pSD);
	PLANTTOP(plantTop)
	//plantType
	std::string plantType;
	plantTop->getChild("plantType")->get(plantType);
	//get the root type
	std::string rootType;
	if(surfaceArea){
		pSD->getParent(3)->getChild("rootType")->get(rootType);
	}else {
		pSD->getParent(2)->getChild("rootType")->get(rootType);
	}
	//pointer to object that gives root type specific density
	density=GETROOTPARAMETERS(plantType,rootType)->getChild("density","g/cm3");
	//carbon conversion factor.
	CinDryWeight=plantTop->getChild("carbonToDryWeightRatio","100%");
}
void RootNodePotentialCarbonSinkForGrowth::calculate(const Time &t,double &c){
	//C in dry weight
	double cdw;
	CinDryWeight->get(t,cdw);
	//get density in g/cm3
	density->get(c);
	c*=cdw;
	//get volume increase, either in diameter or in length
	double g(0);
	growth->getRate(t,g);	
	if(surfaceArea){
		//this is a root node, calculate carbon needed for secondary growth
		//g is the increase in diameter
		//increase in volume (assuming cylinder of length unity):
		// pi*(r+g/2)^2 - pi*(r)^2
		// or approximately, assuming this is not a cylinder, that is g/2<<r
		// surface*thickness = s*g/2
		//anymore precision, based radial growth, increase in length over the timestep etc, doesnot seem to help the carbon balance much, and sometimes makes it worse.
		double g1(g);
		SimulaBase *next(growth->followChain(t));
		if(next!=growth){
			next->getRate(t,g1);//for backward compatibility if the growth point does not simulate this.
		}else{
			g1=0;//for backward compatibility if the growth point does not simulate this. Should be 0 for the growthpoint anyway
		}

		double s;
		surfaceArea->get(t,s);

		c*=s*(g1+g)/4.;
	}else{
		//get current radius of the growth point in cm
		c*=g;
		diameter->get(t,g);
		//this is a growth point
		g/=2;
		g*=g*M_PI;//cm3/day*g/cm3=g/day
		//calculate carbon needed for growth
		c*=g;
	}
}
std::string RootNodePotentialCarbonSinkForGrowth::getName()const{
	return "rootNodePotentialCarbonSinkForGrowth";
}
DerivativeBase * newInstantiationRootNodePotentialCarbonSinkForGrowth(SimulaDynamic* const pSD){
   return new RootNodePotentialCarbonSinkForGrowth(pSD);
}

LeafPotentialCarbonSinkForGrowth::LeafPotentialCarbonSinkForGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//growth simulator
	gSimulator=pSD->existingSibling("stressAdjustedPotentialLeafArea","cm2");
	if(!gSimulator) gSimulator=pSD->getSibling("potentialLeafArea","cm2");
	//pointer to plant
	SimulaBase * plantTop(pSD);
	PLANTTOP(plantTop)
	//plantType
	std::string plantType;
	plantTop->getChild("plantType")->get(plantType);
	//plantingTime
	plantTop->getChild("plantingTime")->get(plantingTime);
	//pointer to object that gives root type specific leaf area (g/cm2
	densitySimulator=GETSHOOTPARAMETERS(plantType)->getChild("specificLeafArea","g/cm2");
	//check units
	// unit of carbon (g)
	if(pSD->getUnit().element(1)!=densitySimulator->getUnit().element(1))
		msg::error("LeafPotentialCarbonSinkForGrowth:  units differ: sink in "+pSD->getUnit().element(1)+" while density in "+densitySimulator->getUnit().element(2));
	// unit of leaf (most likely area, but possibly weight)
	if(gSimulator->getUnit().element(1)!=densitySimulator->getUnit().element(2))
		msg::error("LeafPotentialCarbonSinkForGrowth:  units differ: growth in "+gSimulator->getUnit().element(1)+" while density in "+densitySimulator->getUnit().element(1));
	//carbon conversion factor.
	CinDryWeight=plantTop->getChild("carbonToDryWeightRatio","100%");
}
void LeafPotentialCarbonSinkForGrowth::calculate(const Time &t,double &carbon){
	//get specificLeafArea in g/cm2
	double d;
	densitySimulator->get(t,d);
	//get current potential growth in cm2/day
	double g;
	gSimulator->getRate(t,g);
	//C in dry weight
	double cdw;
	CinDryWeight->get(t,cdw);
	//calculate carbon needed for growth
	carbon=g*d*cdw; //  (cm2/day) * (g/cm2) * (g/g) = g/day
}
std::string LeafPotentialCarbonSinkForGrowth::getName()const{
	return "leafPotentialCarbonSinkForGrowth";
}
DerivativeBase * newInstantiationLeafPotentialCarbonSinkForGrowth(SimulaDynamic* const pSD){
   return new LeafPotentialCarbonSinkForGrowth(pSD);
}


StemPotentialCarbonSinkForGrowth::StemPotentialCarbonSinkForGrowth(SimulaDynamic* pSD):
		DerivativeBase(pSD),leafSinkSimulator(nullptr), leafAllocSimulator(nullptr),
		stemDW(nullptr),CinDryWeight(nullptr),pot(nullptr),act(nullptr),plantingTime(0)
{
	//todo check units

	//pointer to plant
	SimulaBase * plantTop(pSD);
	PLANTTOP(plantTop)
	//plant type
	std::string plantType;
	pSD->getParent(3)->getChild("plantType")->get(plantType);
	//plantingTime
	plantTop->getChild("plantingTime")->get(plantingTime);
	//pointer to object that gives increases in stem dw
	CinDryWeight=plantTop->getChild("carbonToDryWeightRatio","100%");
	stemDW=GETSHOOTPARAMETERS(plantType)->existingChild("stemDryWeightAccumulation","g/day");
	//scaling relative to leafs
	if(stemDW){
		act=pSD->existingSibling("stressAdjustedPotentialLeafArea","cm2");
		if(act) 		pot=pSD->existingSibling("potentialLeafArea","cm2");
		pottil=pSD->existingSibling("potentialTillers","#");
		if(pottil) acttil=pSD->getSibling("tillers","#");
	}else{
		//stems are a fraction of the leaf biomass
		leafSinkSimulator=pSD->getSibling("leafPotentialCarbonSinkForGrowth","g");
		//plant parameters
		SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
		//carbon allocation simulator (1-root)
		leafAllocSimulator=parameters->getChild("resources")->getChild("carbonAllocation2LeafsFactor","100%");
	}
}
void StemPotentialCarbonSinkForGrowth::calculate(const Time &t,double &carbon){
	if(stemDW){
		//better, easier to parameterize new implementation
		stemDW->get(t-plantingTime,carbon);
		double d;
		CinDryWeight->get(t,d);
		carbon*=d;
		if(pottil){ //note that at te moment potential leaf area is already reduced for number of tillers.
			pottil->get(t,d);
			carbon/=(d+1);
			acttil->get(t,d);
			carbon*=(1+d);
		}
		if(act){
			pot->getRate(t,d);
			if(d>1e-6){
				carbon/=d;
				act->getRate(t,d);
				carbon*=d;
			}else{
				//there is no leaf area
				carbon=0;
			}
		}

	}else{
		//original fall back
		leafSinkSimulator->getRate(t,carbon);
		double a;
		leafAllocSimulator->get(t,a);
		//shoot = leaf+stem && leaf= alloc*shoot
		//thus stem= shoot-leaf = leaf/alloc - leaf = leaf*(1/alloc-1)
		carbon*=((1./a)-1.);
	}
}
std::string StemPotentialCarbonSinkForGrowth::getName()const{
	return "stemPotentialCarbonSinkForGrowth";
}
DerivativeBase * newInstantiationStemPotentialCarbonSinkForGrowth(SimulaDynamic* const pSD){
   return new StemPotentialCarbonSinkForGrowth(pSD);
}


RootSystemPotentialCarbonSinkForGrowth::RootSystemPotentialCarbonSinkForGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//pointer to plant
	SimulaBase * plantTop(pSD->getParent());
	//PLANTTOP(plantTop)
	//plantType
	std::string plantType;
	plantTop->getChild("plantType")->get(plantType);
	//plantingTime
	plantTop->getChild("plantingTime")->get(plantingTime);
	//pointer to object that gives root type specific Root area (g/cm2
	gSimulator=GETRESOURCEPARAMETERS(plantType)->getSibling("root")->getChild("rootDryWeightAccumulation","g/day");
	//carbon conversion factor.
	CinDryWeight=plantTop->getChild("carbonToDryWeightRatio","100%");
}
void RootSystemPotentialCarbonSinkForGrowth::calculate(const Time &t,double &carbon){
	//get current potential growth in cm2/day
	double g;
	gSimulator->get(t-plantingTime,g);
	//C in dry weight
	double cdw;
	CinDryWeight->get(t,cdw);// about 0.42
	//calculate carbon needed for growth
	carbon=g*cdw; //  (cm2/day) * (g/cm2) * (g/g) = g/day
}
std::string RootSystemPotentialCarbonSinkForGrowth::getName()const{
	return "rootSystemPotentialCarbonSinkForGrowth";
}
DerivativeBase * newInstantiationRootSystemPotentialCarbonSinkForGrowth(SimulaDynamic* const pSD){
   return new RootSystemPotentialCarbonSinkForGrowth(pSD);
}



class AutoRegisterCarbonSinks {
public:
   AutoRegisterCarbonSinks() {
		BaseClassesMap::getDerivativeBaseClasses()["rootPotentialCarbonSinkForGrowth"] = newInstantiationRootPotentialCarbonSinkForGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["rootSystemPotentialCarbonSinkForGrowth"] = newInstantiationRootSystemPotentialCarbonSinkForGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["rootNodePotentialCarbonSinkForGrowth"] = newInstantiationRootNodePotentialCarbonSinkForGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["leafPotentialCarbonSinkForGrowth"] = newInstantiationLeafPotentialCarbonSinkForGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["stemPotentialCarbonSinkForGrowth"] = newInstantiationStemPotentialCarbonSinkForGrowth;
   }
}static p;

//static AutoRegisterCarbonBalanceInstantiationFunctions p;



