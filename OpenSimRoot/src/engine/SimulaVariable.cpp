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

#include "SimulaVariable.hpp"
#include "BaseClasses.hpp"
#include "../math/InterpolationLibrary.hpp"
#include "../math/MathLibrary.hpp"
#include "../tools/StringExtensions.hpp"
#if _WIN32 || _WIN64
#include <algorithm>
#endif

SimulaVariable::SimulaVariable(const std::string &newName, SimulaBase* newAttributeOf, const std::string &rateF, const std::string &integrationF, const Unit &newUnit, const Time &newstartTime, const Time &newEndTime):
	SimulaTimeDriven(newName, newAttributeOf, rateF, newUnit, newstartTime, newEndTime)
{
	//set integration function
	setIntegrationFunction(integrationF);
	//set starting time and state
	table[newstartTime]=Colum2();
}

SimulaVariable::SimulaVariable(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaVariable* copyThis):
	SimulaTimeDriven(newAttributeOf, newstartTime, copyThis)
{
	//set starting time and state
	table[newstartTime]=copyThis->table.begin()->second;
	//generate a new movement calculator (don't just copy pointers)
	setIntegrationFunction(copyThis->integrationF->getName());
}

SimulaVariable::~SimulaVariable(){
}

void SimulaVariable::getRate(const Time &t, double &pd){
	double p(0);
	//is this a call back?
	if(callBack){
		if((t-table.rbegin()->first)<TIMEERROR){
			//interpolate
			p=interpolate<Colum1,Colum2,Table>(t,table).rate;
			if(t>t_solid_) addToPredictorList(this);
		}else{
			if(callBack==-1){//comes from getRate with only 1 entry
				msg::warning("SimulaVariable::getRate: estimating initial rate for "+getName()+". This maybe a poor estimate",1);
				p=0;
				addToPredictorList(this);
			}else{
				//use predictor
				addToPredictorList(this);
				integrationF->predictRate(t,p);
			}
		}
	}else{
		//forward simulation
		integrate_(t,table);
		if(table.size()==1){//we never did a rate calculation so do it now
			//make sure we initiated so that initial values are set (in case time=startTime)
			///@todo this is not save when postIntegration method is called
			//todo problem here is that the wall time is not set, and so the model goes everywhere.
			bool pcb=callBack;
			callingOrder.push_back(this);
			callBack=-1;
			getRateCalculator()->calculate(t,p);
			keepEstimate(t);
			callBack=pcb;
			callingOrder.pop_back();
			//we store this so in case it is not an estimate integration can keep it
			table.rbegin()->second.rate=p;
		}
		if(t<=table.rbegin()->first-TIMEERROR){
			//interpolate
			p=interpolate<Colum1,Colum2>(t,table).rate;
		}else{
			p=table.rbegin()->second.rate;
			if(t>table.rbegin()->first+TIMEERROR){
				//integrate_ refused to move forward in time beyond the newtime of the first called integration routine - simple make a prediction
				//use predictor
				addToPredictorList(this);
			}
		}
	}
	if(multiplier_) {
		double m;
		multiplier_->get(t,m);
		p*=m;
	}
	pd=p;
	if(std::isnan(pd)) {
		msg::error("SimulaVariable::getRate: Rate is NaN");
	}

}

void SimulaVariable::get(const Time &t, double &pd){
	//is this a call back?
	double p;
	if(callBack){
		if(t<table.rbegin()->first+TIMEERROR){
			p=interpolate<Colum1,Colum2>(t,table).state;
			if(t>t_solid_) addToPredictorList(this);
		}else{
			if(callBack==-1){//comes from getRate with only 1 entry
				p=table.rbegin()->second.state;
			}else{
				//use predictor
				addToPredictorList(this);
				integrationF->predict(t,p);
			}
		}
	}else{
		//forward simulation
		//todo calling here integrate_ inherited from SimulaTimeDriven
		integrate_(t,table);

		if(t<table.rbegin()->first-TIMEERROR){
			p=interpolate<Colum1,Colum2>(t,table).state;
		}else{
			if(t<table.rbegin()->first+TIMEERROR){
				p=table.rbegin()->second.state;
			}else{
				//integrate_ refused to move forward in time beyond the newtime of the first called integration routine - simple make a prediction
				//use predictor
				typename Table::reverse_iterator it(table.rbegin());
				if(table.size()>2){
					typename Table::reverse_iterator pit(it);++pit;
					linearInterpolation(pit->first,it->first,t,pit->second.state,it->second.state,p);
					//avoid sign change
					if(it->second.state*pit->second.state>0. && p*it->second.state<0.) p=0.;
				}else{
					p=it->second.state;
				}
				addToPredictorList(this);
			}
		}
	}
	pd=p;
}

