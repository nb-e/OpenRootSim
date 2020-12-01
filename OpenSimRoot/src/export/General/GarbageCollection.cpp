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
#include "../../export/General/GarbageCollection.hpp"

#include <ostream>
#include "../../cli/ANSImanipulators.hpp"
#include "../../engine/SimulaBase.hpp"
#include "../../engine/Origin.hpp"
#include "../../math/MathLibrary.hpp"
#include "../../engine/Database.hpp"

//Probe all objects, but output none
GarbageCollection::GarbageCollection():ExportBase("garbageCollection"),probe(ORIGIN),lacktime(0){}
void GarbageCollection::initialize(){
	SimulaBase *lt=ORIGIN->getChild("simulationControls")->getChild("outputParameters")->getChild("garbageCollection")->existingChild("lackTime");
	if(lt){
		lt->get(lacktime);
	}else{
		lacktime=this->intervalTime;
		msg::warning("GarbageCollection: no 'lackTime' given. Assuming it to be equal to interval time.");
	}
}
void GarbageCollection::finalize(){}

void GarbageCollection::run(const Time &t){
	//this->update(t);
	if(t>lacktime) this->collectGarbage(t-lacktime);
}

void GarbageCollection::collectGarbage(const Time &t){
	//skip if this is a template
	if (probe->getName().find( "Template" , 0 ) != std::string::npos) return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;

	//probe this object
	probe->collectGarbage (t);

	//probe the attributes of this object
	SimulaBase::List allattributes;
		probe->getAllChildren(allattributes);
		for (auto& it:allattributes){
			//for debugging purpose:: std::cout<<(*it)->getName()<<std::endl<<std::flush;
			SimulaBase* pp(probe);
			probe=it;
			this->collectGarbage(t);
			probe=pp;
		}
}

void GarbageCollection::update(const Time &t){
	//skip if this is a template
	if (probe->getName().find( "Template" , 0 ) != std::string::npos) return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "Parameter" , 0 ) != std::string::npos) return;

	//probe this object
	if(probe->getType()=="SimulaVariable"){

		msg::warning("GrabageCollection: probing object");
		double v;
		probe->get (t,v);
	}

	//probe the attributes of this object
	SimulaBase::List allattributes;
		probe->getAllChildren(allattributes,t);
		for (auto & it:allattributes){
			//for debugging purpose:: std::cout<<(*it)->getName()<<std::endl<<std::flush;
			SimulaBase* pp(probe);
			probe=it;
			this->update(t);
			probe=pp;
		}
}

