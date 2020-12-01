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
#include "RootBranching.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaConstant.hpp"
#include "../PlantType.hpp"
#include <math.h>

#if _WIN32 || _WIN64
#include <algorithm>
#endif


//create root branches
///todo if the initiation of these generators is delayed, we could use the constructor instead of an initialize function.
RootBranches::RootBranches(SimulaBase* const pSB):
	ObjectGenerator(pSB),
	parentGrowthpoint(nullptr),
	parentRootLength(nullptr),
	parentRootCreationTime(-1),
	timeLastBranch(pSB->getStartTime()),
	newTime(-1),
	offsetLastBranch(0.0),
	spatialShift(-1),
	aboveGroundAllowed(true),
	maxNumberOfBranches(0),
	allometricScaling(nullptr),
	maximumRootLengthForBranching(0),
	maximumRootAgeForBranching(0),
	lengthRootTip(0),
	branchEmergenceDelay(nullptr),
	branchingFrequency(nullptr),
	branchingFrequencyModifier(nullptr),
	branchingDelay(nullptr),
	branchingDelayModifier(nullptr),
	emergenceDelayForRootsInTheSameWhorl(nullptr),
	branchRootType("notSet"),
	numberOfBranches(nullptr),
	branchList(nullptr),
	latestTime(pSB->getStartTime()),
	relTimeBF(0),
	branchCounter(0)
{}
RootBranches::~RootBranches(){}
void RootBranches::initialize(const Time &t){
	//parent root
	SimulaBase *parentRoot(pSB->getParent()->getParent());
	//parent's root type
	parentRoot->getChild("rootType")->get(parentRootType);
	//parents creation time
	parentRootCreationTime=parentRoot->getStartTime();
	//top of plant
	SimulaBase *top(parentRoot);
	PLANTTOP(top);
	//plant type
	std::string plantType;
	top->getChild("plantType")->get(plantType);
	//pointer to root parameters of the parent
	SimulaBase *parentRootPar(ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild(parentRootType));
	//growthpoint of root's parent root
	parentGrowthpoint=dynamic_cast<SimulaPoint*>(parentRoot->getChild("growthpoint"));
	//parent root length
	parentRootLength=parentGrowthpoint->getChild("rootLongitudinalGrowth","cm");//this is faster than using rootLength which is based on adding up segment length
	//branch's root type
	branchRootType=pSB->getName();
	//branching parameters
	SimulaBase* branchRootPar(parentRootPar->getChild("branchList")->getChild(branchRootType));
	//time after which parent root doesn't create branches (useful for hypocotyl born roots that can not grow aboveground
	SimulaBase* p;
	p=branchRootPar->existingChild("maximumRootLengthForBranching","cm");
	if (p) p->get(t,maximumRootLengthForBranching);
	//rootlength after which parent root doesn't create branches
	p=branchRootPar->existingChild("maximumRootAgeForBranching","day");
	if (p) {
		p->get(t,maximumRootAgeForBranching);
		maximumRootAgeForBranching+=parentRootCreationTime;
	}
	//number of branches/ position (whorl)
	numberOfBranches=branchRootPar->existingChild("numberOfBranches/whorl","#");
	//branching frequency in cm/branch or branches/cm or in tissue age (day or #/day)
	branchingFrequency=branchRootPar->existingChild("branchingFrequency","cm");
	relTimeBF=pSB->getStartTime();
	branchingFrequencyModifier=parentGrowthpoint->existingChild("branchingFrequencyMultiplier");
	if(branchingFrequencyModifier) msg::warning("RootBranches:: using branchingFrequencyModifier of the parent growth point.",1);

	//time delay for branching
	branchingDelay=branchRootPar->existingChild("branchingDelay","day");
	branchingDelayModifier=parentGrowthpoint->existingChild("branchingDelayMultiplier");
	if(branchingDelayModifier) msg::warning("RootBranches:: using branchingDelayModifier of the parent growth point.",1);

	// time delay between roots on the same whorl
	emergenceDelayForRootsInTheSameWhorl = branchRootPar->existingChild("emergenceDelayForRootsInTheSameWhorl", "day");

	//allow branches above ground
	p=branchRootPar->existingChild("allowBranchesToFormAboveGround");
	if(p) p->get(aboveGroundAllowed);
	//branches that will branch from the NEW branch
	branchList=ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild(branchRootType)->getChild("branchList");
	//maximum number of branches
	p=branchRootPar->existingChild("maxNumberOfBranches","#");
	if (p){
		p->get(t,maxNumberOfBranches);
	}else if(!branchingFrequency && numberOfBranches){//only create one whorl
		numberOfBranches->get(parentRootCreationTime,maxNumberOfBranches);
	}
	//allometric scaling
	allometricScaling=branchRootPar->existingChild("allometricScaling");
	if(allometricScaling){
		bool i;
		allometricScaling->get(i);
		if(i) {
			allometricScaling=top->getChild("plantPosition")->getChild("shoot")->existingChild("leafAreaReductionCoefficient");
		}else{
			allometricScaling=nullptr;
		}
	}

	//initial spatial offset
	p=branchRootPar->existingChild("branchingSpatialOffset","cm");
	if(p)p->get(spatialShift);
	if(spatialShift==-1 && !branchingFrequency){
		msg::warning("RootBranches: Neither a spatialShift nor a branchingFrequency have been given for "+pSB->getName()+". No roots will be formed!");
	}

	//minimum length between roottip and first branch
	p=branchRootPar->existingChild("lengthRootTip","cm");
	if (p)	p->get(parentRootCreationTime,lengthRootTip);

	//time between the root tip past a section and the emergence of a branch
	branchEmergenceDelay=branchRootPar->existingChild("branchEmergenceDelay","day");


	//initial  new time
	p=branchRootPar->existingChild("branchingTimeOffset","day");
	if (p){
		if(lengthRootTip>0) msg::error("branchingTimeOffset can not be used in combination with lengthRootTip for "+branchRootType+" branching off "+parentRootType);
		p->get(newTime);
		newTime+=parentRootCreationTime;
	}

	//correction if the input files already contain some branches
	p=pSB->getLastChild();
	if(p){
		timeLastBranch=p->getStartTime();
		newTime=-1;
		spatialShift=-1;
		Time gt=0,mt=0,Mt=1e99;
		Coordinate pos;
		p->get(pos);
		parentGrowthpoint->getTime(pos,gt,mt,Mt);
		parentRootLength->get(gt,offsetLastBranch);
		branchCounter=pSB->getNumberOfChildren();//might not be correct if we have more than 1 root per whorl, but I guess a jump in the numbers does not matter.
	}
}