///@todo this 'optimization algorithm' may not be completely robust when data is not approximately on a linear increase or decrease
#include <iomanip>
void SimulaVariable::getTime(const double & var, Time & t, Time tmin, Time tmax){
	//never collect garbage
	if(collectGarbage_) msg::error("SimulaVariable::getTime: historical data needed but garbage collection is on. Please turn it off for "+ getName());
	//assume that t is intelligent guess (seed) but check if it is within boundaries
	if(!evaluateTime(tmin)) tmin=startTime;
	//TODO could use midpoint between tmin and tmax if they are both within bounds
	if(!evaluateTime(t)) msg::error("SimulaVariable::getTime: seed outside time boundaries of object");
	if(!evaluateTime(tmax)) tmax=t;

	//solve this triangular
	double rl,rs,ru;//rl is lower bound, ru is upper bound rs is midpoint estimate
	Time tl(tmin),ts(t),tu(tmax);//idem
	if(tl>tu) std::swap(tl,tu);
	get(tl,rl);//get value at lower timepoint
	get(tu,ru);//get value at later timepoint


	//if necessary swap times and values so that rl<ru
	if(rl>ru){
		std::swap(tl,tu);
		std::swap(rl,ru);
		//check if boundaries are too strict
		if(var > ru) {
			tu=getStartTime();
			get(tu,ru);
		}
	}else{
		//check if boundaries are too strict
		if(var < rl) {
			tl=getStartTime();
			get(tl,rl);
			if(var<rl) msg::error("SimulaVariable::getTime: value below minimum.");
		}
	}

	//best estimate for ts
	if(rl!=ru){
	  linearInterpolation(rl,ru,var,tl,tu,ts);
	}else{
 		ts=(tl+tu)/2;
 	}

	//Debug line std::cout<<std::endl<<"getTime:"<<std::setprecision(6);
	while(true){
		//Debug line std::cout<<std::endl<<rl<<"\t"<<var<<"\t"<<ru<<"\t"<<tl<<"\t"<<ts<<"\t"<<tu;
		if(var > ru || var < rl)		{
			msg::error("SimulaVariable::getTime: target is out of bounds for "+getPath());
		}
		if(tu-tl<=3*TIMEERROR){//must be 3, note that within one timeerror, the problem is discrete and will not solve.
			t=(tl+tu)/2;
			break;
		}
		get(ts,rs);//get the new value for the new estimated point
		if(rs > ru || rs < rl) {
			msg::error("SimulaVariable::getTime: more than one solution for "+getPath());
		}
		double err((var-rs)*(tu-tl)/(ru-rl));//error estimate in terms of time
		if(err<-TIMEERROR){
			//solution between rl and rs
			ru=rs;tu=ts;
			linearInterpolation(rl,ru,var,tl,tu,ts);
			//ts-=TIMEERROR;//this only helps if the problem is nice linear
			if(!withinBounds(tl,tu,ts)) ts=(tl+tu)/2;
		}else if(err>TIMEERROR){
			//solution between rs and ru
			rl=rs;tl=ts;
			linearInterpolation(rl,ru,var,tl,tu,ts);
			//ts+=TIMEERROR;//this only helps if the problem is nice linear
			if(!withinBounds(tl,tu,ts)) ts=(tl+tu)/2;
		}else{
			//solved
			t=ts;
			break;
		}
	}
}

Time SimulaVariable::lastTimeStep(){
	if(table.size()>1){
		Table::reverse_iterator it(table.rbegin()), pit(it);++pit;
		return it->first-pit->first;
	}else{
		return preferedT;
	}
}

void SimulaVariable::setInitialValue(const double &var){
	//check if table size
	if(table.size()<2){
		//overwrite first value
		table[startTime]=Colum2(var,double(0));
		//if(table.size()!=1) msg::error("SimulaVariable::setInitialValue: Can't change the initial value after simulation has already started");
	}else{
		msg::warning("SimulaVariable::setInitialValue: ignoring initial value setting, as the table already contains data");
	}
}

void SimulaVariable::set(const Time &t, const Colum2 &var){
	//check estimate, and see if time should be moved
	keepEstimate(t);
	//check time is beyond
	if(t<t_solid_) 	msg::error("SimulaVariable: inconsistent time.");
	//insert into table
	table[t]=var;
	t_solid_=t;//assume further integration is not needed.
}

void SimulaVariable::setInitialRate(const double &var){
	//overwrite first value
	table[startTime].rate=var;
	//check if table size
	if(table.size()!=1) msg::warning("SimulaVariable::setInitialValue: Changing the initial value after simulation has already started",2);
}

std::string SimulaVariable::getType()const{
	return "SimulaVariable";
}

/*void SimulaVariable::garbageCollectionOff(){
	collectGarbage_=false;
	table.keepHistory();
}
void SimulaVariable::garbageCollectionOn(){
	collectGarbage_=true;
	table.deleteHistory();
}*/

void SimulaVariable::collectGarbage(const Time & t){
	if(collectGarbage_ && table.size()>3){
		auto it=table.end();
		--it;--it;--it;
		//check time
		if(t < it->first){
			//find iterator that points to
			table.erase(table.begin(),table.lower_bound(t));
		}else{
			--it;
			table.erase(table.begin(),it);
		}
	}
	//allow SimulaBase to clean up as well.
	SimulaBase::collectGarbage(t);
}

std::ostream &operator<<(std::ostream &os, const SimulaVariable &obj){
	//for xml output, this is not complete, so it could be read back.
	//<<std::endl<<"<SimulaVariable name=\""<<name_<<"\" integrationFunction=\""<<integrationF->getName()<<"\" function=\""<<rateCalculator->getName()<<"\"/>";
	return os;
}

SimulaBase* SimulaVariable::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      return new SimulaVariable(attributeOf,startTime,this);
}

void SimulaVariable::getXMLtag(Tag &tag){
	//name
	tag.name="SimulaVariable";
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
	if(!table.empty()){
		std::string d;
		for(auto &dp:table){
			d+=convertToString<Colum1>(dp.first);
			d+="\t";
			d+=convertToString<Colum2>(dp.second);
			d+="\n";
		}
		tag.attributes["_DATA_"]=d;
		tag.closed=false;
	}
	//children
	if(getNumberOfChildren()) tag.closed=false;

}
