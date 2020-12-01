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

#include "SimulaBase.hpp"
#include "Database.hpp"
#include "BaseClasses.hpp"
#include "Origin.hpp"
#include "../cli/Messages.hpp"
#include <ostream>
#include "../math/VectorMath.hpp"

SimulaBase::SimulaBase():
	name_(Database::nameList.getReference("noNameSet")),
	referencePoint(nullptr),
	//set unit
	unit_(&UnitRegistry::noUnit()),
	//set lifetime
	startTime(0),
	endTime(0),
	//set parent_
	parent_(nullptr),
	//set in attributes that this is it's parent
	attributes(nullptr)
{
	++numberOfInstantiations;
}
void SimulaBase::copy(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaBase* copyThis){
	//set name_
	name_=copyThis->name_;
	referencePoint=nullptr;
	//set unit
	unit_=copyThis->unit_;
	//set lifetime
	startTime=newstartTime;
	endTime=copyThis->endTime;
	//set parent_
	parent_=newAttributeOf;
	//register as attribute of parent if parent isn't null-pointer (which is the case for the origin)
	parent_->getChildren()->store(this);
	//copy attributes
	++numberOfInstantiations;
	if(copyThis->attributes){
		attributes=new Database(this);
		attributes->insertCopy(newstartTime, *(copyThis->attributes));
	}
	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	setReference();

}
SimulaBase::SimulaBase(SimulaBase* const newAttributeOf, const Unit &newUnit, const Time &newstartTime):
	//set name_
	name_(Database::nameList.getReference(uniqueName(newAttributeOf))),
	referencePoint(nullptr),
	//set unit
	unit_(&UnitRegistry::getUnit(newUnit.name)),
	//set lifetime
	startTime(newstartTime),
	endTime(0),
	//set parent_
	parent_(newAttributeOf),
	//set in attributes that this is it's parent
	attributes(nullptr)
{
	//register as attribute of parent
	parent_->getChildren()->store(this);
	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	setReference();
	++numberOfInstantiations;
}
SimulaBase::SimulaBase(const std::string &newName, SimulaBase* newAttributeOf, const Unit &newUnit, const Time &newstartTime):
	//set name_
	name_(Database::nameList.getReference(newName)),
	referencePoint(nullptr),
	//set unit
	unit_(&UnitRegistry::getUnit(newUnit.name)),
	//set lifetime
	startTime(newstartTime),
	endTime(0),
	//set parent_
	parent_(newAttributeOf),
	//set in attributes that this is it's parent
	attributes(nullptr)
{
	//register as attribute of parent if parent isn't null-pointer (which is the case for the origin)
	if(parent_!=nullptr){
		parent_->getChildren()->store(this);
	}else{
		if(getName()!="origin") msg::error("SimulaBase::SimulaBase: objects must have a parent");
	}

	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	setReference();
	++numberOfInstantiations;
}
SimulaBase::SimulaBase(const std::string &newName, SimulaBase* newAttributeOf, const Unit &newUnit, const Time &newstartTime, const Time &newEndTime):
	//set name_
	name_(Database::nameList.getReference(newName)),
	referencePoint(nullptr),
	//set unit
	unit_(&UnitRegistry::getUnit(newUnit.name)),
	//set lifetime
	startTime(newstartTime),
	endTime(newEndTime),
	//set parent_
	parent_(newAttributeOf),
	//set in attributes that this is it's parent
	attributes(nullptr)
{
	//register as attribute of parent if parent isn't null-pointer (which is the case for the origin)
	parent_->getChildren()->store(this);
	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	setReference();
	++numberOfInstantiations;
}

