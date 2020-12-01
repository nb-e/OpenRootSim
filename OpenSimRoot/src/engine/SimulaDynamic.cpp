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

#include "SimulaDynamic2.hpp"
#include "BaseClasses.hpp"
#include "../cli/Messages.hpp"


SimulaDynamic::SimulaDynamic(SimulaBase* const newAttributeOf, const std::string &functionName, const Unit &newUnit, const Time &newstartTime):
	SimulaBase(newAttributeOf,newUnit,newstartTime),
	rateCalculator_(nullptr),instantiateRateCalculator_(nullptr),collectGarbage_(true),multiplier_(nullptr)

{
    setRateFunction(functionName);
}
SimulaDynamic::SimulaDynamic(const std::string &newName, SimulaBase* newAttributeOf, const std::string &functionName, const Unit &newUnit, const Time &newStartTime):
	SimulaBase(newName, newAttributeOf, newUnit, newStartTime),
	rateCalculator_(nullptr),instantiateRateCalculator_(nullptr),collectGarbage_(true),multiplier_(nullptr)
{
    setRateFunction(functionName);
}
SimulaDynamic::SimulaDynamic(const std::string &newName, SimulaBase* newAttributeOf, const std::string &functionName, const Unit &newUnit, const Time &newStartTime, const Time &newEndTime):
	SimulaBase(newName, newAttributeOf, newUnit, newStartTime, newEndTime),
	rateCalculator_(nullptr),instantiateRateCalculator_(nullptr),collectGarbage_(true),multiplier_(nullptr)
{
    setRateFunction(functionName);
}
SimulaDynamic::SimulaDynamic(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaDynamic* copyThis):
	SimulaBase(newAttributeOf, newstartTime, copyThis),
	rateCalculator_(nullptr),instantiateRateCalculator_(copyThis->instantiateRateCalculator_),collectGarbage_(copyThis->collectGarbage_),multiplier_(nullptr)
{
}

SimulaDynamic::~SimulaDynamic(){
	if(rateCalculator_) delete rateCalculator_;
}
void SimulaDynamic::garbageCollectionOff(){
	collectGarbage_=false;
}
void SimulaDynamic::garbageCollectionOn(){
	collectGarbage_=true;
}

SimulaBase* SimulaDynamic::followChain(const Time & t){
	getRateCalculator();
	if(getRateCalculator()) {
		return getRateCalculator()->getNext(t);
	}else{
		msg::error("SimulaDynamic::followChain: no getRate function set");
		return nullptr;
	}
}
void SimulaDynamic::setRateFunction(const std::string &nameRF){
	//first delete any old ratefunction
	if(rateCalculator_) delete rateCalculator_;
	if(nameRF=="doNothing"){
		instantiateRateCalculator_= nullptr;
	}else{
		std::map<std::string, derivativeBaseInstantiationFunction >::iterator it(BaseClassesMap::getDerivativeBaseClasses().find(nameRF));
		if(it==BaseClassesMap::getDerivativeBaseClasses().end()){
			std::string err("SimulaVariable::setRateFunction: ratefunction "+nameRF+" not registered.");
			msg::error(err);
		}else{
			instantiateRateCalculator_= (it->second);
		};
	}
}
void SimulaDynamic::getTimeStepInfo(Time &st,Time &rt)const{
	msg::error("SimulaDynamic: timestep info not available.");
};

DerivativeBase* SimulaDynamic::getRateCalculator(){
	if(!rateCalculator_){
		if(instantiateRateCalculator_){
			rateCalculator_=(instantiateRateCalculator_)(this);
			multiplier_=existingChild("multiplier");
		}
	}
	return rateCalculator_;
}

void SimulaDynamic::databaseSignalingHook(SimulaBase * newObject){
	this->getRateCalculator()->addObject(newObject);
}


void SimulaDynamic::setInitialValue(const double &var){
	msg::error("SimulaDynamic: setInitialValue not implemented for object "+this->getType());
}

/*void SimulaDynamic::set(const Time &t, const double &var){
	msg::error("SimulaDynamic: setValue not implemented for object "+this->getType());
}*/

void SimulaDynamic::avoidPredictorCorrectedLoops(){
	;//do nothing just a place holder
}

SimulaBase* SimulaDynamic::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      return new SimulaDynamic(attributeOf,startTime,this);
}

std::string SimulaDynamic::getRateFunctionName(){
	std::string rname;
	if(rateCalculator_) {
		rname=rateCalculator_->getName();
	}else if(instantiateRateCalculator_){
		for(auto &it : BaseClassesMap::getDerivativeBaseClasses() ){
			if(it.second==instantiateRateCalculator_){
				rname=it.first;
				break;
			}
		}
	}
	return rname;
}

void SimulaDynamic::getXMLtag(Tag &tag){
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
	tag.attributes["function"]=getRateFunctionName();
	if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
	if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
	//tag.attributes["function"]=getRateFunctionName();
	if(!collectGarbage_) tag.attributes["garbageCollectionOff"]="true";
	std::string on(getObjectGeneratorName());
	if(!on.empty()) tag.attributes["objectGenerator"]=on;
	//no data
	//children
	if(getNumberOfChildren()) tag.closed=false;

}




