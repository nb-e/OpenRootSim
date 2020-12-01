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

#ifndef SIMULACONSTANT_HPP_
#define SIMULACONSTANT_HPP_

#include <typeinfo>
#include "SimulaBase.hpp"
#include "DataDefinitions/Units.hpp"
#include "../cli/Messages.hpp"
#include "TypeTraits.hpp"
#include "../tools/StringExtensions.hpp"

template <class T, bool _interpolate=is_interpolatable<T>::value>
class SimulaConstant:public SimulaBase{
public:
	typedef T Type;
	SimulaConstant():
		SimulaBase(),constant(0),useMultiplier(true),multiplier(nullptr)
	{}
	SimulaConstant(const std::string &newName, SimulaBase* const newAttributeOf, const Unit &newUnit, const Time &newstartTime, const Time &newEndTime):
		SimulaBase(newName, newAttributeOf, newUnit, newstartTime),
		constant(0),useMultiplier(true),multiplier(nullptr)
	{}
	SimulaConstant(const std::string &newName, SimulaBase* const newAttributeOf, const Type &newConstant, const Unit &newUnit, const Time &newstartTime):
		SimulaBase(newName, newAttributeOf, newUnit, newstartTime),
		constant(newConstant),useMultiplier(true),multiplier(nullptr)
	{}
	SimulaConstant(SimulaBase* newAttributeOf, const Time &startTime, const SimulaConstant* copyThis):
		SimulaBase(newAttributeOf, startTime, copyThis),
		//set constant
		constant(copyThis->constant),useMultiplier(true),multiplier(nullptr)
	{}
	void get(const Time &t, Type &rc){
		rc=constant;
		if(useMultiplier){
			if(!multiplier) multiplier=this->existingChild("multiplier");
			if(multiplier) {
				Type m;
				multiplier->get(t,m);
				rc*=m;
			}else{
				useMultiplier=false;
			}
		}
	}
	void getRate(const Time &t, double &rc){
		rc=0;
	}
	void get(Type &rc){
		rc=constant;
		if(useMultiplier){
			if(!multiplier) multiplier=this->existingChild("multiplier");
			if(multiplier) {
				Type m;
				multiplier->get(m);
				rc*=m;
			}else{
				useMultiplier=false;
			}
		}
	}
	void get(const Time &t, const Coordinate & pos, Type &y){
		get(y);
	}
	void getRate(const Time &t, const Coordinate & pos, Type &y){
		y=0;
	}
	std::string getType()const{
		std::string type="SimulaConstant<";
		if(typeid(T)==typeid(double)) {
			type+="double";
		}else if(typeid(T)==typeid(int)){
			type+="integer";
		}else if(typeid(T)==typeid(std::string)){
			type+="string";
		}else if(typeid(T)==typeid(Coordinate)){
			type+="Coordinate";
		}else if(typeid(T)==typeid(bool)){
			type+="bool";
		}else{
			msg::error("SimulaContstant declared with unsupported type");\
		}
		type+=">";
		return type;
	}

	void setConstant(const Type &rc){constant=rc;}
	void copy(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaConstant<Type>* copyThis){
		SimulaBase* tmp(this);
		tmp->copy(newAttributeOf,newstartTime,(const SimulaBase*)copyThis);
		constant=(copyThis->constant);
	}

	virtual void getXMLtag(Tag &tag){
		//name
		tag.name="SimulaConstant";
		tag.closed=true;
		//attributes
		tag.attributes.clear();
		tag.attributes["name"]=getName();
		tag.attributes["prettyName"]=getPrettyName();
		tag.attributes["type"]=getType().substr(15,getType().size()-16);
		tag.attributes["unit"]=getUnit().name;
		if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
		if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
		std::string on(getObjectGeneratorName());
		if(!on.empty()) tag.attributes["objectGenerator"]=on;
		//data
		tag.attributes["_DATA_"]=convertToString<Type>(constant);
		tag.closed=false;

	}

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
	      return new SimulaConstant<T>(attributeOf,startTime,this);
	};

	template <class TVar>
	friend std::istream &operator>>(std::istream &is, SimulaConstant<TVar> &obj);

protected:
	Type constant;
	//multiplier
	bool useMultiplier;
	SimulaBase* multiplier;

};

template <class T>
class SimulaConstant<T,false>:public SimulaBase{
public:
	typedef T Type;
	SimulaConstant():
		SimulaBase()
	{}
	SimulaConstant(SimulaBase* newAttributeOf, const Time &newstartTime):
		SimulaBase(newAttributeOf, UnitRegistry::noUnit(), newstartTime)
		//constant not set, this may cause troubles when used
	{}
	SimulaConstant(const std::string &newName, SimulaBase* const newAttributeOf, const Unit &newUnit, const Time &newstartTime, const Time &newEndTime):
		SimulaBase(newName, newAttributeOf, newUnit, newstartTime)
	{}
	SimulaConstant(const std::string &newName, SimulaBase* const newAttributeOf, const Type &newConstant, const Unit &newUnit, const Time &newstartTime):
		SimulaBase(newName, newAttributeOf, newUnit, newstartTime),
		constant(newConstant)
	{}
	SimulaConstant(SimulaBase* newAttributeOf, const Time &startTime, const SimulaConstant* copyThis):
		SimulaBase(newAttributeOf, startTime, copyThis),
		//set constant
		constant(copyThis->constant)
	{}

