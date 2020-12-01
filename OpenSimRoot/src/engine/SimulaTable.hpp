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

#ifndef SIMULATABLE_HPP_
#define SIMULATABLE_HPP_
#include <typeinfo>
#include "SimulaBase.hpp"
#include "SimulaDynamic.hpp"
#include <string>
#include <map>
#include "DataDefinitions/Units.hpp"
#include "../cli/Messages.hpp"
#include "../tools/StringExtensions.hpp"
#include "../math/InterpolationLibrary.hpp"
#include "../math/MathLibrary.hpp"


template <class T=Time>
class SimulaTable:public SimulaDynamic {
public:
	typedef double Colum1;
	typedef T Colum2;
	typedef std::map<Colum1,Colum2> Table;

	SimulaTable(SimulaBase* newAttributeOf, const Time &newstartTime):
		SimulaDynamic(newAttributeOf, "doNothing", UnitRegistry::noUnit(), newstartTime),
		//set name_ of the first colum
		nameColum1("NA"),
		//set units of the first colum
		unitColum1(UnitRegistry::noUnit()),
		//multiplier_
		useMultiplier(true),
		interpolationMethod(TableInterpolation::linear),
		garbageCollected(-1)
	{
		garbageCollectionOff();
	}
	SimulaTable(const std::string &newNameColum1, SimulaBase* newAttributeOf, const Unit &newUnitColum1, const std::string &newNameColum2, const Unit &newUnitColum2, const Time &newstartTime):
		SimulaDynamic(newNameColum2, newAttributeOf, "doNothing", newUnitColum2, newstartTime),
		//set name_ of the first colum
		nameColum1(newNameColum1),
		//set units of first colum
		unitColum1(newUnitColum1),
		//multiplier_
		useMultiplier(true),
		interpolationMethod(TableInterpolation::linear),
		garbageCollected(-1)
	{
		garbageCollectionOff();
	}
	SimulaTable(const std::string &newNameColum1, SimulaBase* newAttributeOf, const Unit &newUnitColum1, const std::string &newNameColum2, const Unit &newUnitColum2, const Time &newstartTime, const Time &newEndTime):
		SimulaDynamic(newNameColum2, newAttributeOf, "doNothing", newUnitColum2, newstartTime, newEndTime),
		//set name_ of the first colum
		nameColum1(newNameColum1),
		//set units of first colum
		unitColum1(newUnitColum1),
		//multiplier_
		useMultiplier(true),
		interpolationMethod(TableInterpolation::linear),
		garbageCollected(-1)
	{
		garbageCollectionOff();
	}

	SimulaTable(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaTable* copyThis):
		SimulaDynamic(newAttributeOf, newstartTime, copyThis),
		//set name_ of the first colum
		nameColum1(copyThis->nameColum1),
		//set units of the first colum
		unitColum1(copyThis->unitColum1),
		//multiplier_
		useMultiplier(true),
		interpolationMethod(copyThis->interpolationMethod),
		garbageCollected(-1),
		table(copyThis->table)
	{
		//turn garbage collector on for copied time tables
		if(unitColum1=="day") {
			collectGarbage_=true;
		}
		//collectGarbage_=false; //turned this off, for it keeps causing problems.
	}

	void get(const Colum1 &xin, Colum2 &y){
		//conversion from time to degrees days, input is double and assumed to be time since start of simulation and time conversion should only be local!
		Colum1 x=xin;
		const auto rc = getRateCalculator();

		if(rc){
			SimulaTimeDriven::callingOrder.push_back(this);
			if(table.empty()){
				rc->calculate(startTime,y);
				if(table.empty()){
					//rc->calculate(startTime,y);
					msg::error("SimulaTable: Table '"+getPath()+"' is empty. Name of table writing function is "+rc->getName());
				}
			}
			if((garbageCollected<startTime && table.begin()->first>x)|| (x-table.rbegin()->first>TIMEERROR)) rc->calculate(x,y); //only if table is small to avoid conflicts with the garbage collector
			SimulaTimeDriven::callingOrder.pop_back();
		}else{
			if(table.empty()) msg::error("SimulaTable: Table '"+getPath()+"' is empty.");
		}


		///@todo just linear intepolation for now. later switch box with more advanced interpolation methods or something the like
		///@todo time error and min timestep used here to round off and check boundaries, but not all tables are based on time!
		if(table.begin()->first>x-TIMEERROR){
			// x is at or before the start of the table
			if(table.begin()->first>x+TIMEERROR){
				msg::warning("SimulaTable:get: extrapolation requested in table: "+getName()+". Requested value: "+std::to_string(x)+". Min value: "+std::to_string(table.begin()->first)+". Max value: "+std::to_string(table.rbegin()->first)+". StartTime: "+std::to_string(getStartTime())+". Table size: "+std::to_string(table.size()));
			}
			y=table.begin()->second;
		}else if(table.rbegin()->first<x-TIMEERROR){
			//extrapolate: x is at or beyond the end of the table
			typename Table::reverse_iterator it(table.rbegin());
			if(table.size()>1){
				typename Table::reverse_iterator pit(it);++pit;
				if(interpolationMethod==TableInterpolation::linear){
					linearInterpolation(pit->first,it->first,x,pit->second,it->second,y);
					//avoid sign chance
					if(it->second*pit->second>0. && y*it->second<0.) y=0.;
				}else{
					y=it->second;
				}
			}else{
				y=it->second;
			}
			predictorOrder.push_back(this); //note that this is important
			if(table.rbegin()->first<x-MAXTIMESTEP){
				msg::warning("SimulaTable:: Extrapolation at end of table "+getName());
			}
		}else{
			if(interpolationMethod==TableInterpolation::linear){
				y=interpolate<Colum1,Colum2>(x,table);
			}else{
				Colum1 xt=TIMEERROR*round(x/TIMEERROR);
				typename Table::iterator it(table.upper_bound(xt));
				--it;
				y=it->second;
			}
		}
		if(useMultiplier){
			if(!multiplier_)
				multiplier_=this->existingChild("multiplier");
			if(multiplier_) {
				Colum2 m;
				multiplier_->get(m);
				y*=m;
			}else{
				useMultiplier=false;
			}
		}
	}
	void getRate(const Colum1 &t, Colum2 &var){
		//estimate it using finite difference
		//note that if the finite difference is too small we may get, because of numerical errors, 0
		Colum1 d1(MINTIMESTEP);
		Colum1 t1(t-d1),t2(t+d1);///@todo this difference is hard coded, since we use it for time tables only, but with anything else another delta should be better
		Colum1 tb(table.begin()->first);
		Colum2 r0,r1;
		if(t1<tb && t>=tb) t1=tb;
		get(t1,r0);

		//not really necessary, as we extrapolate anyway.
		//tb=table.rbegin()->first;
		//if(t2>tb && t<=tb && table.size()>1 ) t2=tb;
		get(t2,r1);
		var=(r1-r0)/(t2-t1);
	}
	void get(const Time &t, const Coordinate & pos, Colum2 &y){
		//use t if column1 is time, but pos.y if column1 is state
		if(getUnitColum1()=="day"){
			get(t,y);
		}else if(getUnitColum1()=="degreesC.day"){
			get(t,y);
		}else if(getUnitColum1()=="cm"){
			get(pos.y,y);
		}else{
			msg::error("SimulaTable::get(time,coordinate,y) cannot reduce dimensionality for units of column1 of table \""+getPath()+"\" are not cm or day but "+getUnitColum1().name);
		}
	}
	void getRate(const Time &t, const Coordinate & pos, Colum2 &y){
		if(getUnitColum1()=="day"){
			getRate(t,y);
		}else if(getUnitColum1()=="degreesC.day"){
			getRate(t,y);
		}else if(getUnitColum1()=="cm"){
			get(pos.y,y);
		}else{
			msg::error("SimulaTable::getRate(time,coordinate,y) cannot reduce dimensionality for units of column1 of table \""+getPath()+"\" are not cm or day but "+getUnitColum1().name);
		}
	}