SimulaBase::SimulaBase(SimulaBase* newAttributeOf,  const Time &newstartTime, const SimulaBase* const copyThis):
	//set name_
	name_(copyThis->name_),
	referencePoint(nullptr), //this must be here so that the children that are being copied will get a nullptr when asking for the reference pointer, and not something undetermined
	//set unit
	unit_(copyThis->unit_),
	//set lifetime
	startTime(newstartTime),
	endTime(copyThis->endTime),
	//set parent_
	parent_(newAttributeOf),
	//set in attributes that this is it's parent
	attributes(nullptr)
{
	//register as attribute of parent if parent isn't null-pointer (which is the case for the origin)
	parent_->getChildren()->store(this);
	++numberOfInstantiations;
	//copy attributes
	if(copyThis->attributes){
		attributes=new Database(this);
		attributes->insertCopy(newstartTime, *(copyThis->attributes));
	}
	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	//this must come after the children being copied, for the children will ask this object what the reference is,
	//and they will not get the right answer resolved based onthe virtual functions, for this object is not fully constructed yet.
	setReference();
}

SimulaBase::~SimulaBase(){
	//remove from parent's attributes if this is not the origin in which case the attributeOff is a null-pointer
	if(parent_!=nullptr)parent_->getChildren()->erase(this);
	if(attributes) {
		delete attributes;
		attributes=nullptr;
	}
	--numberOfInstantiations;
}

int SimulaBase::numberOfInstantiations=0;
SimulaBase::PointerList SimulaBase::predictorOrder;

void SimulaBase::databaseSignalingHook(SimulaBase * newObject){
	msg::error("SimulaBase::databaseSignalingHook called but not implemented for this object");
}

void SimulaBase::registerSignalingHook(){
	SimulaBase::signalMeAboutNewObjects(this);
}

void SimulaBase::updateAll(const Time& t){
	Time st,et;
	SimulaTimeDriven::getGlobalTimeStepInfo(st,et);
	if(st>0. && t<st) st=t;
	if(st>Database::updatedAll+TIMEERROR && !Database::updateAllLock){
		Database::updateAllLock=true;
		ORIGIN->getChildren()->updateRecursively(st);
		Database::updatedAll=st;
		Database::updateAllLock=false;
	}
}

void SimulaBase::updateRecursively(const Time &t){
	if(attributes) attributes->updateRecursively(t);
}


std::string SimulaBase::getName()const{
	return name_;
}
std::string SimulaBase::getPrettyName()const{
	std::string n;
	bool camelCase=false;
	for(std::size_t i(0); i<name_.size() ; ++i ){
		if(isupper(name_[i])) {
			auto j=i+1;
			if(camelCase) n+=' ';
			if(j<name_.size() && isupper(name_[j])) camelCase=false;
			if(camelCase) {
				n+=tolower(name_[i]);
			}else{
				n+=name_[i];
			}
		}else if(name_[i]=='2' && i<name_.size()-2){
			n+=" to ";
			camelCase=true;
		}else if(name_[i]=='4' && i<name_.size()-2){
			n+=" for ";
			camelCase=true;
		}else{
			camelCase=true;
			n+=name_[i];
		}
	}
	return n;
}
void SimulaBase::setName(const std::string &newName){
	//set new name_
	name_=Database::nameList.getReference(newName);
	//if this is the origin (parent_ is a null-pointer), you can not change its name_ or a segmentation fault will occur
	if(parent_!=nullptr){
		//clear old name_ in parent's attribute list
		parent_->getChildren()->erase(this);
		//reregister in parent's attribute list
		parent_->getChildren()->store(this);
	}
}
std::string SimulaBase::getType()const{
	return "SimulaBase";
}

SimulaBase* SimulaBase::getParent() const{
	return parent_;
}
void SimulaBase::setParent(SimulaBase* p){
	//set parent_
	parent_=p;
	//register as attribute of parent
	parent_->getChildren()->store(this);
	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	setReference();
}

SimulaBase* SimulaBase::getReference(){
	if(	referencePoint==parent_) setReference(); //this line is simply there, because we can not determine the reference point in the constructor if the parent has not finished construction yet. This happens when the parent copies children during construction. The reference Pointer than becomes the parent. Here we try to update that, but ofcourse in many cases the reference pointer is the parent. This line could also be left out
	return referencePoint;
}

