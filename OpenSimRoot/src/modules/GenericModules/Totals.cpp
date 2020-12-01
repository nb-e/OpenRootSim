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
#include "Totals.hpp"
#include "../../cli/Messages.hpp"
#include <set>
#include "../../tools/StringExtensions.hpp"

TotalBase::TotalBase(SimulaDynamic* const pSV) :
	DerivativeBase(pSV),
	 chain1(pSD->getParent(3)->existingChild("growthpoint")),
	 chain2(nullptr) {
	if(chain1) chain1=chain1->existingChild(pSD->getName());
}
void TotalBase::calculate(const Time &t, double& flux) {
	msg::error("TotalBase::calculate: your class should overide this method");
}
SimulaBase *TotalBase::getNext(const Time & t){
	//find next datapoint
	if (!chain2){
		chain2 = pSD->getParent()->getNextSibling(t);
		if(chain2) chain2=chain2->getChild(pSD->getName());
	}
	//check based on time what the next point is: growthpoint of datapoint
	if (chain2 && t >= chain2->getStartTime()) {
		return chain2;
	} else {
		if(chain1) {
			return chain1;
		}else{
			//msg::warning("TotalBase::getNext: ../../../growthpoint/"+pSD->getName()+"  not found. Breaking chain and returning this");
			return this->pSD;
		}
	}
}
std::string TotalBase::getName()const{
	msg::error("TotalBase::getName: programming error: forgot to include a getName() function in class used by "+pSD->getPath());
	return "TotalBase";
}
DerivativeBase * newInstantiationTotalBase(SimulaDynamic* const pSV) {
	return new TotalBaseLabeled(pSV);
}



TotalBaseLabeled::TotalBaseLabeled(SimulaDynamic* const pSV) :
	DerivativeBase(pSV) {
	//initiate the chain
	chain1=pSD->getParent(4)->existingChild("growthpoint");
	if(chain1) chain1=chain1->getChild(pSD->getParent()->getName())->getChild(pSD->getName());
	chain2=nullptr;
}
TotalBaseLabeled::~TotalBaseLabeled(){

}
void TotalBaseLabeled::calculate(const Time &t, double& flux) {
	msg::error("TotalBaseLabeled::calculate: your class should overide this method");
}
SimulaBase *TotalBaseLabeled::getNext(const Time & t){
	//find next datapoint
	if (!chain2){
		chain2 = pSD->getParent(2)->getNextSibling(t); //time is important so that the list is first updated, but than excludes the point if it is before time t
		if(chain2) chain2=chain2->getChild(pSD->getParent()->getName())->getChild(pSD->getName());
	}
	//check based on time what the next point is: growthpoint of datapoint
	if (chain2 && t >= chain2->getStartTime()) {
		return chain2;
	} else {
		if(chain1){
			return chain1;
		}else{
			msg::error("TotalBaseLabeled: getNext requested, but next is growthpoint which does not exist");
			return chain1;
		}
	}
}
std::string TotalBaseLabeled::getName()const{
	msg::error("TotalBaseLabeled::getName: programming error: forgot to include a getName() function in class used by "+pSD->getPath());
	return "TotalBaseLabeled";
}
DerivativeBase * newInstantiationTotalBaseLabeled(SimulaDynamic* const pSV) {
	return new TotalBaseLabeled(pSV);
}