	void get(const Time &t, Type &rc)const{rc=constant;}
	void get(const Time &t, Type &rc){rc=constant;}
	void getRate(const Time &t, double &rc){rc=0;}
	void get(Type &rc)const{rc=constant;}
	void get(Type &rc){rc=constant;}
	Type get()const{return constant;}
	std::string getType()const{
		std::string type="SimulaConstant<";
		if(typeid(T)==typeid(double)) {
			type+="double";
		}else if(typeid(T)==typeid(int)){
			type+="integer";
		}else if(typeid(T)==typeid(std::string)){
			type+="string";
		}else if(typeid(T)==typeid(Coordinate)){
			type+="Coordinate";
		}else if(typeid(T)==typeid(bool)){
			type+="bool";
		}else{
			msg::error("SimulaContstant declared with unsupported type");\
		}
		type+=">";
		return type;
	}

	void setConstant(const Type &rc){constant=rc;}
	void copy(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaConstant<Type>* copyThis){
		SimulaBase* tmp(copyThis);
		copy(newAttributeOf,newstartTime,tmp);
		constant(copyThis->constant);
	}

	virtual void getXMLtag(Tag &tag){
		//name
		tag.name="SimulaConstant";
		tag.closed=true;
		//attributes
		tag.attributes.clear();
		tag.attributes["name"]=getName();
		tag.attributes["prettyName"]=getPrettyName();
		tag.attributes["type"]=getType().substr(15,getType().size()-16);
		tag.attributes["unit"]=getUnit().name;
		if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
		if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
		std::string on(getObjectGeneratorName());
		if(!on.empty()) tag.attributes["objectGenerator"]=on;
		//data
		tag.attributes["_DATA_"]=convertToString<Type>(constant);
		tag.closed=false;

	}

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
	      return new SimulaConstant<T>(attributeOf,startTime,this);
	};

	template <class TVar>
	friend std::istream &operator>>(std::istream &is, SimulaConstant<TVar> &obj);

protected:
	Type constant;
};




//SimulaConstant<Coordinate> specialization
//this specialization is needed for the getAbsolute function which requires 'constant' to be of type Coordinate
template <>
class SimulaConstant<Coordinate>:public SimulaBase{
public:
	typedef Coordinate Type;
	SimulaConstant():
		SimulaBase(),multiplier(this)
	{}
	SimulaConstant(SimulaBase* newAttributeOf, const Time &newstartTime):
		SimulaBase(newAttributeOf, UNITCOORDINATES, newstartTime),multiplier(this)
	{}
	SimulaConstant(const std::string &newName, SimulaBase* newAttributeOf, const Type &newConstant, const Time &newstartTime):
		SimulaBase(newName, newAttributeOf, UNITCOORDINATES, newstartTime),
		//set constant
		constant(newConstant),multiplier(this)
	{}
	SimulaConstant(const std::string &newName, SimulaBase* newAttributeOf, const Time &newstartTime, const Time &newEndTime):
		SimulaBase(newName, newAttributeOf, UNITCOORDINATES, newstartTime, newEndTime),multiplier(this)
	{}

	SimulaConstant(SimulaBase* newAttributeOf,  const Time &startTime, const SimulaConstant<Coordinate>* copyThis):
		SimulaBase(newAttributeOf, startTime, copyThis),
		//set constant
		constant(copyThis->constant),multiplier(this)
	{}
	~SimulaConstant(){
		//do nothing
	}

	void get(const Time &t, Type &rc){
		rc=constant;
		if(multiplier==(this)) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			double m;
			multiplier->get(t,m);
			rc*=m;
		}
	}
	void get(const Time &t, MovingCoordinate &rc)const{rc=constant;}
	void get(const Time &t, MovingCoordinate &rc){rc=constant;}
	void get(Type &rc){
		rc=constant;
		if(multiplier==(this)) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			double m;
			multiplier->get(m);
			rc*=m;
		}
	}
	void getAbsolute(const Time &t, MovingCoordinate &p){
		//current relative position
		p=constant;
		//if referencePoint is NULL-pointer, than this point is the origin.
		if (parent_!=nullptr){
			//current absolute position & direction of the referencePoint
			Coordinate abs;
			parent_->getAbsolute(t,abs);
			//current absolute position & direction
			p+=abs;
		}
	}
	void getAbsolute(const Time &t, Coordinate &p){
		//current relative position
		p=constant;
		//if referencePoint is NULL-pointer, than this point is the origin.
		if (parent_!=nullptr){
			//current absolute position & direction of the referencePoint
			Coordinate abs;
			parent_->getAbsolute(t,abs);
			//current absolute position & direction
			p+=abs;
		}
	}

	virtual SimulaBase *  getReference(){
		return this;
	}

	void setConstant(const Type &rc){constant=rc;}

	std::string getType()const{
		return "SimulaConstant<Coordinate>";
	}

	virtual void getXMLtag(Tag &tag){
		//name
		tag.name="SimulaConstant";
		tag.closed=true;
		//attributes
		tag.attributes.clear();
		tag.attributes["name"]=getName();
		tag.attributes["prettyName"]=getPrettyName();
		tag.attributes["type"]=getType().substr(15,getType().size()-16);
		tag.attributes["unit"]=getUnit().name;
		if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
		if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
		std::string on(getObjectGeneratorName());
		if(!on.empty()) tag.attributes["objectGenerator"]=on;
		//data
		tag.attributes["_DATA_"]=convertToString<Type>(constant);
		tag.closed=false;

	}

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
	      return new SimulaConstant<Coordinate>(attributeOf,startTime,this);
	};

	template <class TVar>
	friend std::istream &operator>>(std::istream &is, SimulaConstant<TVar> &obj);

protected:
	Type constant;
	SimulaBase* multiplier; //todo similar as done here, we can safe memory in the other objects be using multiplier==this pointer instead of usemultiplier as bool to indicate initiation
	//SimulaBase* referencePoint;
};


#endif /*SIMULACONSTANT_HPP_*/
