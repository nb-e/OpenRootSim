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
#include "SecondaryGrowth.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include <math.h>
#include "../PlantType.hpp"

RootDiameter::RootDiameter(SimulaDynamic* pSD):TotalBase(pSD),multiplier(1)
{
 	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type
	int pos(3);
	if (pSD->getParent()->getName()=="growthpoint") pos=2;
	std::string rootType;
	pSD->getParent(pos)->getChild("rootType")->get(rootType);
	//root type specific diameter simulator object
	SimulaBase* param(GETROOTPARAMETERS(plantType,rootType));
	rootTypeSpecificDiameterSimulator=param->existingChild("rootDiameter");
	if(!rootTypeSpecificDiameterSimulator) rootTypeSpecificDiameterSimulator=param->getChild("diameter");

	//stochasticity in length, may also effect diameter
	SimulaBase *p(pSD->getParent()->existingSibling("growthRateMultiplier"));
	if(p){
		p->get(multiplier);
		msg::warning("RootDiameter: using length - diameter relation. e.g. longer laterals are thicker",3);
		SimulaBase *c(param->existingChild("lengthMultiplier2DiameterMultiplier"));
		//data from pages suggests simple linear relation between diameter and length
		//but with an intercept. So we use a table to do the conversion.
		//note that pages data is species, but not root class specific,
		//and may not hold within root class.
		if(c){
			double lm(multiplier);
			c->get(lm,multiplier);
		}else{
			msg::warning("RootDiameter: no lengthMultiplier2DiameterMultiplier specified for "+rootType+"s of "+plantType,2);
		}
		if(multiplier<1e-10)multiplier=1e-10;
	}else{
		msg::warning("RootDiameter: NOT using length - diameter relation. e.g. longer laterals are thicker",3);
	}


}
void RootDiameter::calculate(const Time &t,double &d){
	//diameter of the root -- note, currently this is not simulated but a constant.
    rootTypeSpecificDiameterSimulator->get(t-pSD->getStartTime(),d);
    d*=multiplier;
}
std::string RootDiameter::getName()const{
	return "rootDiameter";
}

SumSteelCortex::SumSteelCortex(SimulaDynamic* pSD):TotalBase(pSD),
		steel(nullptr),cortex(nullptr)
{
		if(pSD->getName()=="rootDiameterNoRCS"){
			steel=pSD->getSibling("rootDiameterSteel",pSD->getUnit());
			cortex=(pSD->getSibling("rootDiameterCortexNoRCS",pSD->getUnit()));

		}else{
			steel=(pSD->getSibling(pSD->getName()+"Steel",pSD->getUnit()));
			cortex=(pSD->getSibling(pSD->getName()+"Cortex",pSD->getUnit()));
		}
}

void SumSteelCortex::calculate(const Time &t,double &d){
	double c;
	steel->get(t,d);
	cortex->get(t,c);
	d+=c;
}

std::string SumSteelCortex::getName()const{
	return "SumSteelCortex";
}



CortexDiameter::CortexDiameter(SimulaDynamic* pSD):TotalBase(pSD),rcs(nullptr)
{
	if(pSD->getName()!="rootDiameterCortexNoRCS"){
		rcs=pSD->getSibling("rootCorticalSenescenceStage","100%");
	}

	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type
	int pos(3);
	if (pSD->getParent()->getName()=="growthpoint") pos=2;
	std::string rootType;
	pSD->getParent(pos)->getChild("rootType")->get(rootType);
	//root type specific diameter simulator object
	SimulaBase* param(GETROOTPARAMETERS(plantType,rootType));


	double f;
	pSD->getParent(3)->getChild("growthpoint")->getChild("rootDiameter","cm")->get(pSD->getStartTime(),initialDiameter);
	param->getChild("initialCorticalDiameter","100%")->get(f);

	initialDiameter*=f;
}
void CortexDiameter::calculate(const Time &t,double &cortexDiameter){
	if(rcs){
		double f;
		rcs->get(t,f);//fraction
		cortexDiameter=initialDiameter*(1.-f);//current
	}else{
		cortexDiameter=initialDiameter;
	}
}

