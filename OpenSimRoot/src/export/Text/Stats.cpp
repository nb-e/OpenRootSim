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
#include "../../export/Text/Stats.hpp"

#include <ostream>
#include <fstream>
#include <iomanip>
#include "../../cli/ANSImanipulators.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"

#include "../../engine/SimulaBase.hpp"
#include "../../engine/Origin.hpp"

#include "../../math/MathLibrary.hpp"

#include "../../import/xmlReader/Indenting.hpp"
//#include "../../FunctionLibrary/PlantType.hpp"

//Write output Statss
#define SEP "\t"
#define QUOTE "\""
Stats::Stats():ExportBase("rootStatistics"),probe(ORIGIN), variableList(),skipList(),currentSearchDepth(0){}
void Stats::initialize(){


	//variable list
	std::string v;
	SimulaBase *param(ORIGIN->getChild("simulationControls")->getChild(
			"outputParameters")->getChild("rootStatistics"));
	SimulaBase *rv = param->getChild("requestedVariables");
	if(rv) {
		rv->get(v);
		std::string::size_type pos(0);
		while (pos<v.size()){
			auto n=nextWord(v,pos,',');
			variableList.insert(n);
		}
	}

	//variables to skip
	SimulaBase *p(param->existingChild("skipTheseVariables"));
	if(p){
		p->get(v);
		std::string::size_type pos=0;
		while (pos<v.size()){
			skipList.insert(nextWord(v,pos,','));
		}
	}


}

void Stats::finalize(){
	//Write Stats footer

}

void Stats::run(const Time &t){
	//Open file
	bool openedfile=false;
	if (!os.is_open()) {
#if _WIN32 || _WIN64
		std::string ext(".xls");
#else
		std::string ext(".tab");
#endif
		std::string filename = "stats_"+convertToString(t) + ext;
		os.open(filename.c_str());
		if (!os.is_open())
			msg::error("generateStats: Failed to open " + filename);
		os<<"name"<<SEP<<"time"<<SEP<<"value"<<SEP<<"unit"<<SEP<<"label"<<SEP<<"path";
		openedfile=true;
	}


	//skip if this is a template
	if (probe->getName().find( "Template" , 0 ) != std::string::npos) return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "Parameter" , 0 ) != std::string::npos) return;

	//name
	std::string name=probe->getName();

	if (skipList.find(name)!=skipList.end()) return;

	//path
	const std::string prevPath(path);
	if(name!="origin"){
		path+="/"+name;
	}else{
		path+="/";
	}

	//find name in list and write line
	std::string type(probe->getType());
	if(!variableList.empty()){ //try all objects that simulate double
		if( variableList.find(name)!=variableList.end() ){
			double value;
			probe->get(t,value);
			//label
			std::string label;
			if(currentSearchDepth>1 && probe->getParent(2)->getName()=="plantPosition"){
				label=probe->getParent(1)->getName();
			}else if(currentSearchDepth>2 && probe->getParent(3)->getName()=="plantPosition"){
				label=probe->getParent(2)->getName();
			}else if(currentSearchDepth>2 && probe->getParent(3)->getName()=="branches"){
				label=probe->getParent(2)->getName();
			}else if(currentSearchDepth>3 && probe->getParent(4)->getName()=="branches"){
				label=probe->getParent(3)->getName();
			}else{
				label="NA";
			}
			//write to file, or store in store..
			os<<std::endl<<std::fixed<<QUOTE<<name<<QUOTE<<SEP<<std::setprecision(3)<<t<<SEP<<std::setprecision(8)<<value<<SEP<<QUOTE<<probe->getUnit()<<QUOTE<<SEP<<QUOTE<<label<<QUOTE<<SEP<<QUOTE<<prevPath<<QUOTE;
		}
	};

	//searchdepth
		//probe the attributes of this object
		SimulaBase::List allattributes;
			probe->getAllChildren(allattributes,t);
			++currentSearchDepth;
			SimulaBase *pp(probe);
			for (auto & it:allattributes){
				probe=it;
				run(t);
			}
			probe=pp;
			--currentSearchDepth;
	path=prevPath;


	if(openedfile){
		os<<std::endl<<std::flush;
		//Close file
		os.close();
	}

}