void SimulaBase::setReference(){
	if(parent_){
		referencePoint=parent_->getReference();
		if(!referencePoint) referencePoint=parent_;
	}else{
		referencePoint=nullptr;
	}

	/*SimulaBase* r=parent_;
	while (r!=nullptr && r->getType()!="SimulaPoint" && r->getType()!="SimulaConstant<Coordinate>") r=r->getParent();

	if (referencePoint==r){
	}else{
		msg::warning("ref=!r");
		referencePoint=r;
	}*/
}


SimulaBase * SimulaBase::getParent(const unsigned int i) const{
	SimulaBase *  r(this->getParent());
	for (unsigned int j(1);j<i && r ;j++) r=r->getParent();
	return r;
}
/*Database const  * SimulaBase::getChildren() const{
	if(!attributes) msg::error("SimulaBase::getChildren: There are no children registered for "+name_);
	return attributes;
}*/
Database* SimulaBase::getChildren(){
	if(!attributes) {
		attributes=new Database(this);
	}
	return attributes;
}

SimulaBase* SimulaBase::getChild(const std::string & name,const Time & t){
	if(attributes) {
		return attributes->get(name,t);
	}else{
		msg::error("SimulaBase::getChild: "+this->getName()+" does not have any children.");
		return nullptr;
	}
}

SimulaBase* SimulaBase::existingChild(const std::string& name,const Time& t){
	if(attributes) {
		return attributes->existing(name,t);
	}else{
		return nullptr;
	}
}
SimulaBase* SimulaBase::getChild(const std::string & name){
	if(attributes) {
		return attributes->get(name);
	}else{
		msg::error("SimulaBase::getChild: "+name+" requested but "+this->getName()+" does not have any children.");
		return nullptr;
	}
}

SimulaBase* SimulaBase::existingChild(const std::string& name){
	if(attributes) {
		return attributes->existing(name);
	}else{
		return nullptr;
	}
}
SimulaBase* SimulaBase::getChild(const std::string & name,const Unit & u){
	if(attributes) {
		return attributes->get(name,u);
	}else{
		msg::error("SimulaBase::getChild:  "+name+" requested but "+this->getName()+" does not have any children.");
		return nullptr;
	}
}

SimulaBase* SimulaBase::existingChild(const std::string& name,const Unit& u){
	if(attributes) {
		return attributes->existing(name,u);
	}else{
		return nullptr;
	}
}


void SimulaBase::getAllChildren(List& list, const Time &t){
	if(attributes) {
		return attributes->getAll(list,t);
	}
}
void SimulaBase::getAllChildren(List& list)const{
	if(attributes) {
		return attributes->getAll(list);
	}
}

SimulaBase* SimulaBase::getSibling(const std::string & name,const Time & t){
	if(parent_) {
		return parent_->getChild(name,t);
	}else{
		msg::error("SimulaBase::getSibling: "+this->getName()+" does not have a parent.");
		return nullptr;
	}
}

SimulaBase* SimulaBase::existingSibling(const std::string& name,const Time& t){
	if(parent_) {
		return parent_->existingChild(name,t);
	}else{
		return nullptr;
	}
}
SimulaBase* SimulaBase::getSibling(const std::string & name){
	if(parent_) {
		return parent_->getChild(name);
	}else{
		msg::error("SimulaBase::getSibling: "+this->getName()+" does not have a parent.");
		return nullptr;
	}
}

SimulaBase* SimulaBase::existingSibling(const std::string& name){
	if(parent_) {
		return parent_->existingChild(name);
	}else{
		return nullptr;
	}
}
SimulaBase* SimulaBase::getSibling(const std::string & name,const Unit & u){
	if(parent_) {
		return parent_->getChild(name,u);
	}else{
		msg::error("SimulaBase::getSibling: "+this->getName()+" does not have a parent.");
		return nullptr;
	}
}

SimulaBase* SimulaBase::existingSibling(const std::string& name,const Unit& u){
	if(parent_) {
		return parent_->existingChild(name,u);
	}else{
		return nullptr;
	}
}




