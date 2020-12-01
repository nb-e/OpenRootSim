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
#include "SimulaGrid.hpp"

#include <typeinfo>
#include "SimulaBase.hpp"
#include <string>
#include <map>
#include "DataDefinitions/Units.hpp"
#include "../cli/Messages.hpp"
#include "../tools/StringExtensions.hpp"
#include "../math/InterpolationLibrary.hpp"
#include "../math/MathLibrary.hpp"
#include "../math/VectorMath.hpp"
#include "Database.hpp"

SimulaGrid::SimulaGrid(const std::string &newName, SimulaBase* newAttributeOf,
		const Unit &newUnit, const Time &newstartTime, const Time &newEndTime) :
	SimulaBase(newName, newAttributeOf, newUnit, newstartTime, newEndTime),
	//multiplier
			useMultiplier(true), multiplier(nullptr),
			interpolationMethod(GridInterpolation::triLinear),param(-1),dparam1(0),dparam2(0),dparam3(0) {
}


SimulaGrid::SimulaGrid(SimulaBase* newAttributeOf, const Time &newstartTime,
		const SimulaGrid* copyThis) :
	SimulaBase(newAttributeOf, newstartTime, copyThis),
	//multiplier
			useMultiplier(true), multiplier(nullptr),
			interpolationMethod(copyThis->interpolationMethod),param(-1),dparam1(0),dparam2(0),dparam3(0),	table(copyThis->table)
{
}


void SimulaGrid::get(const Time &t, const Coordinate & pos, double &y) {
//@todo could implement cach here for position, like in simuladerivative.
	switch (interpolationMethod) {
		case GridInterpolation::triLinear:
			triLinearInterpolation(pos,y);
			break;
		case GridInterpolation::naturalNeighbour:
			naturalNeighbourInterpolation(pos,y);
			break;
		case GridInterpolation::nearestNeighbour:
			nearestNeighbourInterpolation(pos,y);
			break;
		case GridInterpolation::kriging3D:
			kriging3D(pos,y);
			break;
		case GridInterpolation::inverseDistanceWeightedAverage:
			inverseDistanceWeightedAverage(pos,y);
			break;
		case GridInterpolation::diffuseSpots:
			diffuseSpots(pos,y);
			break;
		default:
			// Should only get here if someone extends GridInterpolation without updating this switch
			msg::error("SimulaGrid::get: unknown interpolation method");
			break;
	}


	if (useMultiplier) {
		if (!multiplier)
			multiplier = this->existingChild("multiplier");
		if (multiplier) {
			double m;
			multiplier->get(m);
			y *= m;
		} else {
			useMultiplier = false;
		}
	}
}


void SimulaGrid::triLinearInterpolation(const Coordinate & pos, double &y){
	msg::error("SimulaGrid: interpolationMethod triLinearInterpolation not implemented");
	//find nearest point
	//find corner in y direction
	//find corner in x direction
	//find corner in z direction
	//find corner in yx direction
	//find corner in yz direction
	//find corner in xz direction
	//find corner in xyz direction
	//interpolate

}
void SimulaGrid::naturalNeighbourInterpolation(const Coordinate & pos, double &y){
	msg::error("SimulaGrid: interpolationMethod naturalNeighbour not implemented");
}
void SimulaGrid::nearestNeighbourInterpolation(const Coordinate & pos, double &y){

	//check table length
	if(table.empty()) msg::error("SimulaGrid: table is empty: there is not data in grid "+getName());
	//find nearest point
	double d, md=1e9;
	//todo, this could be faster if we slice the y space, for now the number of points is small anyway.
	for(Table::iterator l(table.begin()); l!=table.end(); ++l){
		d=vectorlength(pos,l->first);
		if(md>d ){
			md=d;
			y=l->second;
		}
	}
}
void SimulaGrid::inverseDistanceWeightedAverage(const Coordinate & pos, double &y){
    if(param<1) {
		param=1;
    	SimulaBase *p=existingChild("power");
    	if(p){
    		p->get(dparam3);
    	}else{
    		dparam3=1.;
    	}
    	p=existingChild("backgroundlevel");
    	if(p){
    		p->get(dparam1);
    	}else{
    		dparam1=0;
    	}

    }
	//check table length
	if(table.empty()) msg::error("SimulaGrid: table is empty: there is not data in grid "+getName());
	//find nearest point
	double w, tot=0;y=0;
	//todo, this could be faster if we slice the y space, for now the number of points is small anyway.
	for(Table::iterator l(table.begin()); l!=table.end(); ++l){
		w=vectorlength(pos,l->first);
		if(w<1e-3) w=1e-3;
		w=1./pow(w,dparam3);
		y+=	w*(l->second-dparam1);
		tot+=w;
	}
	y/=tot;
	y+=dparam1;
}
void SimulaGrid::diffuseSpots(const Coordinate & pos, double &y){
    if(param<1) {
    	SimulaBase *p=existingChild("power");
    	if(p){
    		p->get(dparam3);
    	}else{
    		dparam3=1;
    	}
    	p=existingChild("backgroundlevel");
    	if(p){
    		p->get(dparam1);
    	}else{
    		dparam1=0;
    	}
    	p=existingChild("diffusionCoefficient");
    	if(p){
    		p->get(dparam2);
    	}else{
    		dparam2=1;
    	}
		param=1;
    }
	//check table length
    //diffuse

	if(table.empty()) msg::error("SimulaGrid: table is empty: there is not data in grid "+getName());
	//find nearest point
	double w;//tot=0;
	y=0;
	//todo, this could be faster if we slice the y space, for now the number of points is small anyway.
	for(Table::iterator l(table.begin()); l!=table.end(); ++l){
		w=vectorlength(pos,l->first);
		if(w<1e-3) w=1e-3;
		w=dparam2/pow(w,dparam3);
		if(w>1) w=1;
		y+=	w*(l->second-dparam1);
		//tot+=w;
	}
	y+=dparam1;
}



