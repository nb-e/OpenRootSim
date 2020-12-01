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
#include "../../export/General/PrimeModel.hpp"

#include <ostream>
#include "../../cli/ANSImanipulators.hpp"
#include "../../engine/SimulaBase.hpp"
#include "../../engine/Origin.hpp"
#include "../../math/MathLibrary.hpp"
#include "../../engine/Database.hpp"

//A module which tries to query the objects in a certain order to get a faster and more accurate solution
//I do not think this module works in anyway.
PrimeModel::PrimeModel():ExportBase("primeModel"),probe(ORIGIN),currentSearchDepth(0){}
void PrimeModel::initialize(){
	msg::warning("PrimeModel: This is an experimental module, which I think makes no real difference.");
}
void PrimeModel::finalize(){}

void PrimeModel::run(const Time &t){
	//skip if this is a template
	if (probe->getName().find( "Template" , 0 ) != std::string::npos) return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;

	//dummy variables
	Coordinate cd;
	double dd;

	//probe this object
	if ((probe->getName().find( "otential" , 0 ) != std::string::npos) ||
		(probe->getName().find( "ptimal" , 0 ) != std::string::npos) ||
		(probe->getName().find( "inimal" , 0 ) != std::string::npos) )
	{
		//note that this might in theory give an error when the probe does not simulate d. but so far we should be OK.
//		if(probe->getType()=="SimulaVariable"){
		    probe->get(t,dd);
//		}
	}

	//probe the attributes of this object
	if(currentSearchDepth<4){
		++currentSearchDepth;
		SimulaBase::List allattributes;
		probe->getAllChildren(allattributes,t);
		SimulaBase* pp(probe);
		for (auto &it:allattributes){
			probe=it;
			run(t);
		}
		probe=pp;
		--currentSearchDepth;
	}
}
