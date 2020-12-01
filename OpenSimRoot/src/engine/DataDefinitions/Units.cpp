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
#include "Units.hpp"
#include <string>
#include "../../cli/Messages.hpp"
#include  <memory>

//using namespace units;

//Class Unit constructors
Unit::Unit(){
	order="NA";
	name="noUnit";
	factor=1;
}
Unit::Unit(const char * sc){
	std::string s(sc);
	*this=Unit(s);
}
Unit::Unit(const std::string &s){
	Unit tr=UnitRegistry::getUnit(s);
	name=tr.name;
	factor=tr.factor;
	order=tr.order;
}

Unit::Unit(const std::string &nOrder, const std::string &newName, const double &newFactor){
	order=nOrder;
	name=newName;
	factor=newFactor;
}
Unit::Unit(const Unit &copyThis){
	order=copyThis.order;
	name=copyThis.name;
	factor=copyThis.factor;
}
bool Unit::operator== (const Unit &u)const{
	//match based on names.
	return (u.name==this->name);
}
bool Unit::operator!= (const Unit &u)const{
	//match based on names.
	return (u.name!=this->name);
}
bool Unit::operator< (const Unit &u)const{
	//match based on names.
	return (this->name<u.name);
}
bool Unit::operator> (const Unit &u)const{
	//match based on names.
	return (this->name>u.name);
}

void Unit::operator= (const Unit &u){
	this->order=u.order;
	this->name=u.name;
	this->factor=u.factor;
}
Unit Unit::operator/ (const Unit &u)const{
	Unit r;
	r.order="composite";
	r.name=this->name+"/"+u.name;
	r.factor=this->factor/u.factor;
	//TODO: composite simplification (example kg/kg->kg or kg/m/m2-> kg*m or m/day/m->m2/day )
	return r;
}
Unit Unit::operator* (const Unit &u)const{
	Unit r;
	r.order="composite";
	r.name=this->name+"."+u.name;
	r.factor=this->factor*u.factor;
	//TODO: composite simplification (example kg/kg->kg or kg/m/m2-> kg*m or m/day/m->m2/day )
	return r;
}
void Unit::operator/= (const Unit &u){
	order="composite";
	name+="/"+u.name;
	factor/=u.factor;
	//TODO: composite simplification (example kg/kg->kg or kg/m/m2-> kg*m or m/day/m->m2/day )
}
void Unit::operator*= (const Unit &u){
	order="composite";
	name+="."+u.name;
	factor*=u.factor;
	//TODO: composite simplification (example kg/kg->kg or kg/m/m2-> kg*m or m/day/m->m2/day )
}
///todo decomposition of the unit is now in the registry, and should really be done in one place
std::string Unit::element(unsigned int i)const{
	std::string r;
	unsigned int count(1);
	for (std::string::const_iterator it=name.begin(); it!=name.end() ; ++it){
		if(*it=='/' || *it=='*' || *it=='.'|| *it==' ') {
			++count;
		}else if (count==i){
			r+= *it;
		}else{
			continue;
		};
	}
	if (r.empty()) {
		r="element ";
		r+=std::to_string(i)+" in name "+name+" not found";
	}
	return r;
}

void Unit::check(const Unit &check,const std::string &parameterName,const std::string &moduleName)const{
	if(*this!=check)msg::error("Unit::check: "+moduleName+" wants '"+parameterName+"' in "+check.name+"' but it has unit '"+name);
}

//unit reading functions


//writing functions
std::ostream &operator<<(std::ostream &os, const Unit &obj){
	//os<<obj.formattedName();
	os<<obj.name;
	return os;
}


//unit conversion
double Unit::getUnitConversionFactor(const Unit &target)const{
		///@todo composite units need to have the order better specified
		if (target.order!=this->order) msg::error("Unit::getUnitConversionFactor: Can't convert '"+this->name+"' to '"+target.name+"' as they are of different order.");
		if (target.order=="composite") msg::error("Unit::getUnitConversionFactor: Can't convert composite units yet.");
		double factor= this->factor/target.factor;
		if(factor<0) msg::error("Unit:::getUnitConversiontFactor: factor unknown for units "+this->name+" into "+target.name);
		return factor;
}

std::string Unit::formattedName()const{
	std::string r;
	char sign='+';
	bool writeLetter=true;
	int count=0;
	for (std::string::const_iterator it=name.begin(); it!=name.end() ; ++it){
			if(*it=='/'){
				if(writeLetter){
					if(sign=='-'){
						r+='^';
						r+=sign;
						r+='1';
						r+='*';
					}
				}
				r+=' ';
				sign='-';
				writeLetter=true;
				++count;
			}else if(*it=='*') {
				if(writeLetter){
					if(sign=='-'){
						r+='^';
						r+=sign;
						r+='1';
						r+='*';
					}
				}
				r+=' ';
				writeLetter=true;
				++count;
			}else if (isdigit(*it)){
				r+='^';
				if(sign=='-' && writeLetter) {
					r+=sign;
				}
				r+= *it;
				r+='*';
				writeLetter=false;
			}else{
				r+= *it;
			};
	}
	if(count>0 && writeLetter){
		if(sign=='-'){
			r+='^';
			r+=sign;
			r+='1';
			r+='*';
    	}
	}
	auto pos=r.find(UnitRegistry::noUnit().name,0);

	if(pos!=std::string::npos){
		r.erase(pos,UnitRegistry::noUnit().name.size());
	}

	//fix 100%
	if(name=="100%") r=name;

	return r;
}