void SimulaGrid::kriging3D(const Coordinate & pos, double &y){
	msg::error("SimulaGrid: interpolationMethod kriging3D not implemented");
}

void SimulaGrid::set(const Coordinate & p, const double & v) {
	table[p]=v;
}

void SimulaGrid::getRate(const Time &t, const Coordinate & pos, double &dy) {
	dy = 0;
}


void SimulaGrid::setInterpolationMethod(const std::string &method) {
	for(auto const & im : GridInterpolationNames){
		if (im.second==method) {
			interpolationMethod=im.first;
			if (method=="triLinearInterpolation"
					|| method=="naturalNeighbourInterpolation"
					|| method=="Kriging3D") {
				msg::error("SimulaGrid: interpolationMethod="
					+ method + " not implemented");
			}
			return;
		}
	}
	msg::error("SimulaGrid: unknown interpolation method '"
		+ method + "' for '" + getName()
		+ "'. Use nearestNeighbourInterpolation, inverseDistanceWeightedAverage, or diffuseSpots.");
}


std::string SimulaGrid::getType() const {
	return "SimulaGrid";
}

void SimulaGrid::getXMLtag(Tag &tag){
	//name
	tag.name="SimulaGrid";
	tag.closed=true;
	//attributes
	tag.attributes.clear();
	tag.attributes["name"]=getName();
	tag.attributes["prettyName"]=getPrettyName();
	tag.attributes["unit"]=getUnit().name;
	if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
	if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());

	// look up interpolation method name
	auto im=GridInterpolationNames.find(interpolationMethod);
	if(im!=GridInterpolationNames.end()){
		tag.attributes["interpolationMethod"]=im->second;
	} else {
		// If we get here, it means some values in GridInterpolation
		// are missing from GridInterpolationNames
		msg::error("SimulaGrid: don't known how to write XML tag for interpolation method '"
			+ std::to_string(static_cast<int>(interpolationMethod))
			+ "' in '" + getName()
			+ "'. Please file a bug at https://gitlab.com/rootmodels/OpenSimRoot.");
	}

	std::string on(getObjectGeneratorName());
	if(!on.empty()) tag.attributes["objectGenerator"]=on;
	//data
	if(!table.empty()){
		std::string d;
		for(auto &dp:table){
			d+=convertToString<Coordinate>(dp.first);
			d+="\t";
			d+=convertToString<double>(dp.second);
			d+="\n";
		}
		tag.attributes["_DATA_"]=d;
		tag.closed=false;
	}
	//children
	if(getNumberOfChildren()) tag.closed=false;

}


SimulaBase* SimulaGrid::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      return new SimulaGrid(attributeOf,startTime,this);
}

