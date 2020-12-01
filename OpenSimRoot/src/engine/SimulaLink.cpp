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

#include "SimulaLink.hpp"
#include "DataDefinitions/Units.hpp"
#include "../cli/Messages.hpp"
//todo simulalink is through these macros "plant" aware, but should not
#include "../modules/PlantType.hpp"

SimulaLink::SimulaLink(const std::string &newName, SimulaBase* newAttributeOf, const Time &newstartTime):
	SimulaBase(newName, newAttributeOf, UnitRegistry::noUnit(), newstartTime),
	link_(nullptr),
	linkedName_(),
	useMultiplier(true),
	multiplier(nullptr),
	refTime(newstartTime)
{}

SimulaLink::SimulaLink(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaLink* copyThis):
	SimulaBase(newAttributeOf, newstartTime, copyThis),
	link_(nullptr),
	linkedName_(copyThis->linkedName_),
	useMultiplier(true),
	multiplier(nullptr),
	refTime(newstartTime)
{}

void SimulaLink::createLink(SimulaBase* link){
	link_=link;
	setUnit(link_->getUnit());
}
void SimulaLink::setLinkName(const std::string &l){
	if(link_) msg::error("SimulaLink::setLinkName: programming error: name given, but link already set");
	linkedName_=l;
}

SimulaBase* SimulaLink::getLink(){
	if(!link_){
		//set link based on context
		//TODO this is the only part of the engine that is root model specific and therefor ugly.
		//It may have to become a seperate plugin for links?
		//get name of table
		std::string name;
		if(linkedName_.size()>0){
			name=linkedName_;
		}else{
			name=this->getName();
		}
		//get name of group
		std::string group(this->getParent()->getName());
		//plant type
		std::string plantType;
		PLANTTYPE(plantType,this);

		//determine if we are doing this for the roots or the shoot
		//determine if we are doing the parameter is listed under a group
		if(name[0]=='/' || name[0]=='.' ){//path
			link_=this->getPath(name);
			refTime=0;
			//if(!link_)msg::error("SimulaLink: path \""+ name +"\"does not exist");
		}else if(this->getParent(2)->getName()=="branches"){
			//get parameters for this type of branch
			std::string rootType;
			this->getParent(3)->getChild("rootType")->get(rootType);
			std::string branchType=this->getParent()->getName();
			SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));//set pointer to table
			link_=parameters->getChild("branchList")->getChild(branchType)->getChild(name);
		}else if(this->getParent()->getName()=="shoot"){
			//shoot without group
			SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));
			//set pointer to table
			link_=parameters->getChild(name);
		}else if(this->getParent(2)->getName()=="shoot"){
			//shoot with group
			SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));
			//set pointer to table
			link_=parameters->getChild(group)->getChild(name);
		}else if(this->getParent()->getName()=="growthpoint"){
			//growthpoint without group
			//get the root type parameters
			std::string rootType;
			this->getParent(2)->getChild("rootType")->get(rootType);
			SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
			//set pointer to table
			link_=parameters->getChild(name);
		}else if(this->getParent(2)->getName()=="growthpoint"){
			//growthpoint with group
			//get the root type parameters
			std::string rootType;
			this->getParent(3)->getChild("rootType")->get(rootType);
			SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
			//set pointer to table
			link_=parameters->getChild(group)->getChild(name);
		}else if(this->getParent(3)->existingChild("rootType")){
			//datapoint without group
			//get the root type parameters
			std::string rootType;
			this->getParent(3)->getChild("rootType")->get(rootType);
			SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
			//set pointer to table
			link_=parameters->getChild(name);
		}else{
			//datapoint with group
			//get the root type parameters
			std::string rootType;
			this->getParent(4)->getChild("rootType")->get(rootType);
			SimulaBase *parameters(GETROOTPARAMETERS(plantType,rootType));
			//set pointer to table
			link_=parameters->getChild(group)->getChild(name);
		}
		setUnit(link_->getUnit());
	}
	return link_;
}

void SimulaLink::get(const Time &t, int &rc) {
	getLink()->get(t-refTime, rc);
}
void SimulaLink::get(const Time &t, const Coordinate &pos, double &rc) {
	getLink()->get(t-refTime, pos, rc);
	if(useMultiplier){
		if(!multiplier) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			double m;
			multiplier->get(t,pos,m);
			rc*=m;
		}else{
			useMultiplier=false;
		}
	}
}
void SimulaLink::get(const Time &t, Time &rc) {
	Coordinate pos;
	this->getAbsolute(t,pos);
	getLink()->get(t-refTime, pos, rc);
	if(useMultiplier){
		if(!multiplier) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			Time m;
			multiplier->get(t,m);
			rc*=m;
		}else{
			useMultiplier=false;
		}
	}
}
void SimulaLink::get(const Time &t, Coordinate &rc) {
	getLink()->get(t-refTime, rc);
	if(useMultiplier){
		if(!multiplier) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			double m;
			multiplier->get(t,m);
			rc*=m;
		}else{
			useMultiplier=false;
		}
	}
}
void SimulaLink::get(int &rc) {
	getLink()->get(rc);
}
void SimulaLink::get(Time &rc) {
	getLink()->get(rc);
	if(useMultiplier){
		if(!multiplier) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			Time m;
			multiplier->get(m);
			rc*=m;
		}else{
			useMultiplier=false;
		}
	}
}
void SimulaLink::get(Coordinate &rc) {
	getLink()->get(rc);
	if(useMultiplier){
		if(!multiplier) multiplier=this->existingChild("multiplier");
		if(multiplier) {
			double m;
			multiplier->get(m);
			rc*=m;
		}else{
			useMultiplier=false;
		}
	}
}

std::string SimulaLink::getType() const {
	if(!link_) {
		return "SimulaLink<notset>";
	}else{
		return "SimulaLink->"+link_->getType();
	}
}
Unit SimulaLink::getUnit(){
	getLink();//make sure this is linked up first
	return *unit_;
}

SimulaBase* SimulaLink::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      return new SimulaLink(attributeOf,startTime,this);
}

void SimulaLink::getXMLtag(Tag &tag){
	//name
	tag.name="SimulaLink";
	tag.closed=true;
	//attributes
	tag.attributes.clear();
	tag.attributes["name"]=getName();
	tag.attributes["prettyName"]=getPrettyName();
	//not set, inherited from link tag.attributes["unit"]=getUnit();
	if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
	if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
	if(!linkedName_.empty()) tag.attributes["linksName"]=linkedName_;

	std::string on(getObjectGeneratorName());
	if(!on.empty()) tag.attributes["objectGenerator"]=on;
	//no data
	//children
	if(getNumberOfChildren()) tag.closed=false;

}