SimulaBase* SimulaBase::getPath(const std::string &name, const Time &t){
	SimulaBase *probe(existingPath(name,t));
	if(!probe) msg::error("SimulaBase::getPath: path "+getPath()+name+" not found");
	return probe;
}
SimulaBase* SimulaBase::getPath(const std::string &name, const Unit &u){
	SimulaBase *probe(existingPath(name,u));
	if(!probe) {
		std::string p(name[0]=='/'?"":getPath());
		msg::error("SimulaBase::getPath: path "+p+name+" not found");
	}
	return probe;
}
SimulaBase* SimulaBase::getPath(const std::string &name){
	SimulaBase *probe(existingPath(name));
	if(!probe) msg::error("SimulaBase::getPath: path "+getPath()+name+" not found");
	return probe;
}




SimulaBase* SimulaBase::existingPath(const std::string &name, const Time &t){
	//Separate path into list names
	std::list<std::string> list;
	std::string::size_type pos(0);
	//loop through the list
	SimulaBase* probe;
	Database* l;
	if(name[pos]=='/'){
		++pos;
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=ORIGIN;
	}else{
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=this;
	}
	for(std::list<std::string>::iterator it(list.begin());it!=list.end() && probe!=nullptr ;++it){
		if(*it==".."){
			probe=probe->getParent();
		}else{
			l=probe->hasChildren();
			if(l){
				probe=l->existing(*it,t);
			}else{
				probe=nullptr;
			}
		}
	}
	return probe;
}
SimulaBase* SimulaBase::existingPath(const std::string &name, const Unit &u){
	//Separate path into list names
	std::list<std::string> list;
	std::string::size_type pos(0);
	//loop through the list
	SimulaBase* probe;
	Database* l;
	if(name[pos]=='/'){
		++pos;
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=ORIGIN;
	}else{
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=this;
	}
	for(std::list<std::string>::iterator it(list.begin());it!=list.end() && probe!=nullptr ;++it){
		if(*it==".."){
			probe=probe->getParent();
		}else{
			l=probe->hasChildren();
			if(l){
				probe=l->existing(*it);
			}else{
				probe=nullptr;
			}
		}
	}
	if(probe) probe->checkUnit(u);
	return probe;
}

void SimulaBase::getAllPositions(const Time & t, SimulaBase::Positions& list){
	SimulaBase::updateAll(t);
	list.clear();
	//insert range in list. Note we do not assert the list is empty - since this is a multimap, duplicates may exist
	for(auto & it:Database::positions){
		if(it.second->evaluateTime(t)) list.insert(it);
	}
}

void SimulaBase::getAllPositions(Positions& list){
	list.clear();
	//insert range in list. Note we do not assert the list is empty - since this is a multimap, duplicates may exist
	for(auto & it:Database::positions){
		list.insert(it);
	}
}

void SimulaBase::getYSlice(const Time & t, const double dl, const double du, SimulaBase::Positions& list){
	SimulaBase::updateAll(t);
	list.clear();
	//set coordinates
	Coordinate cl(-9e10,dl,-9e10), cu(9e10,du,9e10);
	//order dl and du
	if(dl>du){
		cl.y=du;
		cu.y=dl;
	}
	//find boundaries
	SimulaBase::Positions::iterator l(Database::positions.lower_bound(cl)),u(Database::positions.upper_bound(cu));
	//insert in list
	for(SimulaBase::Positions::iterator it(l); it!=u; ++it){
		if(it->second->evaluateTime(t)) list.insert(*it);
	}
}
void SimulaBase::getPositionsInsideBox(const Time & t, const Coordinate& cl, const Coordinate & cu, SimulaBase::Positions& list){
	SimulaBase::updateAll(t);
	list.clear();
	//find boundaries
	SimulaBase::Positions::iterator u(Database::positions.upper_bound(cu));
	//eliminate based on x and z
	//TODO this could be much faster if we used a better container - but for now I am unsure what to use, and this is simple
	for(SimulaBase::Positions::iterator l(Database::positions.lower_bound(cl)); l!=u; ++l){
		if(l->first.x>=cl.x &&
		   l->first.x<=cu.x &&
		   l->first.z>=cl.z &&
		   l->first.z<=cu.z &&
		   l->second->evaluateTime(t)){
			list.insert(*l);
		}
	}
}

