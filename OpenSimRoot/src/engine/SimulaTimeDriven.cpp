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

#include "SimulaTimeDriven.hpp"
#include "BaseClasses.hpp"
#include "../cli/Messages.hpp"

/**
 * CallingOrder is a list which keeps track what SimulaTimeDriven::get calls are in the stack.
 * It is compared to the predictor order. If an object relies on a prediction below it self, it
 * will redo it's timestep, otherwise not.
 *
 * CallingOrder is also used to limit the timestep, so that we do not have extrapolations in the predictor.
 *
 */
SimulaBase::PointerList SimulaTimeDriven::callingOrder;

SimulaTimeDriven::SimulaTimeDriven(SimulaBase* const newAttributeOf, const std::string &functionName, const Unit &newUnit, const Time &newstartTime):
	SimulaDynamic(newAttributeOf, functionName,newUnit,newstartTime),
	minT(MINTIMESTEP),
	maxT(MAXTIMESTEP),
	preferedT(PREFEREDTIMESTEP),
	predictorPosition(0),
//	syncTimeStep_(false),
	integrationF(nullptr),
	callBack(false),
	dependLock_(nullptr),
	forwardOnly(false),
	t_solid_(startTime)
{}
SimulaTimeDriven::SimulaTimeDriven(const std::string &newName, SimulaBase* newAttributeOf, const std::string &functionName, const Unit &newUnit, const Time &newStartTime):
	SimulaDynamic(newName, newAttributeOf, functionName, newUnit, newStartTime),
	minT(MINTIMESTEP),
	maxT(MAXTIMESTEP),
	preferedT(PREFEREDTIMESTEP),
	predictorPosition(0),
	//	syncTimeStep_(false),
	integrationF(nullptr),
	callBack(false),
	dependLock_(nullptr),
	forwardOnly(false),
	t_solid_(startTime)
{}
SimulaTimeDriven::SimulaTimeDriven(const std::string &newName, SimulaBase* newAttributeOf, const std::string &functionName, const Unit &newUnit, const Time &newStartTime, const Time &newEndTime):
	SimulaDynamic(newName, newAttributeOf, functionName, newUnit, newStartTime, newEndTime),
	minT(MINTIMESTEP),
	maxT(MAXTIMESTEP),
	preferedT(PREFEREDTIMESTEP),
	predictorPosition(0),
	//syncTimeStep_(false),
	integrationF(nullptr),
	callBack(false),
	dependLock_(nullptr),
	forwardOnly(false),
	t_solid_(startTime)
{}
SimulaTimeDriven::SimulaTimeDriven(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaTimeDriven* copyThis):
	SimulaDynamic(newAttributeOf, newstartTime, copyThis),
	minT(copyThis->minT),
	maxT(copyThis->maxT),
	preferedT(copyThis->preferedT),
	predictorPosition(0),
	//syncTimeStep_(copyThis->syncTimeStep_),
	integrationF(nullptr),
	callBack(false),
	dependLock_(nullptr),
	forwardOnly(false),
	t_solid_(startTime)
{}

SimulaTimeDriven::~SimulaTimeDriven(){
	delete integrationF;
}
//extern std::map<std::string, integrationInstantiationFunction> BaseClassesMap::getIntegrationClasses();
void SimulaTimeDriven::setIntegrationFunction(const std::string &nameIF){
	if(integrationF) delete integrationF;
	std::map<std::string, integrationInstantiationFunction >::iterator it(BaseClassesMap::getIntegrationClasses().find(nameIF));
	if(it==BaseClassesMap::getIntegrationClasses().end()) {
		msg::error("SimulaVariable::setIntegrationFunction: integrationfunction "+nameIF+" not registered.");
	}else{
		integrationF= (it->second)() ;
	}
}
void SimulaTimeDriven::setIntegrationFunctionTolerance(const double tol)const{
	integrationF->setTolerance(tol);
}

std::string SimulaTimeDriven::getIntegrationFunction()const{
	if(integrationF) {
		return integrationF->getName();
	}else{
		return "integration function not set";
	}
}
void SimulaTimeDriven::getTimeStepInfo(Time &st,Time &rt)const{
	integrationF->getTimeStepInfo(st,rt);
}

void SimulaTimeDriven::set(const Time &t, const StateRate &var){
	msg::error("SimulaTimeDriven::set not implemented, check your code");
};
void SimulaTimeDriven::set(const Time &t, const MovingCoordinate &var){
	msg::error("SimulaTimeDriven::set not implemented, check your code");
};