std::string CortexDiameter::getName()const{
	return "cortexDiameter";
}




SecondaryGrowth::SecondaryGrowth(SimulaDynamic* pSD):TotalBase(pSD), sim1(nullptr), scalingFactor(nullptr), sim2(nullptr)
{
	if(pSD->getName()=="rootDiameter"){
		sim2=pSD->existingSibling("rootDiameterSteel");
		if(sim2) {
			//simply add up cortex and steel
			sim1=pSD->getSibling("rootDiameterCortex");
			double d,dc;
			sim2->get(pSD->getStartTime(),d);
			sim1->get(pSD->getStartTime(),dc);
			pSD->setInitialValue(d+dc);
			msg::warning("SecondaryGrowth:: root diameter is sum cortex+steel, which, when RCS is enabled, may cause false messages about carbon balance errors.");
			return;
		}
	}

	//backwards compatibility warning (with previous input files otherwise we get eternal loop)
	if(pSD->getName()=="rootPotentialSecondaryGrowth") msg::error("SecondaryGrowth: This module does not support potential growth anymore, use function=\"potentialSecondaryGrowth\" instead of function=\"secondaryGrowth\" ");
	//initial diameter of the root is diameter of growth point when at this age
	double d;
	if(pSD->getName()=="rootDiameterSteel"){
		SimulaBase* gpd(pSD->getParent(3)->getChild("growthpoint")->getChild("rootDiameter","cm"));
		gpd->get(pSD->getStartTime(),d);
		double cd;
		pSD->getSibling("rootDiameterCortex",pSD->getUnit())->get(pSD->getStartTime(),cd);
		d-=cd;
	}else{
		SimulaBase* gpd(pSD->getParent(3)->getChild("growthpoint")->getChild(pSD->getName(),"cm"));
		gpd->get(pSD->getStartTime(),d);
	}

	//set initial diameter
	pSD->setInitialValue(d);

	//get the growth rate simulator
	sim1=pSD->existingSibling("rootPotentialSecondaryGrowth");
	if(sim1){
		//scale this if carbon model is on and this is not potential Growth
		//add optional scaling factor based on how far we are from the base of the root
		//TODO this could be a function of transport capacity which could be total rootl below this node
		//calculate by rootLength-root2base
		scalingFactor=pSD;
		PLANTTOP(scalingFactor)//move to the top of the plant
		scalingFactor=scalingFactor->existingChild("secondaryRootGrowthScalingFactor");
	}else{
		//no secondary growth
		auto p=dynamic_cast<SimulaVariable*>(pSD);
		if(p) p->set(1e10,StateRate(d,0.));
		//todo here we could signal simulavariable to stop updating completely and safe some memory (destruct this object)
	}
}



void SecondaryGrowth::calculate(const Time &t,double &d){
	if(sim2){
		//add up diameter of steel and cortex
		double c;
		sim2->getRate(t,d);
		sim1->getRate(t,c);
		d+=c;
	}else if(sim1){
		//no steel and cortex info, simulate on diameter using secondary growth
		//potential secondary growth
		sim1->getRate(t,d);
		//turn potential into actual growth using scaling factor
		if(d>0 && scalingFactor){
			double s;
			scalingFactor->get(t,s);
			d*=s;
		}
	}else{
		//no secondary growth, so we can simply ignore this.
		d=0;
	}
}
std::string SecondaryGrowth::getName()const{
	return "secondaryGrowth";
}