void SimulaBase::getPositionsWithinRadius(const Time &t, const Coordinate& c, const double & r, SimulaBase::Positions& list){
	//get bounding box
	SimulaBase::Positions tl;
	getPositionsInsideBox(t,c-r,c+r,tl);
	//eliminate based on distance
	//TODO this could be much faster if we used a better container - but for now I am unsure what to use, and this is simple
	for(SimulaBase::Positions::iterator l(tl.begin()); l!=tl.end(); ++l){
		if(l->second->evaluateTime(t) && vectorlength(c,l->first)<r ){
			list.insert(*l);
		}
	}

}

void SimulaBase::storePosition(SimulaBase *pSB){
	//add to list of positions
	Coordinate c;
	pSB->getAbsolute(pSB->getStartTime(),c);
	if(Database::updatedAll-pSB->getStartTime()>MAXTIMESTEP+TIMEERROR){
		/*this mostly occurs as root nodes are not created if the root is based on an estimate, so they
		 * are only created during the next update round, but their time is still before that. We put
		 * in some tolerance here (0.2) but really it means there is an inconsistent presentation of the data.
		 */
		msg::error("Database::storePosition: registering element with name "+pSB->getPath()+" created more than MAXTIMESTEP days before the last time the list was updated. See code for comments.");
	}
	if(Database::positions.insert(std::pair<Coordinate,SimulaBase*>(c,pSB))==Database::positions.end()) msg::error("Database: Failed to insert coordinate.");
	//hook for signaling other objects
	for(auto &it:Database::hooks_){
		(it)->databaseSignalingHook(pSB);
	}

}
void SimulaBase::broadcast(SimulaBase *pSB){
	//hook for signaling other objects
	for(auto &it:Database::hooks_){
		(it)->databaseSignalingHook(pSB);
	}
}

void SimulaBase::signalMeAboutNewObjects(SimulaBase * me){
	Database::hooks_.push_back(me);
	///TODO , signal historical info.
}
void SimulaBase::setFunctionPointer(const std::string &name){
	if(!attributes) {
		if(name.size()){
			attributes=new Database(this);
			attributes->setFunctionPointer(name);
		}//else do nothing
	}else{
		attributes->setFunctionPointer(name);
	}
}


void SimulaBase::stopUpdatefunction(){
	//stop updating by setting generator pointer to nullptr
	if(attributes) attributes->stopUpdatefunction();
}

SimulaBase* SimulaBase::existingPath(const std::string &name){
	//Separate path into list names
	std::list<std::string> list;
	std::string::size_type pos(0);
	//loop through the list
	SimulaBase* probe;
	Database* l;
	if(name[pos]=='/'){
		++pos;
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=ORIGIN;
	}else{
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=this;
	}
	for(std::list<std::string>::iterator it(list.begin());it!=list.end() && probe!=nullptr ;++it){
		if(*it==".."){
			probe=probe->getParent();
		}else{
			l=probe->hasChildren();
			if(l){
				probe=l->existing(*it);
			}else{
				probe=nullptr;
			}
		}
	}
	return probe;
}