void RootBranches::getTimeShift(const Time &t, const double & spatialShift, Time &timeShift){//timeshift is absolute time of the new branch
	if (branchingDelay) {
		branchingDelay->get(timeLastBranch-parentRootCreationTime,timeShift);
		//stress modifier
		if(branchingDelayModifier){
			double del;
			branchingDelayModifier->get(timeLastBranch-parentRootCreationTime,del);
			timeShift*=del;
		}
		//avoid negative times
		if(timeShift<0) timeShift*=-1;
		//new time
		timeShift+=timeLastBranch;
	}else if( branchEmergenceDelay ){
		//determine the time the parent root was old enough to fit this branch
		Time bed;
		branchEmergenceDelay->get(t-parentRootCreationTime,bed);//bed is the age the branch minimally has to have
		//determine how old the parent root was when it was passing the new position
		//note that we are assuming here that the parent root has grown enough, which is quaranteed by the spatial shift
		if(bed+parentRootCreationTime>t){
			timeShift=-1;
			return;
		}
		if(timeLastBranch-bed<parentRootCreationTime)timeLastBranch=bed+parentRootCreationTime;
		double l;
		parentRootLength->get(t-bed,l);
		if(l>spatialShift){
			double l;
			parentRootLength->get(timeLastBranch-bed,l);
			if(l<spatialShift){
				//we can safely call getTime
				timeShift=timeLastBranch+MAXTIMESTEP;//best estimate;
				parentRootLength->getTime(spatialShift,timeShift,timeLastBranch-bed,t-bed);
				timeShift+=bed;
			}else{
				timeShift=timeLastBranch;//let lengthRootTip or some other rule take care of it
			}
		}else{
			timeShift=-1;
			return;
		}

		//determine the time that the parent root was long enough to fit this branch
		if(lengthRootTip>0){
			if(timeShift<t){
				double l;
				parentRootLength->get(timeShift,l);
				if(spatialShift+lengthRootTip>l){
					//branch should be created even later, to quarantee that the (slow) parent root has at least grown the required length
					//this could result in a timeShift which is before timeLastBranch and thereby trigger an error exception?
					parentRootLength->getTime(lengthRootTip+spatialShift,timeShift,timeLastBranch,t);//time lastBranch as initial second point.
				}
			}/*else{
			//not really necessary this else, as the condition is already taken care of in the spatialRules. This is not run if these conditions are not met
				parentRootLength->get(t,l);
				if(spatialShift+lengthRootTip>l){
					//cannot determine the length of the root tip yet redo everything the next time round
					timeShift=-1;
					msg::error("RootBranches::This code should not run and is an indication of some inconsistency in spatial rules");
				}
			}*/
		}
	}else if(lengthRootTip>0){
		//time based on when the root was spatialShift+lengthRootTip long
		double ss(spatialShift+lengthRootTip);
		timeShift=(t+timeLastBranch)/2;//halfway as best estimate
		parentRootLength->getTime(ss,timeShift,timeLastBranch,t);//time lastBranch as initial second point.
	}else{
		//root length when last branch was created
		double l;
		parentRootLength->get(timeLastBranch,l);//position of parent root tip at creation of the last branch
		//time it took for the parent root to grow to l+spatialShift- offsetLastBranch
		timeShift=t;//t as best estimate
		parentRootLength->getTime(l+spatialShift-offsetLastBranch,timeShift,timeLastBranch,t);//time lastBranch as intial second point.
	}
}
void RootBranches::getSpatialShift(const Time &t,double &spatialShift){
	//branching frequency in cm
	if(branchingFrequency){
		//branchingfrequency should be determined at the time the root tip of the parent root was growing through the location where the branch was created
		//note that this causes quite different branching behaviour from before, when gpatposition was timeLastBranch and thus much later.
		//this causes significantly more roots in the lupin system compared to the old (2011) executable on which lupin was parametrized initially
		Time gpatposition(timeLastBranch);
		parentRootLength->getTime(offsetLastBranch,gpatposition,timeLastBranch-4,timeLastBranch);
		//msg::warning("RootBranches:: turned of the new time shift for branching freq.");
		branchingFrequency->get(gpatposition-relTimeBF,spatialShift);
		//environmental modifier of the branching frequency.
		if(branchingFrequencyModifier){
			double mod=0;
			branchingFrequencyModifier->get(gpatposition,mod);
			spatialShift*=mod;
			//std::cout<<std::endl<<"mod="<<mod<<"======="<<std::endl;
			if(mod<0.01) msg::error("RootBranches: branchingFrequencyMultiplier is causing a very high branching frequency.");
		}

		//since with random distributions the spatialShift can be negative - we should check
		if(spatialShift<0) {
			spatialShift*=-1;
			msg::warning("Root branches: Negative branching frequency or negative spatial shift, multiplying with -1, but check the limits on your branching frequency");
		}
	}else{
		spatialShift=1e8;
		pSB->stopUpdatefunction();
	}


	//new spatial shift from base of the parent root
	spatialShift+=offsetLastBranch;
}

