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
#include "LongitudinalGrowth.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaTable.hpp"
#include "../PlantType.hpp"
#include <math.h>
#include "../../tools/StringExtensions.hpp"

ConstantRootGrowthRate::ConstantRootGrowthRate(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("cm");
	//pointer to current root (top of this rootgrowthpoint root tree)
	SimulaBase * currentRoot(pSD->getParent(2));
	//start time of that root
	st=currentRoot->getStartTime();
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type
	std::string rootType;
	currentRoot->getChild("rootType")->get(rootType);
	//pointer to parameters
	SimulaBase * parameters(GETROOTPARAMETERS(plantType,rootType));
	//pointer to object that gives root type specific growth rates
	rootTypeSpecificGrowthRate=parameters->getChild("growthRate","cm/day");
	//check if this is a table with length instead of age in column 1
	SimulaTable<> *stp=dynamic_cast<SimulaTable<>* > (rootTypeSpecificGrowthRate);
	if(stp && stp->getUnitColum1()=="cm")   st=-1;
	//growthratemultiplier so we can add random effects
	//Time parentAge(pSD->getStartTime()-pSD->getParent(3)->getStartTime());
	SimulaBase *p(currentRoot->existingChild("growthRateMultiplier"));
	//if(!p)p=parameters->existingChild("longitudinalGrowthRateMultiplier");
	if(p){
		p->get(multiplier);
		msg::warning("ConstantRootGrowth: growthRateMultiplier found",3);
	}else{
		multiplier=1;
		msg::warning("ConstantRootGrowth: growthRateMultiplier not found",2);
	}
	//check for nutrient effects on respiration
	factor=pSD;
	PLANTTOP(factor);
	factor=factor->existingChild("stressFactor:impactOn:rootPotentialLongitudinalGrowth");

}
void ConstantRootGrowthRate::calculate(const Time &t,double &rate){
	//get the time since creation of this root
	Time lt;
	if(st<0){
		//get estimated length;
		pSD->get(t,lt);
	}else{
		//go by time
	    lt=t - st;
	}
	//get growthrate
	rootTypeSpecificGrowthRate->get(lt,rate);
	rate*=multiplier;
	//multiplication factor
	if(factor) {
		double r;
		factor->get(t,r);
		rate*=r;
	}
	if(rate<0.){
		msg::error("ConstantRootGrowthRate: negative growth rates detected. Check your growth rate parameters for "+pSD->getParent(2)->getPath());
	}
}
std::string ConstantRootGrowthRate::getName()const{
	return "potentialRootGrowthRate";
}
DerivativeBase * newInstantiationConstantRootGrowthRate(SimulaDynamic* const pSD){
   return new ConstantRootGrowthRate(pSD);
}

