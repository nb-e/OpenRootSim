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
#include "CarbonAllocation.hpp"
#include "../../cli/Messages.hpp"
//#include "../../Toolkit/StringExtensions.hpp"
#include "../../engine/Origin.hpp"
//#include "../../DataDefinitions/Units.hpp"
#include "../PlantType.hpp"
#include <math.h>
//#include "Constants.hpp"
//#include "../../MathLibrary/MathLibrary.hpp"

//CARBON ALLOCATION TO ROOT - SHOOT - AND OTHER ORGANS (G/DAY)


//Relative carbon allocation (as fraction of total available carbon)
RelativeCarbonAllocation2RootsFromInputFile::RelativeCarbonAllocation2RootsFromInputFile(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//plantingTime
	plantingTime=pSD->getStartTime();
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//plant parameters
	SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
	//carbon allocation simulator
	c2rootSimulator=parameters->getChild("resources")->getChild("carbonAllocation2RootsFactor","100%");
}
void RelativeCarbonAllocation2RootsFromInputFile::calculate(const Time &t,double &scale){
	//carbon allocation to root factor
	c2rootSimulator->get((t-plantingTime),scale);
	if(scale<0)msg::error("CarbonAllocation2Root: allocation < 0");
}
std::string RelativeCarbonAllocation2RootsFromInputFile::getName()const{
	return "relativeCarbonAllocation2RootsFromInputFile";
}

