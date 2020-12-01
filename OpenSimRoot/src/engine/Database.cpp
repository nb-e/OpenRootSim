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

#include "Database.hpp"
#include "../cli/Messages.hpp"
#include "SimulaBase.hpp"
#include "BaseClasses.hpp"
#include "Origin.hpp"
#include <math.h>
#if _WIN32 || _WIN64
#include <ctime>
#endif
//constructor for database

Database::Database(SimulaBase* pSB):
	//set the function pointer for creating objects. nullptr will result in no updatefunction being called
	generator(nullptr),
	//set parent
	attributeOf(pSB)
{}

ReferenceList Database::nameList;
void Database::insertCopy(const Time &startTime, const Database &copyThis){
	//copy the update function
	//todo: this may not be a good place for this, since it may overwrite the previous update function
	if(copyThis.generator)setFunctionPointer(copyThis.generator->name);

	//copy attribute list: note that not the pointers but the objects themselves need to be copied;
	for(Table::const_iterator it=copyThis.data.begin() ; it!=copyThis.data.end() ; ++it){
		it->second->createAcopy(attributeOf,startTime);
	}
}

//Destructor for database. This makes sure that all objects that are registered are also deleted.
Database::~Database(){
	//delete all simulaobjects the database is pointing to when an object is deleted, it will remove itself from the list
	std::vector<SimulaBase*> pl;
	for(auto & it:data){
		pl.push_back(it.second);
	}
	for(auto & it:pl){
		delete (it);
	}

	if(generator) delete generator;
}
void Database::clear(){
	//delete all simulaobjects the database is pointing to
	while(!data.empty())	delete data.begin()->second;
}

SimulaBase* Database::get(const std::string &name, const Time &t){
	//update the attribute list first, the lock guards against
	if (generator) generator->generateObjects(t);

	//find it
	Table::const_iterator it(data.find(nameList.getPointer(name)));

	//check whether it was really found and
	if (it==data.end()){
		//create list of available names
		std::string list;
		for (Table::const_iterator i(data.begin());i!=data.end();++i){
			list+=*(i->first);
			list+=" ; ";
		}
		//message error
		msg::error("Database::get: Object '"+name+"' requested by "+"???"+" but not listed in attribute list '"+attributeOf->getPath()+"'.\nList of available names: "+list);
	}
	//check whether time is between begin and end time (if not set to 0)
	if (t>=0 && !it->second->evaluateTime(t))msg::error("Database::get: Object '"+name+"' requested but object does not exist at time "+ std::to_string(t)+". startTime: "+std::to_string(it->second->getStartTime())+" in attribute list '"+attributeOf->getPath()+"' with startTime "+std::to_string(attributeOf->getStartTime())+"." );

	//return the pointer listed in colum 2
	return it->second;
}
SimulaBase* Database::get(const std::string &name, const Unit &unit)const{
	//update the attribute list first, the lock guards against
	if (generator) generator->generateObjects(attributeOf->getStartTime());

	//find it
	Table::const_iterator it=data.find(nameList.getPointer(name));

	//check whether it was really found and
	//check whether time is between begin and end time (if not set to 0)
	if (it==data.end() ){
		//create list of available names
		std::string list;
		for (it=data.begin() ; it != data.end() ; ++it){
			list+=*(it->first);
			list+=" ; ";
		}

		//message error
		msg::error("Database::get: Object '"+name+"' requested by "+"???"+" but not listed in attribute list '"+attributeOf->getPath()+"'.\nList of available names: "+list);
	}

	//check the unit
	//todo, would me more useful if the modules name was not database::get, but the one calling database::get
	if(unit!="na") it->second->getUnit().check(unit,it->second->getName(),"Database::get");

	//return the pointer listed in colum 2
	return it->second;
}
SimulaBase* Database::getFirst(const Time &t){
	//update the attribute list first
	if (generator) generator->generateObjects(t);

	//find it
	std::set<std::string> al;
	for(Table::const_iterator dit(data.begin()); dit!=data.end() ; ++dit){
		if(dit->second->evaluateTime(t))	al.insert(*(dit->first));
	}
	if(al.empty()){
		return nullptr; ///Todo this is not safely used in the code.
	}
	std::set<std::string>::const_iterator it=al.begin();
	//return the SimulaBase Pointer
	return get(*it);
}
SimulaBase* Database::getFirst()const{
	//find it
	std::set<std::string> al;
	for(Table::const_iterator dit(data.begin()); dit!=data.end() ; ++dit){
		al.insert(*(dit->first));
	}
	if(al.empty()){
		return nullptr;
	}
	std::set<std::string>::const_iterator it=al.begin();
	//return the SimulaBase Pointer
	return get(*it);
}
SimulaBase* Database::getLatest()const{
	//find it
	SimulaBase *p=nullptr;
	Time l(-1);
	for(auto & dit:data){
		Time t(dit.second->getStartTime());
		if(t>l){
			l=t;
			p=dit.second;
		}
	}
	return p;
}