std::map<int,Unit> UnitRegistry::unitsById_;
std::map<std::string,int> UnitRegistry::idByUnits_;
Unit &UnitRegistry::getUnit(const int & id) {
	if(unitsById_.empty()) UnitRegistry::loadStandardUnits();
	std::map<int,Unit>::iterator it(unitsById_.find(id));
	if(it==unitsById_.end()){
		msg::warning("UnitRegistry: unit not found, returning 0");
		it=unitsById_.begin();
	}
	return it->second;
}
Unit & UnitRegistry::getUnit(const std::string & unit){
	if(unitsById_.empty()) UnitRegistry::loadStandardUnits();
	//see if it is stored
	std::map<std::string,int>::iterator it(idByUnits_.find(unit));
	if(it==idByUnits_.end()){
		//unknown unit, try to construct it
		Unit tr,tu;
		std::size_t spos=0,epos=unit.find_first_of("*/. ",spos);
		char del='b';
		//detect composite strings by / or * -> decompose, find units and compose return unit
		if(epos==std::string::npos){
			//unknown unit
			msg::warning("Unitregistry::getUnit: registering unknown unit with name "+unit+". ");
			tr.name=unit;
			tr.order="unknown";
			tr.factor=-1;
		}else{
			//composite unit, decompose it
			while(spos<unit.size()){
				std::string subs(unit.substr(spos,epos-spos));
				tu=UnitRegistry::getUnit(subs);
				switch (del) {
					case 'b':
						tr=tu;
						break;
					case '*':
						tr*=tu;
						break;
					case '/':
						tr/=tu;
						break;
					case '.':
						tr*=tu;
						break;
					case ' ':
						tr*=tu;
						break;
					default:
						break;
				}

				if(epos!=std::string::npos) del=unit[epos];
				spos=epos+1;
				epos=unit.find_first_of("*/. ",spos);
				if(epos==std::string::npos) epos=unit.size();
			}
		}
		//register the new (possibly composite) unit
		int id((int)1+idByUnits_.size());

		if(tr.name!=unit){
			//msg::warning("wrong decomposition "+unit+" into "+tr.name);
			//decomposition led to change in unit name (for example s -> second) etc.
			//this is not a problem, but we either have to register the other name with the old id
			it=idByUnits_.find(tr.name);
			if(it!=idByUnits_.end()){
				id=it->second;
			}
			idByUnits_[unit]=id;
		}
		//msg::warning(convertToString(id)+"Registering unknown unit "+tr.name+" with id "+convertToString(id));
		unitsById_[id]=tr;
		idByUnits_[tr.name]=id;
		//return the new unit
		return unitsById_[id];
	}else{
		return unitsById_[it->second];
	}
}

int UnitRegistry::getId(const Unit & unit) {
	if(unitsById_.empty()) UnitRegistry::loadStandardUnits();
	//see if it is stored
	std::map<std::string,int>::iterator it(idByUnits_.find(unit.name));
	if(it==idByUnits_.end()){
		//unit not yet registered, register it now
		int id=1+(int)idByUnits_.size();
		//msg::warning("Registering unknown unit "+unit.name+" with id "+convertToString(id));
		unitsById_[id]=unit;
		idByUnits_[unit.name]=id;
		return id;
	}else{
		return it->second;
	}
}

Unit & UnitRegistry::noUnit(){
	return unitsById_[1];
}

UnitRegistry::UnitRegistry(){
	UnitRegistry::loadStandardUnits();
}

void UnitRegistry::loadStandardUnits(){
	if (!unitsById_.empty()) return;
	Unit *u=new Unit();int id(1);unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("NA","noUnit",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("NA","na",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("NA","1",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("weight","kg",1E3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("weight","g",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("weight","mg",1E-3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("weight","ug",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("length","m",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("length","cm",1E-2);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("length","km",1E3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("length","mm",1E-3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("length","um",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("area","m2",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("area","cm2",1E-4);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("area","km2",1E6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("area","mm2",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("volume","m3",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("volume","cm3",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("4D","cm4",1E-8);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("volume","km3",1E9);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("volume","mm3",1E-9);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("volume","l",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("volume","ml",1E-3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("volume","ul",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("count","#",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("concentration","M",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("concentration","mM",1E-3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("concentration","uM",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("time","s",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;
	idByUnits_["second"]=id;delete u;
	u=new Unit("time","minute",60);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("time","hour",3600);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("time","day",86400);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("time","week",604800);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("time","year",31536000);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u; //based on 365 days a year

	u=new Unit("angle","radians",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("angle","degrees",57.295779513);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("amount","mol",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id; delete u;
	u=new Unit("amount","mmol",1E-3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id; delete u;
	u=new Unit("amount","umol",1E-6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;
    idByUnits_["uMol"]=id;delete u;//has typo but is here for backwards compatibility

	u=new Unit("energy","J",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("energy","kJ",1E3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("energy","MJ",1E6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("fraction","%",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("fraction","100%",1E-2);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;
	idByUnits_["percent100"]=id;delete u;

	u=new Unit("pressure","hPa",1E2);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("pressure","Pa",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("pressure","kPa",1e3);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;
	u=new Unit("pressure","MPa",1E6);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;delete u;

	u=new Unit("temperature","degreesC",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;
	idByUnits_["C"]=id;delete u;

	u=new Unit("energypertime","W",1);++id;unitsById_[id]=*u;idByUnits_[u->name]=id;
	idByUnits_["J/s"]=id;delete u;

}