bool SimulaTimeDriven::keepEstimate(const double t, const bool lt){
	bool r=true;
	if(r){
		if (callingOrder.empty() || predictorOrder.empty()){
			r=false;
		} else if ( *(callingOrder.begin()) == this) {
			r=false;
		}else if(predictorOrder.size()<=predictorPosition){
			//msg::warning("SimulaTimeDriven::keepEstimate: estimate is true but no predictor called?");
			r=false;
		}else if (forwardOnly){
			r=false;
			if(!r && !predictorOrder.empty() && predictorPosition<predictorOrder.size())
				predictorOrder.erase(predictorOrder.begin()+predictorPosition,predictorOrder.end());

		}else{
			r=false;
			for (std::size_t j(0); j<(callingOrder.size()-1) ; ++j){
				for (std::size_t k(predictorPosition); k<predictorOrder.size() ; ++k){
					if(callingOrder[j]==predictorOrder[k]){
						r=true;
						if(!ignoreLocks_){
							/*this object depends on a object earlier in the calling list.
							 * Lock it so it does not try again until that object calls keepEstimate() and releases the lock.
							 */
							/* not all SimulaBase classes suport this, so only if setDependent returns succesful*/
							if(callingOrder[j]->setDependent(this)) dependLock_=callingOrder[j];
						}
						break;
					}
				}
				if(r) break;
			}
		}
	}

	releaseDependents();
	if(lt && !r && t_solid_<t) t_solid_=t;

	if(!r && !predictorOrder.empty() && predictorPosition<predictorOrder.size())
		predictorOrder.erase(predictorOrder.begin()+predictorPosition+1,predictorOrder.end());

	return r;
}

void SimulaTimeDriven::avoidPredictorCorrectedLoops(){
	forwardOnly=true;
}

#include <cmath>
void SimulaTimeDriven::integrate_(const Time & t,SimulaVariableTable & data){
///todo the task here and that of SimulaVariable::get is somewhat artificially separated and could be joined into one call?
	callBack=true;
	integrationF->initiate(this,*getRateCalculator());
	
	if( callingOrder.empty() ){
		predictorOrder.clear();
	}

	//set predictorPosition
	predictorPosition=predictorOrder.size();

	//delete estimates
	auto lit(data.upper_bound(t_solid_+TIMEERROR));
	if(!dependLock_){
		data.erase(lit,data.end());
	}else{
		//addToPredictorList(this);
		--lit;
		if(t>lit->first+TIMEERROR) addToPredictorList(dependLock_);
	}


	//we need to get the time here to where the first object is integrating to, as we do not want to do additional timesteps.
	//this time is t2 in the predictor of the integrationF.
	//this is only safty, normally the timestepping module will avoid that t is beyond rt
	Time rt=getWallTime(t); //uses calling order, and so as to not include itself, better call before adding self.

	callingOrder.push_back(this);
	while(rt>data.rbegin()->first+TIMEERROR){//simulation needs to be forwarded in time, beyond the the current time to get the rate
		integrationF->integrate(data,*getRateCalculator());
		releaseDependents();
	}

	callBack=false;
	callingOrder.pop_back();
}
void SimulaTimeDriven::integrate_(const Time & t, SimulaPointTable & data){
	callBack=true;
	integrationF->initiate(this,*getRateCalculator());

	if( callingOrder.empty() ){
		predictorOrder.clear();
	}

	//set predictorPosition
	predictorPosition=getPredictorSize();

	//delete estimates
	auto lit(data.upper_bound(t_solid_+TIMEERROR));
	if(!dependLock_){
		data.erase(lit,data.end());
	}else{
		--lit;
		if(t>lit->first+TIMEERROR) addToPredictorList(dependLock_);
	}

	callingOrder.push_back(this);

	//this is only safty, normally the timestepping module will avoid that t is beyond rt
    Time rt=getWallTime(t);
	while((rt-data.rbegin()->first)>TIMEERROR){//simulation needs to be forwarded in time, beyond the the current time to get the rate
		integrationF->integrate(data,*getRateCalculator());
		//many roots have duplicate entries when they are not growing. we do not know for how long they are not growing, but the entries can be deleted.
		if(data.size()>3){
			auto it1=--data.end(), it2(--it1), it3(--it2);
			if(it1->second==it3->second && it2->second==it3->second){
				//remove it2
				data.erase(it2);
			}
		}
		releaseDependents();//this is simply a list so that dependent objects do not trail behind.
	}
	callBack=false;
	callingOrder.pop_back();
}