/*SimulaBase* Database::getLast(const Time &t, const bool &update){
	//update the attribute list first
	if (generator && update) generator->generateObjects(t);

	//check if not empty
	if (data.empty()) return nullptr;

	//find first entry that is within time
	for (Table::reverse_iterator it(data.rbegin()); it!=data.rend() ; ++it){
		if(it->second->evaluateTime(t)) return it->second;
	}
	return nullptr;
}*/
/*Database::List Database::getAll(const Time &t, const bool &update){
	//update the attribute list first, the lock guards against
	if (generator && update) generator->generateObjects(t);
	//return value
	List r;
	//copy all pointers
	if(t<0){
		for (Database::Table::const_iterator it(data.begin()) ; it!=data.end() ; ++it){
			if(it->second->evaluateTime(t)) r.push_back(it->second);
		}
	}else{
		for (Database::Table::const_iterator it(data.begin()) ; it!=data.end() ; ++it){
			r.push_back(it->second);
		}
	}
	//return the vector
	return r;
}*/
void Database::getAll(List &r, const Time &t){
	//update the attribute list first, the lock guards against
	if (generator) {
		generator->generateObjects(t);
	}
	//return value
	r.clear();
	//copy all pointers
	for (Database::Table::const_iterator it(data.begin()) ; it!=data.end() ; ++it){
		if(it->second->evaluateTime(t)) r.push_back(it->second);
	}
}
void Database::getAll(List &r)const{
	//return value
	r.clear();
	//copy all pointers
	for (Database::Table::const_iterator it(data.begin()) ; it!=data.end() ; ++it){
		r.push_back(it->second);
	}
}
std::size_t Database::size(const Time &t){
	//update the attribute list first, the lock guards against
	if (generator) generator->generateObjects(t);
	//if there is no data return true
	if (data.empty()) return 0;
	//try to find whether any of the entries exist during time t, if yes, return false
	if (t>=0){
		int count=0;
		for (Table::const_iterator it=data.begin(); it!=data.end() ; ++it){
			if (it->second->evaluateTime(t)) ++count;
		}
		return count;
	}else{
		//do not check time requirements, just return false for there are some objects.
		return data.size();
	}
}
SimulaBase* Database::existing(const std::string &name, const Time &t){
	//update the attribute list first
	if (generator) generator->generateObjects(t);

	//find it
	Table::const_iterator it(data.find(nameList.getPointer(name)));

	//check whether it is was found
	if (it==data.end()) return nullptr;

	//check time boundaries
	if (t>=0 && !(it->second->evaluateTime(t))) return nullptr;

	//all tests passed return true
	return it->second;
}
SimulaBase* Database::existing(const std::string &name,const Unit &unit)const{	//update the attribute list first
	//find it
	Table::const_iterator it(data.find(nameList.getPointer(name)));

	//check whether it is was found
	if (it==data.end()) return nullptr;

	//check the unit
	it->second->getUnit().check(unit,it->second->getName(),"Database::get");

	//all tests passed return true
	return it->second;
}
SimulaBase* Database::existing(const std::string &name)const{	//update the attribute list first
	//find it
	std::string * strp=nameList.getPointer(name);
	//std::cout<<std::endl<<"Looking for "<<strp;
	//int count(0);
	//for(Table::const_iterator it(data.begin()); it!=data.end(); ++it){
	//	std::cout<<std::endl<<count<<" "<<it->first<<" "<<*(it->first);
	//	++count;
	//}
	Table::const_iterator it(data.find(strp));

	//check whether it is was found
	if (it==data.end()) return nullptr;

	//all tests passed return pointer
	return it->second;
}
SimulaBase* Database::existingPath(const std::string &name, const Time &t){
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
		probe=attributeOf;
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
SimulaBase* Database::existingPath(const std::string &name, const Unit &unit){
	//seperate path into list names
	std::list<std::string> list;
	std::string::size_type pos(0);
	//loop through the list
	SimulaBase* probe;
	if(name[pos]=='/'){
		++pos;
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=ORIGIN;
	}else{
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=attributeOf;
	}
	for(std::list<std::string>::iterator it(list.begin()); it!=list.end() && probe!=nullptr ;++it){
		if(*it==".."){
			probe=probe->getParent();
		}else{
			probe=probe->existingChild(*it);
		}
	}
	if(probe) probe->checkUnit(unit);
	return probe;
}
SimulaBase* Database::getPath(const std::string &name, const Time &t){
	//Separate path into list names
	std::list<std::string> list;
	std::string::size_type pos(0);
	//loop through the list
	SimulaBase* probe;
	if(name[pos]=='/'){
		++pos;
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=ORIGIN;
	}else{
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=attributeOf;
	}
	for(std::list<std::string>::iterator it(list.begin());it!=list.end() && probe!=nullptr ;++it){
		if(*it==".."){
			probe=probe->getParent();
		}else{
			probe=probe->getChild(*it,t);
		}
	}
	return probe;
}

SimulaBase* Database::getPath(const std::string &name, const Unit &unit){
	//seperate path into list names
	std::list<std::string> list;
	std::string::size_type pos(0);
	//loop through the list
	SimulaBase* probe;
	if(name[pos]=='/'){
		++pos;
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=ORIGIN;
	}else{
		while (pos<name.size()) list.push_back(nextWord(name,pos,'/'));
		probe=attributeOf;
	}
	for(std::list<std::string>::iterator it(list.begin()); it!=list.end() && probe!=nullptr ;++it){
		if(*it==".."){
			probe=probe->getParent();
		}else{
			probe=probe->getChild(*it);
		}
	}
	probe->checkUnit(unit);
	return probe;
}

void Database::getPositions(const Time & t, const Coordinate& p, SimulaBase::Positions& list){
	SimulaBase::updateAll(t);
	list.clear();
	//find range, not that coordinate p may not be unique, so there may be multiple answers
	auto r(positions.equal_range(p));
	//insert range in list. Note we do not assert the list is empty - since this is a multimap, duplicates may exist
	for(auto it(r.first); it!=r.second; ++it){
		if(it->second->evaluateTime(t)) list.insert(*it);
	}
}

Time Database::updatedAll(-1);
bool Database::updateAllLock(false);
//todo this is reentried in a loop, nothing is updated and than updateAll is updated to 3 and in matter of fact, estimates (because it was not updated) are not signalled

void Database::updateRecursively(const Time &t){
	//check time
	if(attributeOf->evaluateTime(t)){
		//update the attribute list
		if (generator) generator->generateObjects(t);
		//call children to update
		for (Database::Table::const_iterator it(data.begin()) ; it!=data.end() ; ++it){
			SimulaBase* probe(it->second);
			if (probe->getName().find( "Template" , 0 ) != std::string::npos) continue;
			//if (probe->getName().find( "template" , 0 ) != std::string::npos) continue;
			if(probe->hasChildren()){
				probe->getChildren()->updateRecursively(t);
			}
		}
	}
}

void Database::stopUpdatefunction(){
	//stop updating by setting generator pointer to nullptr
	//deletion is postponed and done by garbage collector, as generator is probably signalling itself that it is done and can be deleted.
	generator->run=false;
}

SimulaBase::Positions Database::positions;

void Database::store(SimulaBase* pSB){
	//check if time of child object is later than parents time
	if(pSB->getStartTime()< attributeOf->getStartTime()-TIMEERROR){
		msg::error("Database::store: can't register an object with lifetime outside it's parents. Debug your code. \n Path:/"+attributeOf->getPath()+"/"+pSB->getName());
	}
	//insert it if not found. Insert inserts a pair and returns a pair<iterator,bool> of which the bool indicates succes
	if(!data.insert(std::pair<Colum1,Colum2>(nameList.getPointer(pSB->getName()),pSB)).second)
		msg::error("Database::store: Failed to store '"+pSB->getName()+"'. An object is already listed as attribute of object '"+attributeOf->getPath()+"'.");
}

std::vector<SimulaBase*> Database::hooks_;


void Database::erase(SimulaBase* pSB){
	data.erase(nameList.getPointer(pSB->getName()));
}

std::size_t Database::size()const{
	return data.size();
}

void Database::setFunctionPointer(const std::string &name){
	if(generator) delete generator;
	if(!name.empty()){
		std::map<std::string, objectGeneratorInstantiationFunction >::iterator it(BaseClassesMap::getObjectGeneratorClasses().find(name));
		if(it==BaseClassesMap::getObjectGeneratorClasses().end()){
			std::string msg("Database::setFunctionPointer: function '"+name+"' not registered.");
			msg::error(msg);
		}else{
			generator= (it->second)(attributeOf) ;
			generator->name=name;
		}
	}else{
		generator=nullptr;
	}
}
std::string Database::getObjectGeneratorName()const{
	std::string r;
	if(generator){
		r=generator->name;
	}
	return r;
}


SimulaBase* Database::getParent() const{
	return attributeOf;
}
void Database::setParent(SimulaBase* parent){
	attributeOf=parent;
}

//Function to generate a unique name for the attributes database in parent
std::string uniqueName(SimulaBase* parent){
	//seed the rand() function with the time to get really a random figure
	srand((unsigned)time(0));
	//create string
	std::string un("unique00000");
	//change last 5 characters to a random character. Do this till a unique string has been created
	while (parent->existingChild(un)){
		un[7]=(char)ceil((double)rand()/(double)RAND_MAX);
		un[8]=(char)ceil((double)rand()/(double)RAND_MAX);
		un[9]=(char)ceil((double)rand()/(double)RAND_MAX);
		un[10]=(char)ceil((double)rand()/(double)RAND_MAX);
		un[11]=(char)ceil((double)rand()/(double)RAND_MAX);
	}
	//return the unique name
	return un;
}


#include <cstring>
extern"C"{
	//database requests by Fortran code - only constants/tables work with this.
    void existingdatapath(long int *y, char path[300],int l){
		SimulaBase* p = ORIGIN->existingPath(path);
		if(p){
			*y=1;
		}else{
			*y=0;
		}

    }
	void getdatabypath(double *x, double *y, char path[300],int l){
		SimulaBase* p;
		p = ORIGIN->getPath(path);
		double X(*x),Y;
		p->get(X,Y);
		*y=Y;
	}
	void getdataarraybypath(double *x, double *y, double *z, double *a, int* al, char path[300],int l){
		SimulaBase* p;
		p = ORIGIN->getPath(path);
		double Y;
		for(int i=0; i!=*al; ++i){
			p->get(0,Coordinate(x[i],y[i],z[i]),Y);
			a[i]=Y;
		}
	}
	void getspatialdatabypath(double *t, double *x, double *y, double *z, double *a, int* al, char path[300],int l){
		SimulaBase* p;
		p = ORIGIN->getPath(path);
		double Y;
		for(int i=0; i!=*al; ++i){
			p->get(*t,Coordinate(x[i],y[i],z[i]),Y);
			a[i]=Y;
		}
	}
	void getdatabypath_coordinate(double *xu,double *yu,double *zu,char path[300],int l){
		SimulaBase* p;
		p = ORIGIN->getPath(path);
		Coordinate X;
		p->get(X);
		*xu=X.x;
		*yu=X.y;
		*zu=X.z;
	}
	void getdatabypath_logical(bool *x,char path[300],int l){
		SimulaBase* p;
		p = ORIGIN->getPath(path);
		bool X;
		p->get(X);
		*x=X;
	}
	void getdatabypath_time(double *x,char path[300],int l){
		SimulaBase* p;
		p = ORIGIN->getPath(path);
		Time X;
		p->get(X);
		*x=X;
	}
	void getdatabypath_string(char s[300],std::size_t k){
		SimulaBase* p;
		p = ORIGIN->getPath(s);
		std::string X;
		p->get(X);
		for(std::size_t i=X.length(); i<k; ++i){
			s[i]=' ';
		}
		strcpy(s,X.c_str());
		k=X.length();
		s[k]=' ';//remove  null-terminator
	}
}

