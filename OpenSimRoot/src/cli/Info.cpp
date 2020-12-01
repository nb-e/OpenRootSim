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
#include "Info.hpp"
#include "ANSImanipulators.hpp"
#include "Messages.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "../engine/BaseClasses.hpp"
#include "../export/ExportLibrary.hpp"
#include "gitversion.hpp"
#if _WIN32 || _WIN64
extern bool dialog_mode;
#include <ctime>
#endif

void writeWarnings() {
	//open the warnings file
	std::string filename = "warnings.txt";
	std::ofstream fs(filename.c_str());
	if (fs.is_open()) {
		fs << getExecutionTime();
	}
	if (!msg::getWarnings().empty()) {
		std::cout << ANSI_Blue << "\nTHE MODEL PUT OUT WARNINGS:" << std::flush;
		if (fs.is_open())
			fs << "\nTHE MODEL PUT OUT WARNINGS:";
		int count = 0;
		for (auto &it : msg::getWarnings()) {
			//write to screen max 50 warnings
			if (count < 50) {
				std::cout << std::endl << it.first << ANSI_right << ANSI_rigth_tab7 << it.second << "x";
			} else if (count == 50) {
				std::cout << std::endl << "More than 50 warnings, only showed first 50, see warnings.txt for complete list";
			}
			//write all warnings to file
			if (fs.is_open())
				fs << std::endl << it.first << '\t' << it.second << "x";
			++count;
		}
	} else {
		std::cout << ANSI_Blue << "\nThere are no warnings.";
		if (fs.is_open())
			fs << "\nThere are no warnings.";
	}
	std::cout << ANSI_Black << std::flush;
	if (fs.is_open()) {
		fs << std::endl;
		fs.close();
	}
}

void exitRoutine() {
	exitRoutine(true);
}

void exitRoutine(const bool writewarnings) {
//	//destroy export modules
//	for(auto it(modules.begin()); it!=modules.end(); ++it){
//		(*it)->finalize();
////		delete(*it);
//	}
//
	// Tell about possible warnings
	if (writewarnings)
		writeWarnings();

	//write run time
	std::cout << std::endl << getExecutionTime();

	//reset terminal
	std::cout << ANSI_ResetTerminal << std::endl << std::flush;

#if _WIN32 || _WIN64
	if (dialog_mode) {
		std::cout<<std::flush;
		system("pause");
	}
#endif
}

cla::cla(const int argc, char **argv) {
	//decompose cl_ arguments into sorted list
	for (int i = 0; i < argc; ++i) {
		std::string ds1(argv[i]), ds2;
		if (i + 1 == argc) {
			ds2 = "";
		} else {
			ds2 = argv[i + 1];
		}
		cl_[ds1] = ds2;
	}

	//file to load may be last argument
	if (!this->find("-f")) {
		cl_["-f"] = argv[argc - 1];
	}
}

bool cla::find(const std::string &lab, const std::string &altlab1, const std::string &altlab2, const std::string &altlab3) const {
	bool r(false);
	if (!lab.empty() && cl_.find(lab) != cl_.end())
		r = true;
	if (!altlab1.empty() && cl_.find(altlab1) != cl_.end())
		r = true;
	if (!altlab2.empty() && cl_.find(altlab2) != cl_.end())
		r = true;
	if (!altlab3.empty() && cl_.find(altlab3) != cl_.end())
		r = true;
	return (r);
}

//determine which file to load
std::string cla::findValue(const std::string &lab, const std::string &altlab1, const std::string &altlab2, const std::string &altlab3) const {
	std::string r;
	if (!lab.empty()) {
		auto it = cl_.find(lab);
		if (it != cl_.end()) {
			r = it->second;
		}
	}
	if (!altlab1.empty()) {
		auto it = cl_.find(altlab1);
		if (it != cl_.end()) {
			r = it->second;
		}
	}
	if (!altlab2.empty()) {
		auto it = cl_.find(altlab2);
		if (it != cl_.end()) {
			r = it->second;
		}
	}
	if (!altlab3.empty()) {
		auto it = cl_.find(altlab3);
		if (it != cl_.end()) {
			r = it->second;
		}
	}
	return (r);
}

void printhelp() {
	std::cout
			<< "Usage: OpenSimRoot [OPTIONS] [FILE]\n\
OpenSimRoot simulates a model defined in FILE\n\
\n\
  -f, --file                 specify simulation file, default last argument\n\
  -h, --help or /?           print this help message\n\
  -v                         be verbose with warnings\n\
  -q                         be quite with warnings\n\
  -l, --list                 print list of registered functions\n\
  -V, --verify               Experimental function for verifying input files\n\
  -ww                        warnings to screen\n\
\n\
Examples:\n\
  OpenSimRoot -h             Prints this message\n\
  OpenSimRoot runModel.xml   Runs the SimRoot model \n\
\n\
Support at j.postma@fz-juelich.de\n\
Built on " << __DATE__ << ", git " << gitversion << "\n\
Licensed to you under the GPLv3 license.\n";
}

void printfunctions(){
	std::cout<< "Registered object generator functions:"<<std::endl;
	for(auto i:BaseClassesMap::getObjectGeneratorClasses()){
		std::cout<<i.first<<" ";
	}
	std::cout<< std::endl<< std::endl;

	std::cout<< "Registered integration functions:"<<std::endl;
	for(auto i:BaseClassesMap::getIntegrationClasses()){
		std::cout<<i.first<<" ";
	}
	std::cout<< std::endl<< std::endl;

	std::cout<< "Registered functions:"<<std::endl;
	for(auto i:BaseClassesMap::getDerivativeBaseClasses()){
		std::cout<<i.first<<" ";
	}
	std::cout<< std::endl;
}