/**
 * this returns the smallest time step in the current calling stack, which should be the last time step
 *
 */
Time SimulaTimeDriven::getWallTime(const Time &t){
	///todo this is only a global wall time, and means that objects can still step beyond a dependent object below them in the tree. We could set this to rbegin forcing all objects to use a small timestep, but that would not be efficient. That is, we need to know the dependencies to do better here.
	Time rt(t),st,et;
	if(!callingOrder.empty()){
		for(auto it=callingOrder.rbegin(); it!=callingOrder.rend(); ++it){
			SimulaTimeDriven* fco=dynamic_cast<SimulaTimeDriven*>(*it);
			if(fco){
				fco->getTimeStepInfo(st,et);
				if(et>0. && et<rt)rt=et;
				break;
			}
		}
	}
	//also get from the object generator the wall time
	ObjectGenerator::getWallTime(rt);///todo getWallTime() does not have a standard call or interpretation..
	return rt;
}

/**
 * this returns the largest time step in the current calling stack, which should be the first time step
 * this is used to make sure we do not update the tree beyond rt.
 */
void SimulaTimeDriven::getGlobalTimeStepInfo(Time &st,Time &rt){
	st=-1;rt=-1;
	//loop is necessary, as table of type simulaDynamic also pushes to callingOrder, but should be ignored.
	for(auto it=callingOrder.begin(); it!=callingOrder.end(); ++it){
		SimulaTimeDriven* fco=dynamic_cast<SimulaTimeDriven*>(*it);
		if(fco){
			fco->getTimeStepInfo(st,rt);
			break;
		}
	}
}

Time &SimulaTimeDriven::minTimeStep(){return minT;}
Time &SimulaTimeDriven::maxTimeStep(){return maxT;}
Time &SimulaTimeDriven::preferedTimeStep(){return preferedT;}
/*bool &SimulaTimeDriven::syncTimeStep(){
	return false;//syncTimeStep_;
}*/
///@todo since all SimulaTimeDriven objects have a table last timestep could be checked/calculated?
Time SimulaTimeDriven::lastTimeStep(){
	msg::error("lastTimeStep: not implemented for this class of type "+getType());
	return 0;
}

bool SimulaTimeDriven::setDependent(SimulaBase* me){
	dependentList.insert(me);
	return true;
}
void SimulaTimeDriven::unLockDependency(){
	dependLock_=nullptr;
}
/* each SimulaTimeDriven object has a list of unique pointers (set) of objects which depend on it.
 * These are objects which have its dependLock_ set.
 *
 */
void SimulaTimeDriven::releaseDependents(){
	for(UniquePointerList::iterator it(dependentList.begin());it!=dependentList.end();++it ){
		(*it)->unLockDependency();
	}
	dependentList.clear();
}
std::size_t SimulaTimeDriven::numberOfDependents()const{
	return dependentList.size();
}

bool SimulaTimeDriven::ignoreLocks_(false);
void SimulaTimeDriven::ignoreLocks(const bool &b){
	ignoreLocks_=b;
}

SimulaBase* SimulaTimeDriven::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      return new SimulaTimeDriven(attributeOf,startTime,this);
}

void SimulaTimeDriven::getXMLtag(Tag &tag){
	std::string n(getType()),t("");
	std::size_t b1(n.find('<')),b2(n.size()-(b1+2));
	if(b1!=std::string::npos){
		t=" type=\""+n.substr(b1+1,b2)+"\" ";
		n=n.substr(0,b1);
	}
	//name
	tag.name=n;
	tag.closed=true;
	//attributes
	tag.attributes.clear();
	tag.attributes["name"]=getName();
	tag.attributes["prettyName"]=getPrettyName();
	tag.attributes["unit"]=getUnit().name;
	if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
	if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
	tag.attributes["function"]=getRateFunctionName();
	if(integrationF) tag.attributes["integrationFunction"]=integrationF->getName();
	if(!collectGarbage_) tag.attributes["garbageCollectionOff"]="true";
	std::string on(getObjectGeneratorName());
	if(!on.empty()) tag.attributes["objectGenerator"]=on;
	//data
	msg::warning("SimulaTimeDriven::dumpXML: dumpXML not implemented for "+n+"Using generic implementation which may be incomplete");
	//children
	if(getNumberOfChildren()) tag.closed=false;

}
