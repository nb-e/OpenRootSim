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
#include "../../export/RSML/RSML.hpp"
#include "../circle.hpp"

#include <iomanip>
#include "../../cli/ANSImanipulators.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../engine/Origin.hpp"
#include "../../math/MathLibrary.hpp"
#include "../../math/VectorMath.hpp"
#include "../../import/xmlReader/Indenting.hpp"


RSML::RSML(const std::string& nameMod):
ExportBase(nameMod),
warning(false),
pointData(true),
plantCount(0),
plantType(0),
rid(0)
{
}
RSML::~RSML(){
	this->finalize();
	//if(runModule) //unclear why this is here, or what it means
		//msg::warning("RSML::addData: Data is not simulated entering 0 in the list");
}
void RSML::loadList(const std::string  & base, SimulaBase* p){

	SimulaBase::List list;
	p->getAllChildren(list);
	for (auto &it:list){
		std::string name=base+(it)->getName();
		if((it)->hasChildren()){
			name+="/";
			loadList(name,(it));
		}else{
			variableList.insert(name);
		}
	};

}


void RSML::initialize(){
	//check parameters
	SimulaBase* sbp=controls->existingChild("includePointData");
	if(sbp)sbp->get(pointData);
	if(pointData){
		std::string param;
		std::string::size_type pos(0);
		SimulaBase *pd=controls->existingChild("pointDataVariables");
		SimulaBase* p(ORIGIN->getChild("dataPointTemplate"));
		if(pd) pd->get(param);
		if(param.size()>0){
			while (pos<param.size()) {
				std::string ps(nextWord(param,pos,','));
				if(p->existingPath(ps)){
					variableList.insert(ps);
				}else if(ps.substr(ps.size()-4)=="Rate"){
					SimulaBase *pr(p->existingPath(ps.substr(0,ps.size()-4)));
					if(pr && pr->getType()=="SimulaVariable") variableList.insert(ps);
				}
			}
		}else{
			//insert all that are in the template
			std::string name;
			loadList(name,p);
		}
		variableList.insert("plantID");
		variableList.insert("plantTypeID");
	}


	//collect list of nutrients for which we will try to write the depletionzone output
	for(std::set<std::string>::iterator nit(variableList.begin()); nit!=variableList.end();++nit){
		std::string nn(*nit);
		std::size_t pos(nn.find("/radiusDepletionZone"));
		if(pos!=std::string::npos){
			nn.erase(pos,std::string::npos);
			nutrients.insert(nn);
		}
	}

}

void RSML::finalize(){
}


void RSML::run(const Time &dt){
	msg::warning("RSML output is underconstruction and might not be complete.");
		//container for circles
		std::vector<circle> sh;
		//open the file
		std::string filename(std::to_string(dt)), filenameshort;
		filename.resize(6,0);
		std::string::size_type pos(filename.find(".",0));
		if (pos<3){
			size_t n(3-pos);
			for(std::size_t i=0; i<n; i++) filename.insert(0,"0");
			filename.resize(6,0);
		}
		std::string extension(".rsml");
		filenameshort=filename+extension;
		filename="roots"+filename+extension;
		std::ofstream os( filename.c_str() );
		//check whether opening was succesful
		if ( !os.is_open() ) msg::error("RSML: Failed to open "+filename);
		//tell os to write fixed numbers instead of scientific notation
		os<<std::fixed;
		os.precision(3);
		//dump the model into the file
		indent()=0;
		//write header
		os	<<"\
<?xml version='1.0' encoding='UTF-8'?>\n\
<rsml xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xsi:noNamespaceSchemaLocation=\"https://raw.githubusercontent.com/RootSystemML/RSMLValidator/master/rsml.xsd\">\n\
	<metadata>\n\
		<version>1</version>\n\
		<unit>cm</unit>\n\
		<resolution>1</resolution>\n\
		<software>OpenSimRoot</software>\n\
        <last-modified>0001-01-01T12:00:00.0000000-00:00</last-modified>\n\
        <user>isprivate</user>\n\
        <file-key>noset</file-key>\n\
	</metadata>\n\
	<scene>";

		//populate vector with circles
		plantCount=0;
		rid=0;
		writePlants(dt,ORIGIN,os);

		//write the footer
		os<<"\n\t\t</plant>\n\t</scene>\n</rsml>\n";

		//close the file
		os.close();
}