PotentialSecondaryGrowth::PotentialSecondaryGrowth(SimulaDynamic* pSD):TotalBase(pSD)
{
 	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type
	int pos(3);
	if (pSD->getParent()->getName()=="growthpoint") pos=2;
	std::string rootType;
	pSD->getParent(pos)->getChild("rootType")->get(rootType);
	//root type specific diameter simulator object
	SimulaBase* param(GETROOTPARAMETERS(plantType,rootType));
	//initial diameter of the root is diameter of growhtpoint when at this age
	double d;
	SimulaBase* gpd(pSD->getParent(3)->getChild("growthpoint")->getChild("rootDiameter","cm"));
	gpd->get(pSD->getStartTime(),d);
    pSD->setInitialValue(d);
    //get the growthrate simulator
	secondaryGrowthRate=param->existingChild("secondaryGrowthRate");
	//add optional scaling factor based on how far we are from the base of the root
	if (secondaryGrowthRate){
		SimulaBase * d2b(pSD->existingSibling("rootLength2Base","cm"));
		SimulaBase * f(param->existingChild("secondaryGrowthScalingFactor"));
		if(d2b && f){
			double l;
			d2b->get(pSD->getStartTime(),l);
			f->get(l,factor);
		}else{
			if(f && !d2b) msg::warning("SecondaryGrowth: scaling factor not used as rootLength2Base is missing");
			factor=1;
		}

		//scale this relative to the shoot size
		SimulaBase* top(pSD);PLANTTOP(top)
		SimulaBase* shoot(top->getChild("plantPosition")->getChild("shoot"));
		allometricScaling=shoot->existingChild("leafAreaReductionCoefficient","cm2/cm2");

		//include stress factor ('etiolation')
		SimulaBase* contr(ORIGIN->getChild("simulationControls")->existingChild("etiolation"));
		stress=nullptr;
		if(contr){
			contr=contr->getChild("reduceSecondaryGrowth");
			bool c;
			contr->get(c);
			if(c) stress=top->existingChild("stressFactor:impactOn:rootSegmentSecondaryGrowth");
		}
	}
}
void PotentialSecondaryGrowth::calculate(const Time &t,double &d){
	if(secondaryGrowthRate){
		secondaryGrowthRate->get(t-pSD->getStartTime(),d);
		d*=factor;
		//apply allometry to secondary growth based on leaf area basis
		if(allometricScaling){
			double c;
			allometricScaling->get(t,c);
			d*=c;
		}
		//etiolation in response to stress
		if(stress){
			double s(1);
			stress->get(t,s);
			d*=s;
		}
	}else{
		d=0;
	}
}
std::string PotentialSecondaryGrowth::getName()const{
	return "potentialSecondaryGrowth";
}

DerivativeBase * newInstantiationRootDiameter(SimulaDynamic* const pSD){
   return new RootDiameter(pSD);
}

DerivativeBase * newInstantiationSecondaryGrowth(SimulaDynamic* const pSD){
   return new SecondaryGrowth(pSD);
}

DerivativeBase * newInstantiationPotentialSecondaryGrowth(SimulaDynamic* const pSD){
   return new PotentialSecondaryGrowth(pSD);
}

DerivativeBase * newInstantiationSumSteelCortex(SimulaDynamic* const pSD){
	   return new SumSteelCortex(pSD);
}

DerivativeBase * newInstantiationCortexDiameter(SimulaDynamic* const pSD){
	   return new CortexDiameter(pSD);
}

//registration of classes
class AutoRegisterSecondaryGrowthInstantiationFunctions {
public:
   AutoRegisterSecondaryGrowthInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["rootDiameter"] = newInstantiationRootDiameter;
		BaseClassesMap::getDerivativeBaseClasses()["rootDiameter.v2"] = newInstantiationRootDiameter;
		BaseClassesMap::getDerivativeBaseClasses()["sumSteelCortex"] = newInstantiationSumSteelCortex;
		BaseClassesMap::getDerivativeBaseClasses()["rootDiameterCortex"] = newInstantiationCortexDiameter;
		BaseClassesMap::getDerivativeBaseClasses()["secondaryGrowth"] = newInstantiationSecondaryGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["potentialSecondaryGrowth"] = newInstantiationPotentialSecondaryGrowth;
   }
};

static AutoRegisterSecondaryGrowthInstantiationFunctions p;
