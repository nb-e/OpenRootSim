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

#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"
#include "RootSegmentDryWeight.hpp"



RootSegmentDryWeight::RootSegmentDryWeight(SimulaDynamic* pSD):TotalBase(pSD),mode(0)
{
	//plant type and root type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type
	std::string rootType;
	pSD->getParent(3)->getChild("rootType")->get(rootType);


	//root segment volume simulator
	std::string name(pSD->getName());
	if(name=="rootSegmentDryWeight"){
		//if steel and cortex are simulated, change the mode to adding upt two values
		volumeSimulator=pSD->existingSibling("rootSegmentDryWeightSteel");
		if(volumeSimulator){
			//add up steel and cortex
			mode=1;
			densitySimulator=pSD->existingSibling("rootSegmentDryWeightCortex");
		}else{
			volumeSimulator=pSD->getSibling("rootSegmentVolume");
			densitySimulator=pSD->existingSibling("rootSegmentSpecificWeight");
			//root specific weight
			if(!densitySimulator){
				//pointer to object that gives root type specific density
				densitySimulator=GETROOTPARAMETERS(plantType,rootType)->getChild("density");
			}
		}
	}else if(name=="rootSegmentDryWeightNoRCS"){
		volumeSimulator=pSD->getSibling("rootSegmentDryWeightSteel");
		mode=1;
		densitySimulator=pSD->getSibling("rootSegmentDryWeightCortexNoRCS");
	}else if(name=="rootSegmentDryWeightSteel"){
		volumeSimulator=pSD->getSibling("rootSegmentVolumeSteel");
		densitySimulator=pSD->existingSibling("rootSegmentSpecificWeightSteel");
		//root specific weight
		if(!densitySimulator){
			densitySimulator=GETROOTPARAMETERS(plantType,rootType)->existingChild("densitySteel");
		}
		if(!densitySimulator){
			densitySimulator=GETROOTPARAMETERS(plantType,rootType)->existingChild("density");
		}
	}else if(name=="rootSegmentDryWeightCortex"){
		volumeSimulator=pSD->getSibling("rootSegmentVolumeCortex");
		densitySimulator=pSD->existingSibling("rootSegmentSpecificWeightCortex");
		if(!densitySimulator){
			densitySimulator=GETROOTPARAMETERS(plantType,rootType)->existingChild("densityCortex");
		}
		if(!densitySimulator){
			densitySimulator=GETROOTPARAMETERS(plantType,rootType)->existingChild("density");
		}
	}else if(name=="rootSegmentDryWeightCortexNoRCS"){
		volumeSimulator=pSD->getSibling("rootSegmentVolumeCortexNoRCS");
		densitySimulator=pSD->existingSibling("rootSegmentSpecificWeightCortex");
		if(!densitySimulator){
			densitySimulator=GETROOTPARAMETERS(plantType,rootType)->existingChild("densityCortex");
		}
		if(!densitySimulator){
			densitySimulator=GETROOTPARAMETERS(plantType,rootType)->existingChild("density");
		}
	}else{
		msg::error("RootSegmentDryWeight: Unknown mode for "+name);
	}

	dw=0;
}
void RootSegmentDryWeight::calculate(const Time &t,double &vol){
	//segment volume
	volumeSimulator->get(t,vol);
	//segment density
	double dens;
	densitySimulator->get(t,dens);
	if(mode){
		//add up cortex and steel dw
		vol+=dens;
	}else{
		//segment weight
		vol*=dens;
	}
}
bool RootSegmentDryWeight::postIntegrationCorrection(SimulaVariable::Table & data){
	//but here for backwards compatibility reasons
	msg::warning("RootSegmentDryWeight::postIntegrationCorrection: using this as a SimulaVariable object is known to cause inaccuracies.");
	bool r(false);
	//iterators
	SimulaVariable::Table::iterator eit(data.end()); --eit;
	SimulaVariable::Table::iterator pit(eit); --pit;
	Time dt(eit->first-pit->first);
	double rate((dw-pit->second.state)/dt);
	if(pit->second.rate!=rate){
		pit->second.rate=rate;
		eit->second.rate=rate;
		r=true;
	}
	return r;
}
std::string RootSegmentDryWeight::getName()const{
	return "rootSegmentDryWeight";
}

DerivativeBase * newInstantiationRootSegmentDryWeight(SimulaDynamic* const pSD){
   return new RootSegmentDryWeight(pSD);
}

RootSegmentSpecificWeight::RootSegmentSpecificWeight(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//get the root type
	std::string rootType;
	pSD->getParent(3)->getChild("rootType")->get(rootType);
	//pointer to object that gives root type specific density
	densitySimulator=GETROOTPARAMETERS(plantType,rootType)->getChild("density");
	if(densitySimulator->getType()!="SimulaConstant<double>") msg::error("RootSegmentSpecificWeight: assuming root density is constant");
}
void RootSegmentSpecificWeight::calculate(const Time &t,double &sw){
	///@todo current specific weight is same for the whole root. This may change!!!
	///@todo this should get the average specific weight of both nodes of the segment
	densitySimulator->get(t,sw);
}
std::string RootSegmentSpecificWeight::getName()const{
	return "rootSegmentSpecificWeight";
}

DerivativeBase * newInstantiationRootSegmentSpecificWeight(SimulaDynamic* const pSD){
   return new RootSegmentSpecificWeight(pSD);
}



//==================registration of the classes=================
class AutoRegisterRootDryWeightInstantiationFunctions {
public:
   AutoRegisterRootDryWeightInstantiationFunctions() {
	  BaseClassesMap::getDerivativeBaseClasses()["rootSegmentDryWeight"] = newInstantiationRootSegmentDryWeight;
	  BaseClassesMap::getDerivativeBaseClasses()["rootSegmentSpecificWeight"] = newInstantiationRootSegmentSpecificWeight;
   }
};



// our one instance of the proxy
static AutoRegisterRootDryWeightInstantiationFunctions p9675876558;