	unsigned int size(){
		return table.size();
	}

	void set(const Colum1 &x, const Colum2 &y){
		table[x]=y;
	}
///todo this could be abstracted into simulabase, since all other objects assume column1 is time
	Unit getUnitColum1()const{
		return unitColum1;
	}
	void setUnitColum1(const Unit &newUnit){
		unitColum1=newUnit;
	}
	Unit getUnitColum2(){
		return getUnit();
	}
	bool setStartTime()const{
		if(table.empty()){
			return true;
		}else{
			return (table.begin()->first>startTime && garbageCollected<0);
		}
	}
	void setUnitColum2(const Unit &newUnit){
		setUnit(newUnit);
	}
	void setInterpolationMethod(const std::string &method){
		for(auto const & im : TableInterpolationNames){
			if (im.second==method) {
				interpolationMethod=im.first;
				return;
			}
		}
		msg::error("SimulaTable: unknown interpolation method '"+method+"' for '"+getPath()+"'. Use linear or step.");
	}

	std::string getNameColum1()const{
		return nameColum1;
	}
	void setNameColum1(const std::string &newName){
		nameColum1=newName;
	}
	std::string getType()const{
		std::string type="SimulaTable<";
		if(typeid(T)==typeid(double)){
			type+="double";
		}else{
			type+="unknown";
		}
		type+=">";
		return type;
	}

	virtual void collectGarbage(const Time& t){
		if(collectGarbage_ && table.size()>3 && t>garbageCollected){
			//check time
			//Time s=fmin(table.rbegin()->first,t);
			//make sure we keep at least 3 timesteps
			auto it=table.end();
			------it;
			//int so=table.size();
			if(t < it->first){
				//find iterator that points to
				table.erase(table.begin(),table.lower_bound(t));
			}else{
				--it;
				table.erase(table.begin(),it);
			}
			garbageCollected=t;
		}
		//allow SimulaBase to clean up as well.
		SimulaBase::collectGarbage(t);
	}

	virtual void getXMLtag(Tag &tag){
		//name
		tag.name="SimulaTable";
		tag.closed=true;
		//attributes
		tag.attributes.clear();
		tag.attributes["name_column1"]=getNameColum1();
		tag.attributes["name_column2"]=getName();
		tag.attributes["prettyName"]=getPrettyName();
		tag.attributes["unit_column1"]=getUnitColum1().name;
		tag.attributes["unit_column2"]=getUnit().name;
		if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
		if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
		std::string fn=getRateFunctionName();
		if(!fn.empty()) tag.attributes["function"]=fn;
		// look up interpolation method name
		auto im=TableInterpolationNames.find(interpolationMethod);
		if(im!=TableInterpolationNames.end()){
			tag.attributes["interpolationMethod"]=im->second;
		} else {
			// If we get here, it means some values in TableInterpolation
			// are missing from TableInterpolationNames
			msg::error("SimulaGrid: don't known how to write XML tag for interpolation method '"
				+ std::to_string(static_cast<int>(interpolationMethod))
				+ "' in '" + getName()
				+ "'. Please file a bug at https://gitlab.com/rootmodels/OpenSimRoot.");
		}
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

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
	      return new SimulaTable<T>(attributeOf,startTime,this);
	};


protected:
	std::string nameColum1;
	Unit unitColum1;
	bool useMultiplier;
	TableInterpolation interpolationMethod;
	Time garbageCollected;
	Table table;
};




#endif /*SIMULATABLE_HPP_*/