///@todo getNext, getFirst and getPrevious are expensive code, which could be done more elegantly?
SimulaBase* SimulaBase::getNextSibling(const Time &t){
	Database *a=getParent()->getChildren();

	//update the attribute list first
	if (a->generator) a->generator->generateObjects(t);

	//find it -- copy is done in order to have alphabetic order. data is not sorted alphabetically.
	std::set<std::string> al;
	for(auto dit(a->data.begin()); dit!=a->data.end() ; ++dit){
		if(dit->second->evaluateTime(t))	al.insert(*(dit->first));
	}
	std::set<std::string>::const_iterator it=al.find(name_);

	//check whether it was really found and
	//check whether time is between begin and end time (if not set to 0)
	if (it==al.end() ) {
		if(al.size()!=a->data.size()) {
			msg::error("Database::getNext: Object '"+name_+"' requested by "+"???"+" but not listed (or asked before its own start time) in attribute list '"+getPath()+"'.");
		}else{
			msg::error("Database::getNext: Object '"+name_+"' requested by "+"???"+" but not listed in attribute list '"+getPath()+"'.");
		}
	}

	//increase to next entry
	++it;
	if(it==al.end()) return nullptr;

	//return the SimulaBase Pointer
	return a->get(*it);
}
SimulaBase* SimulaBase::getNextSibling()const{
	Database *a=getParent()->getChildren();
	//find it
	std::set<std::string> al;
	for(auto dit(a->data.begin()); dit!=a->data.end() ; ++dit){
		al.insert(*(dit->first));
	}
	std::set<std::string>::const_iterator it=al.find(name_);

	//check whether it was really found and
	if (it==al.end()) {
		msg::error("Database::getNext: Object '"+name_+"' requested by "+"???"+"  but not listed in attribute list '"+getPath()+"'.");
	}

	//increase to next entry
	++it;
	if(it==al.end()) return nullptr;

	//return the SimulaBase Pointer
	return a->get(*it);
}
SimulaBase* SimulaBase::getPreviousSibling(const Time &t){
	Database *a=getParent()->getChildren();
	//update the attribute list first
	if (a->generator) a->generator->generateObjects(t);

	//find it
	std::set<std::string> al;
	for(auto dit(a->data.begin()); dit!=a->data.end() ; ++dit){
		if(dit->second->evaluateTime(t))	al.insert(*(dit->first));
	}
	std::set<std::string>::const_iterator it=al.find(name_);

	//check whether it was really found and check whether time is between begin and end time (if not set to 0)
	if (it==al.end() ) msg::error("Database::getPrevious: Object '"+name_+"' requested by "+"???"+" but not listed in attribute list '"+getPath()+"'.");

	//go to previous entry
	if(it==al.begin()) return nullptr;
	--it;

	//return the SimulaBase Pointer
	return a->get(*it);
}
SimulaBase* SimulaBase::getPreviousSibling()const{
	Database *a=getParent()->getChildren();
	//find it
	std::set<std::string> al;
	for(auto dit(a->data.begin()); dit!=a->data.end() ; ++dit){
		al.insert(*(dit->first));
	}
	std::set<std::string>::const_iterator it=al.find(name_);

	//check whether it was really found and check whether time is between begin and end time (if not set to 0)
	if (it==al.end() ) msg::error("Database::getPrevious: Object '"+name_+"' requested by "+"???"+" but not listed in attribute list '"+getPath()+"'.");

	//go to previous entry
	if(it==al.begin()) return nullptr;
	--it;

	//return the SimulaBase Pointer
	return a->get(*it);
}

SimulaBase* SimulaBase::getFirstChild(const Time &t){
	if(attributes){
		return attributes->getFirst(t);
	}else{
		return nullptr;
	}

}
SimulaBase* SimulaBase::getFirstChild()const{
	if(attributes){
		return attributes->getFirst();
	}else{
		return nullptr;
	}
}
SimulaBase* SimulaBase::getLastChild()const{
	if(attributes){
		return attributes->getLatest();
	}else{
		return nullptr;
	}

}



int SimulaBase::getNumberOfChildren()const{
	if(attributes){
		return attributes->size();
	}else{
		return 0;
	}
}

int SimulaBase::getNumberOfChildren(const Time & t){
	if(attributes){
		return attributes->size(t);
	}else{
		return 0;
	}
}


Database* SimulaBase::hasChildren(){
	return attributes;
}



void SimulaBase::changeParent(SimulaBase* newAttributeOf){
	//store old name_
	parent_->getChildren()->erase(this);
	//set new parent
	parent_=newAttributeOf;
	//register with new parent
	parent_->getChildren()->store(this);
	//set reference point to the first parent of type SimulaPoint or SimulaConstant<Coordinate>
	setReference();
}

bool SimulaBase::evaluateTime(const Time &t)const{
	if (t<startTime){
		return false;
	}
	if (endTime!=0 && t>endTime) {
		return false;
	}
	return true;
}
void SimulaBase::setEndTime(const Time &t){
	endTime=t;
}
void SimulaBase::setStartTime(const Time &t){
	//notice that this function is not 'safe' to use as it does not guarantee consistency.
	//It's just provided so that a default constructor can be used.
	startTime=t;
}
Time SimulaBase::getEndTime()const{
	return endTime;
}
Time SimulaBase::getStartTime()const{
	return startTime;
}





