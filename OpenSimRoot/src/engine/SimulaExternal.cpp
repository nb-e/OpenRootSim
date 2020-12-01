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

#include "SimulaExternal.hpp"
#include "SimulaDynamic.hpp"
#include "BaseClasses.hpp"
#include "../cli/Messages.hpp"

SimulaExternal::SimulaExternal(const std::string &newName, SimulaBase* newAttributeOf, const std::string &dF, const Unit &newUnit, const Time &newstartTime, const Time &newEndTime):
SimulaTimeDriven(newName, newAttributeOf, dF, newUnit, newstartTime, newEndTime),reEntryLocked_(false)
{}

void SimulaExternal::get(const Time &t, double &var) {
	/*do the calculations*/
	if(!reEntryLocked_){
		reEntryLocked_=true;
		getRateCalculator()->calculate(t, var);
		reEntryLocked_=false;
	}else{
		var=0; // this value is not used anyway, as we simply try to update the table. Solution is not generic, but works for SWMS3D.
		msg::warning("SimulaExternal:: locked, and it has no predictor. This is likely to cause extrapolation warnings. ");
	}
	if(multiplier_) {
		double m;
		multiplier_->get(m);
		var*=m;
	}
}

SimulaBase* SimulaExternal::createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
      msg::error("SimulaExternal can not be copied safely");
      return nullptr;
}


std::string SimulaExternal::getType() const {
	return "SimulaExternal";
}

void SimulaExternal::getXMLtag(Tag &tag){
	//name
	tag.name="SimulaExternal";
	tag.closed=true;
	//attributes
	tag.attributes.clear();
	tag.attributes["name"]=getName();
	tag.attributes["prettyName"]=getPrettyName();
	tag.attributes["unit"]=getUnit().name;
	if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
	if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());
	tag.attributes["function"]=getRateFunctionName();
	if(integrationF) tag.attributes["integrationFunction"]=integrationF->getName();
	if(!collectGarbage_) tag.attributes["garbageCollectionOff"]="true";
	std::string on(getObjectGeneratorName());
	if(!on.empty()) tag.attributes["objectGenerator"]=on;
	//no data
	//children
	if(getNumberOfChildren()) tag.closed=false;

}



