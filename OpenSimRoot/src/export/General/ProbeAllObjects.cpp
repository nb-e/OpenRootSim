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
#include "../../export/General/ProbeAllObjects.hpp"

#include <ostream>
#include "../../cli/ANSImanipulators.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaTimeDriven.hpp"
#include "../../math/MathLibrary.hpp"
#include "../../engine/Database.hpp"

//Probe all objects, but output none
ProbeAllObjects::ProbeAllObjects() :
		ExportBase("probeAllObjects"), probe(ORIGIN), maxSearchDepth(3), currentSearchDepth(0), variableList(), skipList()
{
	///@todo turned this on by default to run this always, since the model is much faster if we probe all at 0.1 timesteps. This should be fixed in the future.
	if(!runModule){
		msg::warning("Enabling Probe All Objects for current the model is faster and more accurate if \"carbonAllocation2Shoot\" and \"plantNutrientUptake\" is called first. Enable this in parameter section to get rid of this message.",1);
		runModule=true;
		if(intervalTime<1e-5) intervalTime=MINTIMESTEP;
		variableList.insert("carbonAllocation2Shoot");
		variableList.insert("plantNutrientUptake");
	}

}

void ProbeAllObjects::initialize() {
	//depth to follow path
	SimulaBase*pp = ORIGIN->getChild("simulationControls")->getChild("outputParameters")->existingChild("probeAllObjects");
	if (pp) {
		SimulaBase* p = pp->existingChild("searchingDepth");
		if (p) {
			p->get(maxSearchDepth);
		}

		//variable list
		std::string v;
		std::string::size_type pos(0);
		if(variableList.empty()){
			p=pp->existingChild("requestedVariables");
			if(p){
				p->get(v);
				while (pos < v.size()) {
					variableList.insert(nextWord(v, pos, ','));
				}
			}
		}

		//variables to skip
		p=pp->existingChild("skipTheseVariables");
		if (p) {
			p->get(v);
			pos = 0;
			while (pos < v.size()) {
				skipList.insert(nextWord(v, pos, ','));
			}
		}

	}
	//intervalTime = 0.2; //overriding the interval time this should be equal to the timestep as long as there is a speed gain in doing so for the model .
}

void ProbeAllObjects::finalize() {
}


void ProbeAllObjects::run(const Time &t) {
	//skip if this is a template
	if (probe->getName().find("Template", 0) != std::string::npos)
		return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;

	//name
	std::string name = probe->getName();
	if (skipList.find(name) != skipList.end())
		return;

	std::set<std::string>::iterator it(variableList.find(name));
	if (it != variableList.end() || variableList.empty() ) {
		//dummy variables
		Coordinate cd;
		double dd;

		//probe this object
		std::string type = probe->getType();
		if (type == "SimulaPoint") {
			probe->get(t, cd);
			//} else if (type.substr(0, 14) == "SimulaConstant") {
			//	; //continue constants don't need to be probed for
		} else if (type == "SimulaVariable") {
			probe->get(t, dd);
		} else if (type == "SimulaDerivative<State>") {
			probe->get(t, dd);
		} else if (type == "SimulaDerivative<time>") {
			probe->get(t, dd);
		} else if (type == "SimulaDerivative<Coordinate>") {
			probe->get(t, cd);
		}
	}

	//searchdepth
	if (currentSearchDepth < maxSearchDepth) {
		//probe the attributes of this object
		SimulaBase::List allattributes;
			probe->getAllChildren(allattributes, t);
			++currentSearchDepth;
			SimulaBase *pp(probe);
			for (auto it = allattributes.rbegin(); it != allattributes.rend(); ++it) {
				probe = *it;
				//for debugging only
				//std::cout<<std::endl<<probe->getName()<<" "<<currentSearchDepth;
				run(t);
			}
			probe = pp;
			--currentSearchDepth;
		}
}