//generic aggregation functions
RootTotal::RootTotal(SimulaDynamic* pSD):DerivativeBase(pSD),rates(true)
{	//parents name
	parent=pSD->getParent()->getName();
	//are we getting rates or not?
	if(pSD->getType()!="SimulaVariable")rates=false;
	//check if parent is datapoints or grandparent is datapoints
	if(pSD->existingSibling("rootType")){
		mode=1;
	}else{
		mode=2;
	}
	//the name of the variable that needs to be summed up.
	name=pSD->getName();
	//insert the 'Segment' following nameing rules: rootSegment - root - rootSystem
	name.insert(4,"Segment");
	//last datapoint and growthpoint
	last=pSD->getParent(mode)->getChild("dataPoints");
	gp=pSD->getParent(mode)->getChild("growthpoint");
	if(mode==1){
		gp=gp->getChild(name);
	}else{
		gp=gp->getChild(parent)->getChild(name);
	}
}
void RootTotal::calculate(const Time &t,double &total){
	total=0;
	///@todo used getAll() as long as we can not get iterators from the database directly
	double sv;
	SimulaBase::List segments;
	last->getAllChildren(segments,t);
	SimulaBase * p;
	for(auto & it:segments){
		if(mode==1){
			p=(it)->getChild(name);
		}else{
			p=(it)->getChild(parent)->getChild(name);
		}
		if(rates && p->getType()=="SimulaVariable"){
			p->getRate(t,sv);
		}else{
			p->get(t,sv);
		}
		total+=sv;
	}
	//add data for growthpoint
	if(rates && gp->getType()=="SimulaVariable"){
		gp->getRate(t,sv);
	}else{
		gp->get(t,sv);
	}
	total+=sv;
}
std::string RootTotal::getName()const{
	return "rootTotal";
}


RootTotal2::RootTotal2(SimulaDynamic* pSD):DerivativeBase(pSD),rates(true)
{
	//parents name
	parent=pSD->getParent()->getName();
	//are we getting rates or not?
	if(pSD->getType()!="SimulaVariable")rates=false;
	//check if parent is datapoints or grandparent is datapoints
	if(pSD->existingSibling("rootType")){
		mode=1;
	}else{
		mode=2;
	}
	//the name of the variable that needs to be summed up.
	name=pSD->getName();
	//insert the 'Segment' following nameing rules: rootSegment - root - rootSystem
	name.insert(4,"Segment");
	//last datapoint and growthpoint
	first=pSD->getParent(mode)->getChild("dataPoints");
	if(first->getNumberOfChildren(pSD->getStartTime())<1) msg::error("RootTotal2: no datapoints found.");
	if(mode==1){
		first=first->getFirstChild(pSD->getStartTime())->getChild(name);//todo get first behaves as existingFirst.
	}else{
		first=first->getFirstChild(pSD->getStartTime())->getChild(parent)->getChild(name);
	}
	pSD->checkUnit(first->getUnit());
	gp=pSD->getParent(mode)->getChild("growthpoint");
	if(mode==1){
		gp=gp->getChild(name);
	}else{
		gp=gp->getChild(parent)->getChild(name);
	}
	pSD->checkUnit(gp->getUnit());
}
void RootTotal2::calculate(const Time &t,double &total){
	//reset
	total=0;
	double sv(0);
	for(SimulaBase * f(first);f!=nullptr;f=f->followChain(t)){
		if(rates){
			f->getRate(t,sv);
		}else{
			f->get(t,sv);
		}
		total+=sv;
		if(f==gp) break; //just in case growthpoint does not have a chain rull that returns nullptr
	}
}
std::string RootTotal2::getName()const{
	return "rootTotal.v2";
}