DerivativeBase * newInstantiationCarbonRelativeAllocation2RootsFromInputFile(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2RootsFromInputFile(pSD);
}
RelativeCarbonAllocation2ShootFromInputFile::RelativeCarbonAllocation2ShootFromInputFile(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//plantingTime
	plantingTime=pSD->getStartTime();
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//plant parameters
	SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
	//carbon allocation simulator (1-root)
	c2rootSimulator=parameters->getChild("resources")->getChild("carbonAllocation2RootsFactor","100%");
}
void RelativeCarbonAllocation2ShootFromInputFile::calculate(const Time &t,double &scale){
	//carbon allocation to root factor
	c2rootSimulator->get((t-plantingTime),scale);
	scale=1-scale;
	if(scale<0)msg::error("RelativeCarbonAllocation2ShootFromInputFile: allocation < 0");
}
std::string RelativeCarbonAllocation2ShootFromInputFile::getName()const{
	return "relativeCarbonAllocation2ShootFromInputFile";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2ShootFromInputFile(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2ShootFromInputFile(pSD);
}

RelativeCarbonAllocation2LeafsFromInputFile::RelativeCarbonAllocation2LeafsFromInputFile(SimulaDynamic* pSD):
		DerivativeBase(pSD),c2leafsSimulator(nullptr), leafsink(nullptr), stemsink(nullptr)
{
	//plantingTime
	plantingTime=pSD->getStartTime();
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//plant type
	std::string plantType;
	pSD->getParent(3)->getChild("plantType")->get(plantType);
	//plant parameters
	SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
	stemsink=parameters->getChild("shoot")->existingChild("stemDryWeightAccumulation","g/day");
	if(stemsink){
		//based on the relative sink strength
		stemsink=pSD->getSibling("stemPotentialCarbonSinkForGrowth","g");
		leafsink=pSD->getSibling("leafPotentialCarbonSinkForGrowth","g");
	}else{
		//base input file
		//carbon allocation simulator (1-root)
		c2leafsSimulator=parameters->getChild("resources")->getChild("carbonAllocation2LeafsFactor","100%");
	}
}
void RelativeCarbonAllocation2LeafsFromInputFile::calculate(const Time &t,double &scale){
	if(stemsink){
		double stem;
		leafsink->getRate(t,scale);
		stemsink->getRate(t,stem);
		if((scale+stem)>1e-6) {
			scale/=(scale+stem);
		}else{
			scale=1;
		}
	}else{
		//carbon allocation to root factor
		c2leafsSimulator->get((t-plantingTime),scale);
	}
	if(scale>1){
		msg::warning("RelativeCarbonAllocation2LeafsFromInputFile: allocation > 1");
		scale=1;
	}
	if(scale<0){
		msg::warning("RelativeCarbonAllocation2LeafsFromInputFile: allocation < 0");
		scale=0;
	}
}
std::string RelativeCarbonAllocation2LeafsFromInputFile::getName()const{
	return "relativeCarbonAllocation2LeafsFromInputFile";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2LeafsFromInputFile(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2LeafsFromInputFile(pSD);
}

RelativeCarbonAllocation2RootsPotentialGrowth::RelativeCarbonAllocation2RootsPotentialGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//plantingTime
	plantingTime=pSD->getStartTime();
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon available (g)
	Ctotal=pSD->getSibling("carbonAvailableForGrowth","g");
	//Total carbon required for potential growth
	Cpotential=pSD->getSibling("rootPotentialCarbonSinkForGrowth","g");
}
void RelativeCarbonAllocation2RootsPotentialGrowth::calculate(const Time &t,double &scale){
	//get total carbon available (g/day)
	double c;
	Ctotal->getRate(t,c);
	if (c<=0) msg::error("RelativeCarbonAllocation2RootsPotentialGrowth: There is no carbon available for growth");
	//carbon needed for potential growth
	double p;
	Cpotential->getRate(t,p);
	//calculate scaling factor.
	scale=p/c;
	if(scale>1) scale=1;
}
std::string RelativeCarbonAllocation2RootsPotentialGrowth::getName()const{
	return "relativeCarbonAllocation2RootsPotentialGrowth";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2RootsPotentialGrowth(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2RootsPotentialGrowth(pSD);
}



RelativeCarbonAllocation2ShootPotentialGrowth::RelativeCarbonAllocation2ShootPotentialGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon available (g/day)
	Ctotal=pSD->getSibling("carbonAvailableForGrowth","g");
	//Total carbon required for potential growth
	Cpotential=pSD->getSibling("shootPotentialCarbonSinkForGrowth","g");
	//plantingTime
	plantingTime=pSD->getStartTime();
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//plant parameters
	SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
	//threshold parameter
	threshold=parameters->getChild("resources")->existingChild("maxCarbonAllocation2Shoot","100%");
	//potential root growth
	carbon4potentialRootGrowth=pSD->getSibling("rootPotentialCarbonSinkForGrowth","g");
}
void RelativeCarbonAllocation2ShootPotentialGrowth::calculate(const Time &t,double &scale){
	std::size_t nmbpred = pSD->getPredictorSize();
	double p;
	//carbon for potential shoot growth
	Cpotential->getRate(t, p);
	//calculate relative allocation.
	if (p <= 0) {
		if (p < 0 && nmbpred == pSD->getPredictorSize())
			msg::error(
					"RelativeCarbonAllocation2ShootPotentialGrowth: Potential growth is negative");
		scale = 0;
	} else {
		//get total carbon available (g/day)
		double c;
		Ctotal->getRate(t, c);
		if (c > 0) {
			//scaling factor
			scale = p / c;
		} else {
			scale = 0;
		}
		//do not allow this to be larger than threshold
		if (threshold) {
			//make sure we only reduce allocation to shoot when it would otherwise reduce root growth
			double cr;
			carbon4potentialRootGrowth->getRate(t, cr);
			cr /= c;
			if (scale + cr > 1) {//we are carbon limited
				double trsh;
				threshold->get(t - plantingTime, trsh);
				//make sure that the threshold does not cause the roots to grow more than potential.
				double rtrsh(1 - cr); //scale needed for potential root growth
				if (rtrsh > trsh)
					trsh = rtrsh; //maximize scale for shoot growth.
				//set threshold
				if (scale > trsh) {
					//check if we need to limit root growth.
					if (nmbpred == pSD->getPredictorSize())
						msg::warning(
								"RelativeCarbonAllocation2ShootPotentialGrowth: Not enough carbon for potential growth. Setting shoot growth to max available carbon.");
					scale = trsh;
				}
			}
		}
	}
}
std::string RelativeCarbonAllocation2ShootPotentialGrowth::getName()const{
	return "relativeCarbonAllocation2ShootPotentialGrowth";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2ShootPotentialGrowth(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2ShootPotentialGrowth(pSD);
}

RelativeCarbonAllocation2ShootSwitch::RelativeCarbonAllocation2ShootSwitch(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon available (g/day)
	Ctotal=pSD->getSibling("carbonAvailableForGrowth","g");
	//Total carbon required for potential growth
	Cpotential=pSD->existingSibling("shootPotentialCarbonSinkForGrowth","g");
	//plantingTime
	plantingTime=pSD->getStartTime();
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//plant parameters
	SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
	//threshold parameter
	threshold=parameters->getChild("resources")->existingChild("maxCarbonAllocation2Shoot","100%");
	//carbon allocation simulator (1-root)
	parameterSection=parameters->getChild("resources")->getChild("carbonAllocation2RootsFactor","100%");
}
void RelativeCarbonAllocation2ShootSwitch::calculate(const Time &t,double &scale){
	std::size_t predsize=pSD->getPredictorSize();
	double potentialScale(1.), inputScale(1.);
	Time age(t-plantingTime);
	if(age<20){
		double p,c;
		//carbon for potential shoot growth
		Cpotential->getRate(t,p);
		//get total carbon available (g/day)
		Ctotal->getRate(t,c);
		//check
		if(p>c){
			if(predsize==pSD->getPredictorSize()) msg::warning("RelativeCarbonAllocation2ShootSwitch: Not enough carbon for potential growth. Setting shoot growth to max available carbon.");
			p=c;
		}
		//calculate relative allocation.
		if (c<=0 || p<=0) {
			if(p<0 && predsize==pSD->getPredictorSize())	msg::error("RelativeCarbonAllocation2ShootSwitch: Potential growth is negative");
			scale=0;
		}else{
			//scaling factor
			scale=p/c;
		}
		//do not allow this to be larger than threshold
		double trsh(1.0);//don't change this a backward compatibility default
		if(threshold)threshold->get(t-plantingTime,trsh);
		scale=fmin(scale,trsh);
		potentialScale=scale;
	}
	if(age>10){
		//carbon allocation to root factor
		parameterSection->get(age,scale);
		scale=1-scale;
		inputScale=scale;
	}
	if(age<10){
		scale=potentialScale;
	}else if(age<20){//smooth transition
		double weight((age-10)/10);
		scale=inputScale*weight+potentialScale*(1-weight);
	}else{
		scale=inputScale;
	}

}
std::string RelativeCarbonAllocation2ShootSwitch::getName()const{
	return "relativeCarbonAllocation2ShootSwitch";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2ShootSwitch(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2ShootSwitch(pSD);
}

RelativeCarbonAllocation2RootsScaledGrowth::RelativeCarbonAllocation2RootsScaledGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon required for potential growth
	CpotentialRoot=pSD->getSibling("rootPotentialCarbonSinkForGrowth","g");
	CpotentialPlant=pSD->getSibling("plantPotentialCarbonSinkForGrowth","g");
}
void RelativeCarbonAllocation2RootsScaledGrowth::calculate(const Time &t,double &scale){
	//carbon necessary for potential growth
	double r,p;
	CpotentialRoot->getRate(t,r);
	CpotentialPlant->getRate(t,p);
	if (p<=0) msg::error("RelativeCarbonAllocation2RootsScaledGrowth: Potential growth of plant <= 0");
	///@todo: weight factor
	//double d(1);
	//if(weight!=nullptr) weight->get(t,d);
	//meaning if root get's 30% more allocated than it would get on equal terms - shoot get's that absolute amount less
	scale=r/p;
}
std::string RelativeCarbonAllocation2RootsScaledGrowth::getName()const{
	return "relativeCarbonAllocation2RootsScaledGrowth";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2RootsScaledGrowth(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2RootsScaledGrowth(pSD);
}

RelativeCarbonAllocation2ShootScaledGrowth::RelativeCarbonAllocation2ShootScaledGrowth(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon required for potential growth
	CpotentialShoot=pSD->getSibling("shootPotentialCarbonSinkForGrowth","g");
	CpotentialPlant=pSD->getSibling("plantPotentialCarbonSinkForGrowth","g");
}
void RelativeCarbonAllocation2ShootScaledGrowth::calculate(const Time &t,double &scale){
	//carbon necessary for potential growth
	double s,p;
	CpotentialShoot->getRate(t,s);
	CpotentialPlant->getRate(t,p);
	if (p<=0) msg::error("RelativeCarbonAllocation2ShootScaledGrowth: Potential growth of plant <= 0");
	///@todo: weight factor
	//double d(1);
	//if(weight!=nullptr) weight->get(t,d);
	//meaning if root get's 30% more allocated than it would get on equal terms - shoot get's that absolute amount less
	scale=s/p;
}
std::string RelativeCarbonAllocation2ShootScaledGrowth::getName()const{
	return "relativeCarbonAllocation2ShootScaledGrowth";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2ShootScaledGrowth(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2ShootScaledGrowth(pSD);
}

RelativeCarbonAllocation2RootsOneMinusShoot::RelativeCarbonAllocation2RootsOneMinusShoot(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon required for potential growth
	shootAlloc=pSD->getSibling("relativeCarbonAllocation2Shoot","100%");
}
void RelativeCarbonAllocation2RootsOneMinusShoot::calculate(const Time &t,double &scale){
	shootAlloc->get(t,scale);
	scale=1-scale;
	//if(scale<0) msg::error("RelativeCarbonAllocation2RootsOneMinusShoot: Shoot uses more than available carbon: root growth can not become negative");
}
std::string RelativeCarbonAllocation2RootsOneMinusShoot::getName()const{
	return "relativeCarbonAllocation2RootsOneMinusShoot";
}
DerivativeBase * newInstantiationCarbonRelativeAllocation2RootsOneMinusShoot(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2RootsOneMinusShoot(pSD);
}


RelativeCarbonAllocation2StemsOneMinusLeafs::RelativeCarbonAllocation2StemsOneMinusLeafs(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//Total carbon required for potential growth
	leafAlloc=pSD->getSibling("relativeCarbonAllocation2Leafs","100%");
}
void RelativeCarbonAllocation2StemsOneMinusLeafs::calculate(const Time &t,double &scale){
	leafAlloc->get(t,scale);
	scale=1-scale;
	if(scale<0) {
		scale=0;
		if(scale<-1e4) msg::warning("RelativeCarbonAllocation2StemsOneMinusLeafs: Leafs use more than available carbon: stem growthrate becomes negative");
	}
}
std::string RelativeCarbonAllocation2StemsOneMinusLeafs::getName()const{
	return "relativeCarbonAllocation2StemsOneMinusLeafs";
}
DerivativeBase * newInstantiationRelativeCarbonAllocation2StemsOneMinusLeafs(SimulaDynamic* const pSD){
   return new RelativeCarbonAllocation2StemsOneMinusLeafs(pSD);
}

//CARBON ALLOCATION TO THE SHOOT IN G/DAY
CarbonAllocation2Shoot::CarbonAllocation2Shoot(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//Total carbon available (g/day)
	Ctotal=pSD->getSibling("carbonAvailableForGrowth","g");
	//carbon allocation simulator (1-root)
	c2shootSimulator=pSD->getSibling("relativeCarbonAllocation2Shoot","100%");
}
void CarbonAllocation2Shoot::calculate(const Time &t,double &scale){
	//get total carbon available (g/day)
	Ctotal->getRate(t,scale);

	//multiply with the carbon allocation to shoot factor
	double a;
	c2shootSimulator->get(t,a);
	scale*=a;
	if(scale<0){
		scale=0;
		//if(nmbpred==pSD->getPredictorSize())
		msg::error("CarbonAllocation2Shoot: allocation < 0, carbon production lower than respiration?");
	}
}
std::string CarbonAllocation2Shoot::getName()const{
	return "carbonAllocation2Shoot";
}
DerivativeBase * newInstantiationCarbonAllocation2Shoot(SimulaDynamic* const pSD){
   return new CarbonAllocation2Shoot(pSD);
}

//CARBON ALLOCATION TO THE LEAFS IN G/DAY
CarbonAllocation2Leafs::CarbonAllocation2Leafs(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//Total carbon available (g/day)
	CtotalShoot=pSD->getParent(3)->getChild("carbonAllocation2Shoot","g");
	//carbon allocation simulator
	c2leafsSimulator=pSD->getSibling("relativeCarbonAllocation2Leafs","100%");
}
void CarbonAllocation2Leafs::calculate(const Time &t,double &scale){
	//get total carbon available (g/day)
	CtotalShoot->getRate(t,scale);

	//multiple with the carbon allocation to leafs factor
	double a;
	c2leafsSimulator->get(t,a);
	scale*=a;
	if(scale <0 && scale>-1e-4) scale=0;
	if(scale<0) msg::error("CarbonAllocation2Leafs: allocation < 0");
}
std::string CarbonAllocation2Leafs::getName()const{
	return "carbonAllocation2Leafs";
}
DerivativeBase * newInstantiationCarbonAllocation2Leafs(SimulaDynamic* const pSD){
   return new CarbonAllocation2Leafs(pSD);
}

CarbonAllocation2Stems::CarbonAllocation2Stems(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//Total carbon available (g/day)
	CtotalShoot=pSD->getParent()->getParent()->getSibling("carbonAllocation2Shoot","g");
	//carbon allocation simulator
	c2stemsSimulator=pSD->getSibling("relativeCarbonAllocation2Stems","100%");
}
void CarbonAllocation2Stems::calculate(const Time &t,double &scale){
	//get total carbon available (g/day)
	CtotalShoot->getRate(t,scale);
	//multiple with the carbon allocation to Stems factor
	double a;
	c2stemsSimulator->get(t,a);
	scale*=a;
	if(scale <0 ) scale=0;
}
std::string CarbonAllocation2Stems::getName()const{
	return "carbonAllocation2Stems";
}
DerivativeBase * newInstantiationCarbonAllocation2Stems(SimulaDynamic* const pSD){
   return new CarbonAllocation2Stems(pSD);
}


//CARBON ALLOCATION TO THE ROOTS IN G/DAY
CarbonAllocation2Roots::CarbonAllocation2Roots(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//Total carbon available (g/day)
	Ctotal=pSD->getSibling("carbonAvailableForGrowth","g");
	//carbon allocation simulator
	c2rootsSimulator=pSD->getSibling("relativeCarbonAllocation2Roots","100%");
}
void CarbonAllocation2Roots::calculate(const Time &t,double &scale){
	//get total carbon available (g/day)
	Ctotal->getRate(t,scale);
	//multiple with the carbon allocation to root factor
	double a;
	c2rootsSimulator->get(t,a);
	scale*=a;
	if(scale<0){
		if (scale<-1e-4) msg::warning("CarbonAllocation2Roots: allocation < 0");
		scale=0;
	}
}
std::string CarbonAllocation2Roots::getName()const{
	return "carbonAllocation2Roots";
}
DerivativeBase * newInstantiationCarbonAllocation2Roots(SimulaDynamic* const pSD){
   return new CarbonAllocation2Roots(pSD);
}



//Scaling factor for root growth (0-1). All roots are scaled equally
RootGrowthScalingFactor::RootGrowthScalingFactor(SimulaDynamic* pSD):DerivativeBase(pSD),mt(12),scale_(1)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("100%");
	//potential growth of the rootsystem in g/day
	potentialRootGrowthSimulator=pSD->getSibling("rootPotentialCarbonSinkForGrowth","g");
	potentialRootGrowthSimulatorMajorAxis=pSD->existingSibling("rootPotentialCarbonSinkForGrowth;majorAxis","g");
	//set mode
	if(pSD->getName()=="secondaryRootGrowthScalingFactor"){
		mode=2;
	}else{
		mode=1;
	}
	//carbon allocation 2 roots simulator in g/day
	c2rootsSimulator=pSD->getSibling("carbonAllocation2Roots","g");
	//pSD->preferedTimeStep()=0.05;
	//for now subtract carbon for secondary growth
	c2SecondaryGrowth=pSD->existingSibling("rootSecondaryPotentialCarbonSinkForGrowth","g");
	if (mode==2 && !c2SecondaryGrowth) msg::error("rootGrowthScalingFactor, can't calculate the scaling factor for secondary growth when the rootSecondaryPotentialCarbonSinkForGrowth is missing.");

	//plantingTime
	plantingTime=pSD->getStartTime();
	//plant type
	std::string plantType;
	pSD->getSibling("plantType")->get(plantType);
	//plant parameters
	SimulaBase *parameters(GETPLANTPARAMETERS(plantType));
	//threshold parameter
	threshold=parameters->getChild("resources")->existingChild("maxCarbonAllocation2SecondaryGrowth","100%");
	//separate scaling factor for major axis
	if(potentialRootGrowthSimulatorMajorAxis && pSD->getName()=="rootGrowthScalingFactor;majorAxis")	mode=3;
	//set initial values
	SimulaVariable* v(dynamic_cast<SimulaVariable* >(pSD));
	if(v){
		v->setInitialValue(1);
		v->setInitialRate(0);
		rates=true;
	}else{
		rates=false;
	}

	//reserves
	reserves=pSD->getSibling("reserves","g");
	//time after which seed reserves have expired
	/// UNUSED
	// SimulaBase* param(GETRESOURCEPARAMETERS(plantType));
	// SimulaBase *p(param->existingChild("seedReserveDuration","day"));
}
void RootGrowthScalingFactor::calculate(const Time &t,double &scale){
	std::size_t predsize=pSD->getPredictorSize();
	if(t-pSD->getStartTime()<1) {
		if(rates){
			scale=0;
			scale_=1;
		}else{
			scale=1;
		}
		return;
	}
	//get potential root growth of the rootsystem in g/day
	double p(1);
	potentialRootGrowthSimulator->getRate(t,p);

	//compare rate to reserves (accuracy hack, for lack of estimates)
	///@todo note that this assumes reserves allocate enough for potential growth
	///@todo assumes timestep smaller than 1
	double r;
	reserves->get(t,r);
	if(std::isnan(r)) {
		reserves->get(t,r);
		msg::error("RootGrowthScalingFactor: reserves is NaN");
	}
	if(r> p && t-pSD->getStartTime()<mt){
		if(rates){
			scale=0;
			scale_=1;
		}else{
			scale=1;
		}
		pSD->clearPredictions(predsize);
		return;
	}

	//get carbon allocation to the roots
	double a;
	c2rootsSimulator->getRate(t,a);
	if(fabs(a-p)<p*5e-3) {//allocation equals potential
		if(rates){
			scale=0;
			scale_=1;
		}else{
			scale=1;
		}
		pSD->clearPredictions(predsize);
		return;
	}

	//carbon needed for secondary growth
	double s(0),sp(0);
	if(c2SecondaryGrowth && a>0){
		c2SecondaryGrowth->getRate(t,sp);
		s=sp;
		//make sure secondary growth is not larger than threshold
		double thrh(1.0);//don't change this a backward compatibility default
		if(threshold)threshold->get(t-plantingTime,thrh);
		s/=a;
		if(s>thrh){
			s=thrh;
			msg::warning("RootGrowthScaling Factor: Not enough carbon for secondary growth. Using threshold from input files to limit secondary growth");
		}
		s*=a;
	}

	//secondary axis.
	//preferences secondary axis: see borch et al., 1999 (bean) and french paper on maize
	double ma(0),mp(0);
	if(mode!=2){
		//Subtract allocation for secondary growth
		a-=s;
		p-=s;
		//subtract carbon needed for major axis
		if(potentialRootGrowthSimulatorMajorAxis && p>0 ){
			potentialRootGrowthSimulatorMajorAxis->getRate(t,mp);
			if(mp>p) mp=p;//should not happen but just in case there are some round of errors.
			//see if available carbon does not match sink
			if(a<p*0.9995){
			//shortage of carbon
				//sink fine roots
				double fp((p-mp)/p);
				//carbon shortage, decrease ma
				double thrsh(1-(fp*0.5));
				if(mp<thrsh*a){
					//give preference to major axis
					ma=mp;
				}else{
					ma=thrsh*a;
				}
			}else if(a>p*1.0005){
			//carbon extra increase ma
				ma=mp*a/p;
			}else{
				ma=mp;
			}
		}
	}


	//calculate scale
	switch (mode){
	case 1:
		//scaling factor for primary growth
		//subtract growth of major axis
		a-=ma;
		p-=mp;
		break;
	case 2:
		//scaling factor for secondary growth
		//copy values to p and a
		a = s;
		p = sp;
		break;
	case 3:
		//scaling factor for primary growth of major axis.
		a = ma;
		p = mp;
		break;
	default:
		msg::error("RootGrowthScalingFactor: non-implemented mode");
		break;
	}

	//avoid divide by zero
	if(fabs (p)<=1e-6){
		scale=1;
		if( fabs(a)>1e-6) //predsize==pSD->getPredictorSize() &&
			msg::error("RootGrowthScalingFactor: carbon allocated but no potential growth");
	}else{
		scale = a/p;
	}

	//some checks and warnings
	if(std::isnan(scale)) msg::error("RootGrowthScalingFactor: scale is NaN");
	//if(scale>10 && !scale.estimate)msg::error("RootGrowthScalingFactor: scale>10, much more than potential growth");
	if(scale<0)	{
		//if(predsize==pSD->getPredictorSize())
			msg::error("RootGrowthScalingFactor: scale<0, negative growth");
		scale=0;
	}

	if(scale>1.1 && predsize==pSD->getPredictorSize()){
		//std::cout<<std::endl<<"scale="<<scale<<"--l";
		msg::warning("RootGrowthScalingFactor: result is greater than 1");
	}

	//speed up hack. This does cause some inaccuracies, but they usually stay below 1 percent. on the otherhand it makes the model much faster.
	pSD->clearPredictions(predsize);
	//msg::warning("turned off speed hack in root growth scaling factor");

	if(rates){
		scale_=scale;
		scale=0; //assume no change, fix it in post integration step
	}
}
bool RootGrowthScalingFactor::postIntegrationCorrection(SimulaVariable::Table & data){
	bool r(false);
	//iterators
	SimulaVariable::Table::iterator eit(data.end()); --eit;
	SimulaVariable::Table::iterator pit(eit); --pit;
	Time dt(eit->first-pit->first);
	double rate((scale_-pit->second.state)/dt);
	if(pit->second.rate!=rate){
		pit->second.rate=rate;
		eit->second.rate=rate;
		r=true;
	}
	return r;
}
std::string RootGrowthScalingFactor::getName()const{
	return "rootGrowthScalingFactor";
}

DerivativeBase * newInstantiationRootGrowthScalingFactor(SimulaDynamic* const pSD){
   return new RootGrowthScalingFactor(pSD);
}

RemainingProportion::RemainingProportion(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	std::string path;
	pSD->getChild("link")->get(path);
	s=pSD->getPath(path);
	pSD->checkUnit(s->getUnit().name);
}
void RemainingProportion::calculate(const Time &t,double &scale){
	s->get(t,scale);
	scale=1-scale;
}
std::string RemainingProportion::getName()const{
	return "remainingProportion";
}
DerivativeBase * newInstantiationRemainingProportion(SimulaDynamic* const pSD){
   return new RemainingProportion(pSD);
}

//registration of classes
class AutoRegisterCarbonAllocation{
public:
   AutoRegisterCarbonAllocation() {
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2RootsFromInputFile"] = newInstantiationCarbonRelativeAllocation2RootsFromInputFile;
   		///@todo work on leafs/stems/reproductive tissu - but this partitioning should take place at root level.
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2ShootFromInputFile"] = newInstantiationCarbonRelativeAllocation2ShootFromInputFile;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2LeafsFromInputFile"] = newInstantiationCarbonRelativeAllocation2LeafsFromInputFile;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2RootsPotentialGrowth"] = newInstantiationCarbonRelativeAllocation2RootsPotentialGrowth;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2ShootPotentialGrowth"] = newInstantiationCarbonRelativeAllocation2ShootPotentialGrowth;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2ShootSwitch"] = newInstantiationCarbonRelativeAllocation2ShootSwitch;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2RootsScaledGrowth"] = newInstantiationCarbonRelativeAllocation2RootsScaledGrowth;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2RootsOneMinusShoot"] = newInstantiationCarbonRelativeAllocation2RootsOneMinusShoot;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2StemsOneMinusLeafs"] = newInstantiationRelativeCarbonAllocation2StemsOneMinusLeafs;
   		BaseClassesMap::getDerivativeBaseClasses()["relativeCarbonAllocation2ShootScaledGrowth"] = newInstantiationCarbonRelativeAllocation2ShootScaledGrowth;
		BaseClassesMap::getDerivativeBaseClasses()["carbonAllocation2Roots"] = newInstantiationCarbonAllocation2Roots;
		BaseClassesMap::getDerivativeBaseClasses()["carbonAllocation2Shoot"] = newInstantiationCarbonAllocation2Shoot;
		BaseClassesMap::getDerivativeBaseClasses()["carbonAllocation2Leafs"] = newInstantiationCarbonAllocation2Leafs;
		BaseClassesMap::getDerivativeBaseClasses()["carbonAllocation2Stems"] = newInstantiationCarbonAllocation2Stems;
		BaseClassesMap::getDerivativeBaseClasses()["rootGrowthScalingFactor"] = newInstantiationRootGrowthScalingFactor;
		BaseClassesMap::getDerivativeBaseClasses()["remainingProportion"] = newInstantiationRemainingProportion;
   }
} static p45684389138547374;