bool RootBranches::checkSpatialRules(const double & spatialShift, const Time & t){
	//stop branching when maximum number of branches has been reached
	if (maxNumberOfBranches && maxNumberOfBranches<=pSB->getNumberOfChildren()){
		//all branches have been created: set latest time in database to a large value, so that this update function doesn't get called (this is a computational saving measure)
		pSB->stopUpdatefunction();
		return true;
	}

	//stop branching if distance2base is beyond set limit
	if(maximumRootLengthForBranching && maximumRootLengthForBranching<spatialShift) {
		//all branches have been created: stop update function doesn't get called
		pSB->stopUpdatefunction();
		//break from loop
		return true;
	}

	//check that the branch actually fits
	double l;
	parentRootLength->get(t,l);
	if(spatialShift+lengthRootTip>l) return true;
	if(lengthRootTip<=0 && !branchingDelay && !branchEmergenceDelay){
		double k;
		parentRootLength->get(timeLastBranch,k);
		if(k<offsetLastBranch){
			//this can occur when branchingTimeOffset is used in combination with branching spatialOffset, and the time offset is less than growth based on spatial offset
			msg::error("RootBranches: no space on the root for the branches. Is branchingTimeOffset too short to allow for branchingSpatialOffset for "+branchRootType+" branching off "+parentRootType+"?");
		}
		/*  l is the current length of the root
		 *  k is the length the root had when the last branch was created
		 *  spatialShift-offsetLastBranch is the distance between the last and the new branch
		 *
		 *  the rule here is that the branch is only created if the growth or the parent root is greater than the distance between the new and the last branch.
		 *
		 */
		if(spatialShift-offsetLastBranch>l-k) {
			msg::warning("RootBranches::checkSpatialRules: Branches are too close to the root tip. Please specify a length of the root tip or a branching emergence delay for "+pSB->getName()+" branching of "+pSB->getParent(2)->getName());
			return true;
		}
	}

	return false;
}
bool RootBranches::checkTimeRules(Time & newTime, const Time & t){
	//check model consistency and correct if necessary.
	if(newTime<0){
		//branchtime could not be determined yet as the parent root is not old enough
		return true;
	}
	Time d(latestTime-newTime);
	if(d>0){
		if(d>0.04)
			msg::warning("RootBrances: fixing TimeShift error for "+branchRootType+" - check your branching rules. This may also happen if the growth rate of the parent root is increasing over time.");
		newTime=latestTime;
	}

	if(newTime > t) {
		//the branch will be formed at a time that is later than t. Delay the formation of the branch to next time round.
		return true;
	}


	//break if this is beyond the maximum root age for branching
	if(maximumRootAgeForBranching>parentRootCreationTime && maximumRootAgeForBranching<newTime){
		//all branches have been created: set latest time in database to a large value, so that this update function doesn't get called (this is a computational saving measure)
		pSB->stopUpdatefunction();
		//stop creating branches
		return true;
	}

	//sanity checks
	if(newTime<timeLastBranch)
		msg::error("RootBranching: branches are not formed in order, there is a bug in the code");

	return false;
}


