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

#include "GenerateRootNodes.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaConstant.hpp"
#include "../../math/VectorMath.hpp"


//creates nodes based on position in the growthpoint
RootDataPoints::RootDataPoints(SimulaBase* const pSB):
	ObjectGenerator(pSB),
	growthpoint(nullptr),
	length(nullptr),
	lastUpdated(-1),
	lastPosition(-10000,-10000,-10000),
	timeLastPoint(pSB->getStartTime()),
	count(0)
{
}

double RootDataPoints::defaultSpatialIntegrationLength(-1);

void RootDataPoints::initialize(const Time &t){
	std::string test=pSB->getParent()->getName();
	if(test.find("Template",0)!=std::string::npos){
		lastUpdated=1e10;
		return;
	}
	//last updated needs to check, the input file may already contain some branches, we continue from there
	SimulaBase *p=pSB->getLastChild();
	if(p){
		lastUpdated=p->getStartTime();
		timeLastPoint=lastUpdated;
		count=pSB->getNumberOfChildren();
	}

	//cast the growthpoint to SimulaPoint so we can access the data in the table.
	growthpoint=dynamic_cast<SimulaPoint*>(pSB->getSibling("growthpoint"));
	if (growthpoint==nullptr) msg::error("rootDataPoints: failed dynamic_cast, growthpoint is not of type simulapoint");
	length=growthpoint->getChild("rootLongitudinalGrowth","cm");
	if(defaultSpatialIntegrationLength<0){
		//read default precision for spatial integration - this is a parameter in cm
		SimulaBase* o(ORIGIN->existingPath("/simulationControls/integrationParameters/defaultSpatialIntegrationLength"));
		if(o){
			o->get(defaultSpatialIntegrationLength);
			if(o->getUnit().order!=Unit("cm").order) msg::error("HeunsII: Unit for IntegrationParameters->defaultSpatialIntegrationLength must be of order length i.e. use cm or mm etc");
			//check this is unit is of order length
			//convert to meters
			defaultSpatialIntegrationLength*=o->getUnit().factor;
			//convert to cm
			defaultSpatialIntegrationLength/=Unit("cm").factor;
		}
		if(defaultSpatialIntegrationLength<=0){
			defaultSpatialIntegrationLength=1;
		}
	}
}
void RootDataPoints::generate(const Time &t){
	SimulaPoint::Table const * const table(growthpoint->getTable());
	//check if not already up to date
	if(t<=lastUpdated)	return;

	//grow the growthpoint to current time
	if(growthpoint->evaluateTime(t)) {
		Coordinate d;
		growthpoint->get(t,d);
	}else{
		return;
	}

	//loop through table and create for each entry a dataPoint node.
	SimulaPoint::Table::const_iterator it,pit,nit;
	if(pSB->getNumberOfChildren()){
		it=table->upper_bound(lastUpdated);
	}else{
		it=table->begin();
	}
	nit=it;
	for(;it!=table->end() ; ++it){
		++nit;
		if(nit==table->end() && table->size()>2) break; //skip over the last entry which is simply the growthpoint itself.
		if(it->first<lastUpdated+TIMEERROR) continue;
		if(it->first > growthpoint->getProgressTime()) {
			if(it->first<(t-MAXTIMESTEP)+TIMEERROR){
				msg::warning("RootDataPoints: refusing to create points based on estimated data, but the data is before t.");
			}
			//normally this should be OK, as we are just not generating any points when we are still doing this timestep
			break;
		}
		pit=it;
		if(it!=table->begin())--pit;
		//set to 0 to turn length check off
		double l1,l2,l3(0);
		length->get(it->first,l2);
		length->get(timeLastPoint,l1);
		length->get(pit->first,l3);
		if((l2-l1+l2-l3)>0.9*defaultSpatialIntegrationLength  || it==table->begin() ){
			if(l2-l1>2.0*defaultSpatialIntegrationLength) //the integration module tries to set the timestep such that the length is 1 cm, but does so based on estimates and might not always be correct.
				msg::warning("GenerateRootNodes::generate: creating nodes further than 2.0*defaultSpatialIntegrationLength="+std::to_string(2.0*defaultSpatialIntegrationLength)+" cm apart");
			//create name
			std::string name("dataPoint0000"+std::to_string(count));
			name.erase(9,name.length()-14);
			//create the datapoint
			SimulaConstant<Coordinate>* pNewDataPoint(new SimulaConstant<Coordinate>(name,pSB,it->second.state,it->first));
			//copy standard root children from the root template
			pNewDataPoint->copyAttributes(it->first, ORIGIN->getChild("dataPointTemplate"));
			//add to global list
			SimulaBase::storePosition(pNewDataPoint);
			//update lastPosition and count number
			lastPosition=it->second.state;
			timeLastPoint=it->first;
			count++;

		}
		lastUpdated=it->first; //note this is the time of the last entry in the table without estimate the last time it was called, and not the time it was called at (as stored in the ObjectGenerator).
	}

}

//the maker function for instantiation of the class
ObjectGenerator * newInstantiationRootDataPoints(SimulaBase* const pSB){
   return new RootDataPoints(pSB);
}
/* calculate root segment age */
	std::string RootSegmentAge::getName()const{
		return "rootSegmentAge";
	}
	RootSegmentAge::RootSegmentAge(SimulaDynamic* pSD):DerivativeBase(pSD){

	}
	void RootSegmentAge::calculate(const Time &t,double &age){
		age=t-pSD->getStartTime();
	}

	DerivativeBase * newInstantiationRootSegmentAge(SimulaDynamic* const pSD){
	   return new RootSegmentAge(pSD);
	}



//==================registration of the classes=================
class AutoRegisterObjectGeneratorInstantiationFunctions {
public:
   AutoRegisterObjectGeneratorInstantiationFunctions() {
	  BaseClassesMap::getObjectGeneratorClasses()["rootDataPoints"] = newInstantiationRootDataPoints;
	  BaseClassesMap::getDerivativeBaseClasses()["rootSegmentAge"] = newInstantiationRootSegmentAge;
   }
};

static AutoRegisterObjectGeneratorInstantiationFunctions p97hhh57765v76;