void SimulaBase::get(const Time &t, int &returnConstant){
	msg::error("SimulaBase::get: Integer requested, but not simulated by this simula object: "+getPath()+" of type: "+getType());
}
void SimulaBase::get(const Time &t, std::string &returnConstant){
	msg::error("SimulaBase::get: String requested, but not simulated by this simula object: "+getPath()+" of type: "+getType());
}
void SimulaBase::getRate(const Time &t, Time &var){
	msg::error("SimulaBase::get: double requested, but not simulated by this simula object: "+getPath()+" of type: "+getType());
}
void SimulaBase::get(const Time &t, Time &var){
	msg::error("SimulaBase::get: double requested, but not simulated by this simula object: "+getPath()+" of type: "+getType());
}

void SimulaBase::getRate(const Time &t, Coordinate &var){
	parent_->get(t,var);
}
void SimulaBase::getAbsolute(const Time &t, MovingCoordinate &p){
	referencePoint->getAbsolute(t,p);
}
void SimulaBase::getAbsolute(const Time &t, Coordinate &p){
	referencePoint->getAbsolute(t,p);
}

void SimulaBase::get(const Time &t, Coordinate &p){
	MovingCoordinate mc;
	this->get(t,mc);
	p=mc.state;
}
void SimulaBase::get(const Time &t, const Coordinate & pos, Coordinate &y){
	//reduce dimensions
	this->get(t,y);
}
void SimulaBase::get(const Time &t, MovingCoordinate &p){
	msg::error("SimulaBase::get: (Moving)Coordinate requested, but not simulated by this simula object: "+getPath()+" of type: "+getType());
}
void SimulaBase::getBase(const Time &t, Coordinate &p){
	referencePoint->getAbsolute(t,p);
}

void SimulaBase::get(Time &returnConstant){
	msg::error("SimulaBase::get: Constant of type Time requested. But this object with name "+getPath()+" and type "+getType()+ " does not simulate that.");
}
void SimulaBase::get(int &returnConstant){
	msg::error("SimulaBase::get: Constant of type int requested. But this object with name "+getPath()+" and type "+getType()+ " does not simulate that.");
}
void SimulaBase::get(std::string &returnConstant){
	msg::error("SimulaBase::get: Constant of type string requested. But this object with name "+getPath()+" and type "+getType()+ " does not simulate that.");
}
void SimulaBase::get(Coordinate &point){
	msg::error("SimulaBase::get: Constant of type Coordinate requested. But this object with name "+getPath()+" and type "+getType()+ " does not simulate that.");
}
void SimulaBase::get(bool &b){
	msg::error("SimulaBase::get: Constant of type bool requested. But this object with name "+getPath()+" and type "+getType()+ " does not simulate that.");
}
void SimulaBase::get(const Time &t, const Coordinate & pos, double &y){
	get(t,y);
}
void SimulaBase::getRate(const Time &t, const Coordinate & pos, double &y){
	msg::error("SimulaBase::getRate: missing implementation of get(Time,Coordinate,double) objects of type "+getType()+ ". But it is requested for object with name "+getPath());
}


Time SimulaBase::getTimeStep(const Time& t){
	return 0;
}

void SimulaBase::getAverageRate(const Time &t1, const Time &t2, double &var){
	double s1,s2;
	this->get(t1,s1);
	this->get(t2,s2);
	var=(s2-s1)/(t2-t1);
}
void SimulaBase::getAverageRate(const Time &t1, const Time &t2, Coordinate &var){
	Coordinate s1,s2;
	this->get(t1,s1);
	this->get(t2,s2);
	var=(s2-s1)/(t2-t1);
}

void SimulaBase::getTime(const Coordinate &p, Time &t, Time tmin, Time tmax){
	msg::error("SimulaBase::getTime: method not implemented for "+getPath());
}
void SimulaBase::getTime(const double &p, Time &t, Time tmin, Time tmax){
	msg::error("SimulaBase::getTime: method not implemented for "+getPath());
}