void RootBranches::generateWhorl(const Coordinate &newPosition, const Time &newTime){
	//number of branches/whorl (or position) (default is 1)
	int numberOfBranchesPerPosition=1;
	if (numberOfBranches!=nullptr)	numberOfBranches->get(timeLastBranch-parentRootCreationTime,numberOfBranchesPerPosition);

	//check number of branches. it may not exceed the maximum number of branches
	if (maxNumberOfBranches){
		numberOfBranchesPerPosition=std::min(numberOfBranchesPerPosition, maxNumberOfBranches - (signed int)pSB->getNumberOfChildren());
	}

	//reduce number of branches allometrically
	// TODO:
	//	this calculation gets repeated in RootGrowthDirection::getRadialBranchingAngle.
	// 	Should refactor to only need it in one place, but meanwhile if you edit here
	//  please edit there too.
	if(allometricScaling){
		double m;
		allometricScaling->get(newTime,m);
		numberOfBranchesPerPosition=(int)ceil(m*(double)numberOfBranchesPerPosition);
	}

	//create branches
	for(int i=1;i<=numberOfBranchesPerPosition;i++){
		if (emergenceDelayForRootsInTheSameWhorl){
			double tt = 0;
			emergenceDelayForRootsInTheSameWhorl->get(i, tt);
			if (tt < newTime){
				msg::warning("RootBranches: Roots can not emerge before whorl. Check your inputs");
				tt = 0;
			}
			generateRoots(i, newPosition, newTime + tt);
		}
		else generateRoots(i, newPosition, newTime);
	}

	//increment the counter for branches
	++branchCounter;
}
void RootBranches::generateRoots(const int &rootNumber, const Coordinate &newPosition, const Time &newTime){
	//name of new branch
	std::string temp("00000" + std::to_string(branchCounter));
	temp.erase(0, temp.length()-6);
	std::string nameNewBranch("000" + std::to_string(rootNumber));
	nameNewBranch.erase(0, nameNewBranch.length()-4);
	nameNewBranch = branchRootType+"-"+temp+"-"+nameNewBranch;

	//create new branch by copying it from the template
	SimulaBase* pNewBranch= new SimulaConstant<Coordinate>(nameNewBranch, pSB, newPosition, newTime);
	pNewBranch->setFunctionPointer("generateRoot");
}
void RootBranches::generate(const Time &t){
	//return if we are not beyond the last branching time
	if(t<=parentRootCreationTime || t<=timeLastBranch || t<=latestTime) return;

	//do a small error correction for branches that are based on a branching frequency in cm
	if(branchingFrequency && lengthRootTip>0 && offsetLastBranch>0 ){
		//time based on when the root was spatialShift+lengthRootTip long
		double k;
		parentRootLength->get(timeLastBranch,k);
		//fix small errors in spatial shift due to the fact that the previous branch was positioned based on estimated data
		//here we put small errors in timeLastBranch into the offsetlastBranch in order to avoid errors on branches that were not created last time, but could have fitted.
		//these error typically occur when the growth rate of the parent root is increasing, and add up of many branches finally giving an error message.
		offsetLastBranch=std::max(0.,k-lengthRootTip);//max, as nmr2simroot does not have the right lengthRootTip.
	}
	//============CREATE AS MANY BRANCHES AS BRANCHING CONDITION PERMITS=======//
	while(true){

		//calculate spatial offset from base of the parent Root and the new time.
		if(spatialShift==-1)
			getSpatialShift(t,spatialShift);
		if(checkSpatialRules(spatialShift,t))
			break;
		if(newTime==-1)
			getTimeShift(t,spatialShift,newTime);
		if(checkTimeRules(newTime,t))
			break;
		//recheck the spatial rules with the newTime
		//TODO following line would be more correct check - but it seems to largely cause problems with
		//timeshift which is not always that accurate.
		//if(checkSpatialRules(spatialShift,newTime)) msg::error("RootBranches: timing and spatial rules form an incompatible combination");

		//get the time at which the parent root was spatialShift long
		Time tl(newTime);//newTime as best initial estimate
		parentRootLength->getTime(spatialShift,tl,parentRootCreationTime,t);//time lastBranch as intial second point.
		//get the position of the parent root at that time
		///@todo this assumes that the position did not change since than
		Coordinate newPosition,newAbsolutePosition;
		parentGrowthpoint->get(tl,newPosition);
		///@todo in case newTime is based on spatial shift, tl is a better estimate for newTime.

		//generate the branches at the new position and new time
		parentGrowthpoint->getAbsolute(tl,newAbsolutePosition);
		if(newAbsolutePosition.y<-0.5 || aboveGroundAllowed) generateWhorl(newPosition,newTime);

		//store last position and last branching time
		timeLastBranch=newTime;
		offsetLastBranch=spatialShift;

		//reset
		spatialShift=-1;
		newTime=-1;
	}
	pSB->updateRecursively(t);
	latestTime=t;
}