void RSML::writePlants(const Time &t, SimulaBase* probe, std::ofstream  & os) {
	//skip if this is a template
	///@todo, there maybe an case insensitive flag that we can use here?
	if (probe->getName().find("Template", 0) != std::string::npos)
		return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;
	if (probe->getName().find("parameters", 0) != std::string::npos)
		return;
	if (probe->getName().find("Parameters", 0) != std::string::npos)
		return;
	if (probe->getName().find("growthpoint", 0) != std::string::npos)
		return;
	if (probe->getName().find("soil", 0) != std::string::npos)
		return;
	if (probe->getName().find("atmosphere", 0) != std::string::npos)
		return;

	//probe this object
	if (probe->getName() == "dataPoints" ) {
        ++rid;
		os<<"\n\t\t\t<root id=\""<<rid<<"\" label=\""<<probe->getParent()->getName()<<"\" >";
		os<<"\n\t\t\t\t<geometry>\n\t\t\t\t\t<polyline>";

		//for each datapoint insert a circle
		SimulaBase::List l;
		probe->getAllChildren(l,t);
		//add growthpoint to bottom of the list
		l.push_back(probe->getSibling("growthpoint"));
		Coordinate refpos;
		probe->getBase(t,refpos);
		Coordinate oldPos(-1.,-1.,-1.);
		for (auto it:l){
			Coordinate pos;
			it->get(t,pos);
			if( (pos-oldPos).length()<1e-2) pos.y-=0.001;
			oldPos=pos;
			pos+=refpos;
			os<<"\n\t\t\t\t\t\t<point x=\""<<pos.x<<"\" y=\""<<pos.z<<"\" z=\""<<pos.y<<"\" />";
		}

		os<<"\n\t\t\t\t\t</polyline>";
		os<<"\n\t\t\t\t</geometry>";
		os<<"\n\t\t\t\t<functions>";
		double d;
		for (auto &nit:variableList) {
			if (nit == "plantID") {
				//do nothing
				continue;
			}
			if (nit == "plantTypeID") {
				//add plant id
				//do nothing
				continue;
			}
			if (nit == "plantTypeID") {
				//add plant id
				//do nothing
				continue;
			}
			os<<"\n\t\t\t\t\t<function domain=\"polyline\" name=\""<<nit<<"\">";

			for (auto it:l){
				SimulaBase* p(it->existingPath(nit));
				if (p) {
					p->get(t, d);
				}else{
					if(nit.substr(nit.size() - 4) == "Rate"){
						p = it->existingPath(nit.substr(0, nit.size() - 4));
						if (p) {
							p->getRate(t, d);
						}
					}
				}
				if (p) {
					os<<"\n\t\t\t\t\t\t<sample value=\""<<d<<"\" />";
				}else{
					///todo RSML schema does not allow NA so we write the previous value of d.
					///os<<"\n\t\t\t\t\t\t<sample value=\"NA\" />";
					os<<"\n\t\t\t\t\t\t<sample value=\""<<d<<"\" />";
				}
			}
			os<<"\n\t\t\t\t\t</function>";
		}

		os<<"\n\t\t\t\t</functions>";

		os<<"\n\t\t\t</root>";

	} else {
		//set plant type and id etc
		if (probe->getName() == "plantPosition") {
			++plantCount;
			std::string nm;
			probe->getSibling("plantType")->get(nm);
			plantType = -1;
			for (unsigned int i(0); i < plantTypes.size(); ++i) {
				if (plantTypes[i] == nm) {
					plantType = i;
					break;
				}
			}
			if (plantType < 0) {
				plantType = (int) plantTypes.size();
				plantTypes.push_back(nm);
			}


			if(plantCount>1) os<<"\n\t\t</plant>";
			os<<"\n\t\t<plant id=\""<<plantCount<<"\" label=\""<<probe->getParent()->getName()<<"\">";

		}

		//probe the attributes of this object
		SimulaBase::List allattributes;
		probe->getAllChildren(allattributes, t);
		for (auto & it : allattributes) {
			writePlants(t, it, os);
		}
	}
}

void RSML::addData(const Time &t, circle &c, SimulaBase* probe) {
	//insert optional data
	double d;
	for (std::set<std::string>::iterator it(variableList.begin());
			it != variableList.end(); ++it) {
		if (*it == "plantID") {
			//add plant id
			c.data["plantID"] = plantCount;
			continue;
		}
		if (*it == "plantTypeID") {
			//add plant id
			c.data["plantTypeID"] = plantType;
			continue;
		}
		if (*it == "plantTypeID") {
			//add plant id
			c.data["plantTypeID"] = plantType;
			continue;
		}
		SimulaBase* p(probe->existingPath(*it));
		if (p) {
			p->get(t, d);
			c.data[*it] = d;
		} else if (it->substr(it->size() - 4) == "Rate") {
			p = (probe->existingPath(it->substr(0, it->size() - 4)));
			if (p) {
				p->getRate(t, d);
				c.data[*it] = d;
			}
		}
		if (!p) {
			if (!warning)
				warning = true;
			c.data[*it] = 0;
		}
	}
}

