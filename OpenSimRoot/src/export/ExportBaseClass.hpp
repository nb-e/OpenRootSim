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
/*
 * SHORT DESCRIPTION: Base class for export modules. If you are writing an
 * export module for simroot - you should inherit from this class and
 *
 */
#ifndef EXPORTBASECLASS_HPP_
#define EXPORTBASECLASS_HPP_
#include "../engine/DataDefinitions.hpp"
#include <map>
#include <vector>
#include <set>
class SimulaBase;

//default export class
class ExportBase{
public:
	ExportBase(std::string module);
	virtual ~ExportBase();
	virtual void initialize()=0;
	virtual void run(const Time &t)=0;
	virtual void finalize()=0;
	std::string getName()const;
	bool &enabled();
	Time getStartTime()const;
	Time getEndTime()const;
	Time getIntervalTime()const;
	Time &getCurrentTime();
	Time getNextOutputTime();
protected:
	std::string moduleName;
	Time startTime, endTime, intervalTime, currentTime;
	std::set<Time> outputTimes;
	SimulaBase* controls;
	bool runModule;
};

//global map for registration of export classes.
//typedefinition of instantiation function of rate objects
typedef ExportBase* (*exportBaseInstantiationFunction)();
//instantiation of map that lists all rate classes
extern std::map<std::string, exportBaseInstantiationFunction > exportBaseClassesMap;

typedef  std::vector<ExportBase*> ModuleList;


#endif /*EXPORTBASECLASS_HPP_*/