//the maker function for instantiation of the class
ObjectGenerator * newInstantiationRootBranches(SimulaBase* const pSB){
   return new RootBranches(pSB);
}


InsertRootBranchContainers::InsertRootBranchContainers(SimulaBase* const pSB):
	ObjectGenerator(pSB){}

InsertRootBranchContainers::~InsertRootBranchContainers(){}

void InsertRootBranchContainers::initialize(const Time &t){
	//top of plant
	SimulaBase *top(pSB);
	PLANTTOP(top);
	//plant type
	std::string plantType, branchRootType;
	SimulaBase * p(top->existingChild("plantType"));
	if(p) {
		p->get(plantType);
	}else{
		pSB->stopUpdatefunction();
		return;
	}
	//branch's root type
	pSB->getSibling("rootType")->get(branchRootType);
	//branches that will branch from the NEW branch
	SimulaBase * branchList=ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild(branchRootType)->existingChild("branchList");
	if(branchList && branchList->hasChildren()){
		std::string subBranchType;
		SimulaBase::List l;
		branchList->getAllChildren(l);
		for(auto & it:l){
			subBranchType=(it)->getName();
			if(!pSB->existingChild(subBranchType)){
				SimulaBase* pSubBranch=new SimulaBase(subBranchType, pSB , UnitRegistry::noUnit(), pSB->getStartTime());

				std::string fn="rootBranches";
				SimulaBase* p=(it)->existingChild("objectGeneratorClassToUse");
				if(p) p->get(fn);
				pSubBranch->setFunctionPointer(fn);
				//copy standard root children from the root template
				//this inserts parameters at the level of the container that holds all branches of a type that is inside laterals in here: /laterals/lateral1-1/siblingrootemplate
				//SimulaBase * branchingTemplate=ORIGIN->existingChild("branchingTemplate");
				//if(branchingTemplate) pSubBranch->copyAttributes(newTime, branchingTemplate);
			}
		}
	}

	pSB->stopUpdatefunction();
}
void InsertRootBranchContainers::generate(const Time &t){
	//do nothing
}