/*void SimulaBase::set(const double &x, const double &y){
	msg::error("SimulaBase::set: can not insert values in non-SimulaTable object: "+getPath()+" of type: "+getType());
}*/

SimulaBase* SimulaBase::followChain(const Time & t){
	msg::error("SimulaBase::followChain: this object with name "+getPath()+" is not part of a chain");
	return nullptr;
}


Unit SimulaBase::getUnit(){
		return *unit_;
}
void SimulaBase::checkUnit(const Unit& check)const{
	if(*unit_!=check)msg::error("SimulaBase::checkUnit: '"+getPath()+"' should be in "+check.name+" but is in "+unit_->name);
}
void SimulaBase::setUnit(const Unit &newUnit){
	unit_=&UnitRegistry::getUnit(newUnit.name);
}

void SimulaBase::copyAttributes(const Time &startTime, const SimulaBase *original){
	if(original->getNumberOfChildren()) {
		if(!attributes) {
			attributes=new Database(this);
		}
		//attributes->insertCopy(startTime, *(original->getChildren()));
		attributes->insertCopy(startTime, *(original->attributes));
	}
}


Time &SimulaBase::minTimeStep(){
	msg::error("SimulaBase::minTimeStep: this is a static object");
	return startTime;
}
Time &SimulaBase::maxTimeStep(){
	msg::error("SimulaBase::maxTimeStep: this is a static object");
	return startTime;
}
Time &SimulaBase::preferedTimeStep(){
	msg::error("SimulaBase::preferedTimeStep: this is a static object of type"+getType());
	return startTime;
}
Time SimulaBase::lastTimeStep(){
	msg::error("SimulaBase::lastTimeStep: this is a static object");
	return startTime;
}

void SimulaBase::collectGarbage(const Time & t){
	//clean up generators
	if(attributes && attributes->generator && !attributes->generator->run){
		attributes->setFunctionPointer();//deletes the generator, as function pointer is empty
	}
};
void SimulaBase::garbageCollectionOff(){
}
void SimulaBase::garbageCollectionOn(){
	//do nothing.
}

bool SimulaBase::setDependent(SimulaBase* me){
	//msg::error("SimulaBase::setDependent: no virtual function implemented for this");
	return false;
}
void SimulaBase::unLockDependency(){
	msg::error("SimulaBase::unLockDependency: no virtual function implemented for this");
}

std::string SimulaBase::getObjectGeneratorName()const{
	std::string fn;
	if(attributes){
		fn=attributes->getObjectGeneratorName();
	}
	return (fn);
}

void SimulaBase::getXMLtag(Tag &tag){
	std::string n(getType()),t("");
	std::size_t b1(n.find('<')),b2(n.size()-(b1+2));
	if(b1!=std::string::npos){
		t=" type=\""+n.substr(b1+1,b2)+"\" ";
		n=n.substr(0,b1);
	}
	if(n!="SimulaBase") msg::warning("SimulaBase::dumpXML: dumpXML not implemented for "+n);
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


	std::string on(getObjectGeneratorName());
	if(!on.empty()) tag.attributes["objectGenerator"]=on;
	//nodata
	//children
	if(getNumberOfChildren()) tag.closed=false;

}



std::size_t SimulaBase::getPredictorSize(){
	return predictorOrder.size();
}
void SimulaBase::clearPredictions(const std::size_t &s){
	if(s<predictorOrder.size()){
		predictorOrder.erase(predictorOrder.begin()+s,predictorOrder.end());
	}
}
void SimulaBase::releaseDependents(){
	//do nothing
}
void SimulaBase::addToPredictorList(SimulaBase *p){
	predictorOrder.push_back(p);
}

SimulaBase* SimulaBase::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      return new SimulaBase(attributeOf,startTime,this);
}

std::string SimulaBase::getPath()const{
	//return type
	std::string r=getName();

	//loop till origin as been reached
	SimulaBase* parent=getParent();
	while(parent){
		r.insert(0,parent->getName()+'/');
		parent=parent->getParent();
	}

	r.insert(0,"/");

	return r;
}