ScaledRootGrowthRate::ScaledRootGrowthRate(SimulaDynamic* pSD):DerivativeBase(pSD),gsfSimulator(nullptr)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("cm");
	//find growth scaling factor Simulator (0-1)
	SimulaBase *top(pSD);PLANTTOP(top)
	//See if we are using a separate scaling factor for major axis
	SimulaBase *p;
	p=pSD->getParent(2)->existingChild("rootSystemPotentialCarbonSinkForGrowth;majorAxis");
	if(p){//separate scaling factor for major axis
		std::string rootClasses;
		p->getChild("includedRootClasses")->get(rootClasses);
		std::string thisRootsRootClass;
		pSD->getParent(2)->getChild("rootType")->get(thisRootsRootClass);
		//build list (we could search the string directly, but we want to be sure that spaces in names are allowed and that laterals is not find if lateralsOfPrimaryRoot is present
		std::set<std::string> list;
		std::string::size_type pos(0);
		while (pos<rootClasses.size()){
			list.insert(nextWord(rootClasses,pos,','));
		}
		//search list
		if(list.find(thisRootsRootClass)!=list.end())
			gsfSimulator=top->existingChild("rootGrowthScalingFactor;majorAxis");
	}
	if(!gsfSimulator) gsfSimulator=top->existingChild("rootGrowthScalingFactor");

	//for now just take potential growthrate in cm/day future maybe g/day?
	if(gsfSimulator){
		lgSimulator=pSD->getSibling("rootPotentialLongitudinalGrowth");
	}else{
		lgSimulator=pSD->existingSibling("rootPotentialLongitudinalGrowth");
		if(lgSimulator){
			st=-2.;
			multiplier=1.;
		}else{
			//growth is not scaled (carbon model may not be on) and we will do potential growth.
			//pointer to current root (top of this rootgrowthpoint root tree)
			SimulaBase * currentRoot(pSD->getParent(2));
			//start time of that root
			st=currentRoot->getStartTime();
			//plant type
			std::string plantType;
			top->getChild("plantType")->get(plantType);
			//get the root type
			std::string rootType;
			currentRoot->getChild("rootType")->get(rootType);
			//pointer to parameters
			SimulaBase * parameters(GETROOTPARAMETERS(plantType,rootType));
			//pointer to object that gives root type specific growth rates
			lgSimulator=parameters->getChild("growthRate","cm/day");
			//check if this is a table with length instead of age in column 1
			SimulaTable<> *stp=dynamic_cast<SimulaTable<>* > (lgSimulator);
			if(stp && stp->getUnitColum1()=="cm")   st=-1;
			//growthratemultiplier
			//Time parentAge(pSD->getStartTime()-pSD->getParent(3)->getStartTime());
			SimulaBase *p(currentRoot->existingChild("growthRateMultiplier"));
			//if(!p)p=parameters->existingChild("longitudinalGrowthRateMultiplier");
			if(p){
				p->get(multiplier);
				msg::warning("ScaledRootGrowth: growthRateMultiplier found",3);
			}else{
				multiplier=1;
				msg::warning("ScaledRootGrowth: growthRateMultiplier not found",2);
			}
		}

	}
}
void ScaledRootGrowthRate::calculate(const Time &t,double &rate){
	if(gsfSimulator){//we are running carbon model
		//get potential growth
		lgSimulator->getRate(t,rate); // cm/day

		//get scaling factor
		double scale;
		if(rate>0){
			auto pl(SimulaBase::getPredictorSize());
			gsfSimulator->get(t,scale);
			if(scale<0){
				//only warning if the negative does not come from the predictor
				if(SimulaBase::getPredictorSize()==pl){
					msg::warning("ScaledRootGrowthRate: getting negative scaling factor. Setting growth to 0. This could cause balance errors.");}
				scale=0;
			}

			//Scale the rate accordingly
			rate*=scale; //cm/day
		}
	}else{//no carbon module do potential growth
		//get the time since creation of this root
		Time lt;
		if(st<-1.5){
			lt=t; //just passing value from potentialLongitudinalGrowth (there is no scaling factor), but potential Growth might still have a multiplier for the nutrient environment.
			lgSimulator->getRate(lt,rate);
		}else{
			if(st<-0.5){
				//get estimated length;
				pSD->get(t,lt); //using parameter table based on length
			}else{
				//go by time
				lt=t - st; //using parameter table based on time
			}
			//get growthrate
			lgSimulator->get(lt,rate);
			rate*=multiplier;
		}
	}
	if(rate<0){
		lgSimulator->getRate(t,rate); // cm/day
		msg::error("ScaledRootGrowthRate: negative growth rates detected. Check your growth rate parameters for "+pSD->getParent(2)->getPath());
	}
}
std::string ScaledRootGrowthRate::getName()const{
	return "scaledRootGrowthRate";
}

DerivativeBase * newInstantiationScaledRootGrowthRate(SimulaDynamic* const pSD){
   return new ScaledRootGrowthRate(pSD);
}


//registration of classes
class AutoRegisterLongitudinalGrowthInstantiationFunctions {
public:
   AutoRegisterLongitudinalGrowthInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["potentialRootGrowthRate"] = newInstantiationConstantRootGrowthRate;
		BaseClassesMap::getDerivativeBaseClasses()["scaledRootGrowthRate"] = newInstantiationScaledRootGrowthRate;
   }
};

static AutoRegisterLongitudinalGrowthInstantiationFunctions p;