ObjectGenerator * newInstantiationInsertRootBranchContainers(SimulaBase* const pSB){
   return new InsertRootBranchContainers(pSB);
}

///code for generating new roots, using the template

GenerateRoot::GenerateRoot(SimulaBase* const pSB):
	ObjectGenerator(pSB){}

GenerateRoot::~GenerateRoot(){}

void GenerateRoot::initialize(const Time &t){
	//plant and root
	std::string plantType;
	PLANTTYPE(plantType,pSB);
	std::string rootType;

	//insert rootType attribute of new branch
	if(!pSB->existingChild("rootType")) new SimulaConstant<std::string>	("rootType",pSB,pSB->getParent()->getName(),UnitRegistry::noUnit(), pSB->getStartTime());
	pSB->getChild("rootType")->get(rootType);

	//root type specific template
	SimulaBase* p=ORIGIN->existingChild(rootType+"Template");
	if(p){
		msg::warning("GenerateRoot: using root class specific template for "+rootType, 1);
	}else{
		p=ORIGIN->getChild("siblingRootTemplate");
	}

	//copy standard root children from the root template
	pSB->copyAttributes(pSB->getStartTime(), p);

	//Insert growthRateMultiplier
	p=(GETROOTPARAMETERS(plantType,rootType))->existingChild("longitudinalGrowthRateMultiplier");
	double multiplier(1.);
	if(p){
		Time parentAge(pSB->getStartTime()-pSB->getParent(3)->getStartTime());
		p->get(parentAge,multiplier);
	};
	new SimulaConstant<double>	("growthRateMultiplier",pSB,multiplier,UnitRegistry::noUnit(), pSB->getStartTime());

	//set function pointer in branches
	p=pSB->existingChild("branches");
	if(p) {
		p->setFunctionPointer("insertRootBranchContainers");
		int n=p->getNumberOfChildren(t);//will run the inserRootBranchContainers, and return number of inserted root classes
		if(n>0) {
			//p=pSB->getChild("growthpoint");
			//p->garbageCollectionOff(); normally set in the templates, maybe it should be overruled?
		}else{
			//enable garbage collection
			p=pSB->getChild("growthpoint");
			p->garbageCollectionOn();
		}
	}

	pSB->stopUpdatefunction();
}
void GenerateRoot::generate(const Time &t){
	//do nothing
}

ObjectGenerator * newInstantiationGenerateRoot(SimulaBase* const pSB){
   return new GenerateRoot(pSB);
}



class AutoRegisterRootBranchingInstantiationFunctions {
public:
   AutoRegisterRootBranchingInstantiationFunctions() {
      // register the maker with the factory
	  BaseClassesMap::getObjectGeneratorClasses()["rootBranches"] = newInstantiationRootBranches;
	  BaseClassesMap::getObjectGeneratorClasses()["insertRootBranchContainers"] = newInstantiationInsertRootBranchContainers;
	  BaseClassesMap::getObjectGeneratorClasses()["generateRoot"] = newInstantiationGenerateRoot;
   }
};

static AutoRegisterRootBranchingInstantiationFunctions p;
