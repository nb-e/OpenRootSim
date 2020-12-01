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
/**
 * SHORT DESCRIPTION: Base class for export modules. If you are writing an
 * export module for simroot, you should inherit from this class.
 *
 * Output timings are controlled by four options,
 * each possibly set in either the defaults or in your output module:
 *
 * startTime, endTime, intervalTime:
 * 	Use these to run module at equal intervals within a given time range.
 * 	Specify as SimulaConstants with type="time", e.g.
 * 		<SimulaBase name="modelDump">
 * 			<SimulaConstant name="run" type="bool"> 1  </SimulaConstant>
 * 			<SimulaConstant name="timeInterval" type="time"> 1 </SimulaConstant>
 * 			<SimulaConstant name="startTime" type="time"> 0 </SimulaConstant>
 * 			<SimulaConstant name="endTime" type="time"> 3 </SimulaConstant>
 *		</SimulaBase>
 *
 * outputTimes:
 * 	Use this to run module at arbitrary specified times
 * 	Specify as a SimulaConstant with type="string" and give times as a comma separated list, e.g.
 * 		<SimulaBase name="modelDump">
 * 			<SimulaConstant name="run" type="bool"> 1 </SimulaConstant>
 * 			<SimulaConstant name="outputTimes" type="string">0.1, 0.2, 1, 2, 2.1, 3</SimulaConstant>
 * 		</SimulaBase>
 * 	If an outputTimes is specified, it overrides any start/end/interval specifications.
 *
 */
#include "../export/ExportBaseClass.hpp"

#include <ostream>
#include "../engine/SimulaBase.hpp"
#include "../engine/Origin.hpp"
#include "../engine/Database.hpp"
#include "../tools/StringExtensions.hpp"

//base class for export plugins
ExportBase::ExportBase(std::string module):
	moduleName(module),
	startTime(0),endTime(0),intervalTime(0),currentTime(0),
	controls(nullptr),runModule(false)
{
	//pointer to controls
	SimulaBase* p=ORIGIN->getChild("simulationControls")->getChild("outputParameters");
	controls=(p->existingChild("defaults"));

	//use the default
	SimulaBase *sbp;
	if(controls){
		//set startTime
		sbp=controls->existingChild("startTime");
		if(sbp)	sbp->get(startTime);

		//set endTime
		sbp=controls->existingChild("endTime");
		if(sbp)	sbp->get(endTime);

		//set intervalTime
		sbp=controls->existingChild("timeInterval");
		if(sbp)	sbp->get(intervalTime);

		sbp=controls->existingChild("outputTimes");
		if(sbp) {
			std::string raw_times;
			sbp->get(raw_times);
			std::string::size_type pos(0);
			while (pos<raw_times.size()) {
				outputTimes.insert(convertFromString<Time>(nextWord(raw_times, pos, ',')));
			}
			startTime = std::max(*outputTimes.begin(), startTime);
			endTime = std::min(*outputTimes.rbegin(), endTime);
		}
	}

	//override the default
	controls=(p->existingChild(moduleName));
	if(controls){
		//set startTime
		sbp=controls->existingChild("startTime");
		if(sbp)	sbp->get(startTime);

		//set endTime
		sbp=controls->existingChild("endTime");
		if(sbp)	sbp->get(endTime);

		//set intervalTime
		sbp=controls->existingChild("timeInterval");
		if(sbp)	sbp->get(intervalTime);

		sbp=controls->existingChild("outputTimes");
		if(sbp) {
			outputTimes.clear(); // want to replace default list not add to it
			std::string raw_times;
			sbp->get(raw_times);
			std::string::size_type pos(0);
			while (pos<raw_times.size()) {
				outputTimes.insert(convertFromString<Time>(nextWord(raw_times, pos, ',')));
			}
			startTime = std::max(*outputTimes.begin(), startTime);
			endTime = std::min(*outputTimes.rbegin(), endTime);
		}

		//set runModule boolean
		sbp=controls->existingChild("run");
		if(sbp)	{
			controls->getChild("run")->get(runModule);
		}else{
			runModule=true;
		}
	}

	//set current time
	currentTime=startTime;

	if(runModule && startTime>endTime){
		msg::error("ExportBase:: Please specify a valid start and end time for "+moduleName);
	}

}
ExportBase::~ExportBase(){}

/*void ExportBase::go(){
	if (runModule) {
		run();
	}else{
		std::cout<<"skipped"<<std::flush;
	}
}*/

std::string ExportBase::getName()const{return moduleName;}
bool &ExportBase::enabled(){return runModule;}
Time ExportBase::getStartTime()const{return startTime;}
Time ExportBase::getEndTime()const{return endTime;}
Time ExportBase::getIntervalTime()const{return intervalTime;}
Time &ExportBase::getCurrentTime(){return currentTime;}
Time ExportBase::getNextOutputTime(){
	auto next = outputTimes.upper_bound(currentTime);
	if (next == outputTimes.end()) {
		return std::min(currentTime + intervalTime,  endTime);
	}
	return *next;
}


//global list of of export modules.
std::map<std::string, exportBaseInstantiationFunction > exportBaseClassesMap;