RootSystemTotal::RootSystemTotal(SimulaDynamic* pSD):DerivativeBase(pSD),rates(false)
{
	//name variable that is going to be summed up
	name=pSD->getName();
	//remove 'System' from the name and anything behind ';'
	name.erase(4,6);
	std::string::size_type pos(name.find_first_of(';',4));
	if(pos!=std::string::npos) name.erase(pos);
	//alternative to ; as it is annoying name in bash programming
	pos=name.find_first_of('_',4);
	if(pos!=std::string::npos) name.erase(pos);
	//check if parent is datapoints or grandparent is datapoints
	parent=pSD->getParent()->getName();
	if(pSD->existingSibling("rootType")){
		mode=1;
	}else{
		mode=2;
	}
	//are we getting rates or not?
	if(pSD->getType()=="SimulaVariable") rates=true;
	//get a pointer to the root simulator and to it's branches
	rootSimulator=pSD->getParent(mode);
	if(mode==2)   rootSimulator=rootSimulator->getChild(parent);
	rootSimulator=rootSimulator->existingChild(name);
	SimulaBase *p;
	if(rootSimulator==nullptr){
		//look if should sum up something for growth points
		rootSimulator=pSD->getParent(mode)->getChild("growthpoint");
		if(mode==2)   rootSimulator=rootSimulator->getChild(parent);
		p=rootSimulator->existingChild(name);
		if(p==nullptr){
			//insert 'segment' if we are only doing the growthpoint
			name.insert(4,"Segment");
			p=rootSimulator->existingChild(name);
		}
		rootSimulator=p;
		//check if it exists
		if(rootSimulator==nullptr) {
			msg::error("RootSystemTotal: can't find object with name "+name);
		}else{
			pSD->checkUnit(rootSimulator->getUnit());
		}
	}
	branches=pSD->getParent(mode)->getChild("branches");
	//check if we will only include a certain root class
	std::string rootClasses("includedRootClasses");
	p=pSD->existingChild(rootClasses);
	if(p){
		p->get(rootClasses);
		std::string thisRootsRootClass;
		pSD->getParent(mode)->getChild("rootType")->get(thisRootsRootClass);
		//build list (we could search the string directly, but we want to be sure that spaces in names are allowed and that laterals is not find if lateralsOfPrimaryRoot is present
		std::set<std::string> list;
		std::string::size_type pos(0);
		while (pos<rootClasses.size()){
			list.insert(nextWord(rootClasses,pos,','));
		}
		//search list
		if(list.find(thisRootsRootClass)==list.end()) rootSimulator=nullptr;
	}

}
void RootSystemTotal::calculate(const Time &t,double &total){
	//start with current root
	total=0;
	if(rootSimulator){
		if(rates){
			rootSimulator->getRate(t,total);
		}else{
			rootSimulator->get(t,total);
		}
	}
	//add the sum of all the rootSystems of all branches of all branch types
	///@todo used getAll() as long as we can not get iterators from the database directly
	double sv;
	SimulaBase::List branchtypes;
	branches->getAllChildren(branchtypes,t);
	for(auto &it:branchtypes){
		SimulaBase::List branchnumber;
		(it)->getAllChildren(branchnumber,t);
		SimulaBase * p;
		if(mode==1){
			for(auto & it2:branchnumber){
				p=(it2)->existingChild(pSD->getName());
				if(p){
					if(rates){
						p->getRate(t,sv);
					}else{
						p->get(t,sv);
					}
					total+=sv;
				}
			}
		}else{
			for(auto & it2:branchnumber){
				p=(it2)->existingChild(parent);
				if(p)p=p->existingChild(pSD->getName());
				if(p){
					if(rates){
						p->getRate(t,sv);
					}else{
						p->get(t,sv);
					}
					total+=sv;
				}
			}
		}
	}
}
std::string RootSystemTotal::getName()const{
	return "rootSystemTotal";
}


DerivativeBase * newInstantiationRootTotal(SimulaDynamic* const pSD){
   return new RootTotal(pSD);
}
DerivativeBase * newInstantiationRootTotal2(SimulaDynamic* const pSD){
   return new RootTotal2(pSD);
}
DerivativeBase * newInstantiationRootSystemTotal(SimulaDynamic* const pSD){
   return new RootSystemTotal(pSD);
}
class AutoRegisterTotalsInstantiationFunctions {
public:
   AutoRegisterTotalsInstantiationFunctions() {
      // register the maker with the factory
		BaseClassesMap::getDerivativeBaseClasses()["rootTotal"] = newInstantiationRootTotal;
		BaseClassesMap::getDerivativeBaseClasses()["rootTotal.v2"] = newInstantiationRootTotal2;
		BaseClassesMap::getDerivativeBaseClasses()["rootTotalRates"] = newInstantiationRootTotal;
		BaseClassesMap::getDerivativeBaseClasses()["rootNutrientTotal"] = newInstantiationRootTotal;
		BaseClassesMap::getDerivativeBaseClasses()["rootSystemTotal"] = newInstantiationRootSystemTotal;
		BaseClassesMap::getDerivativeBaseClasses()["rootSystemNutrientTotal"] = newInstantiationRootSystemTotal;
		BaseClassesMap::getDerivativeBaseClasses()["rootSystemTotalRates"] = newInstantiationRootSystemTotal;
   }
};

static AutoRegisterTotalsInstantiationFunctions p;
