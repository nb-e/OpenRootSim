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
#include "Messages.hpp"
#include <exception>
#include <stdexcept>
#include <iostream>
#include "ANSImanipulators.hpp"

MyException::MyException(const std::string &err) :
		std::exception(), error_(err) {
}

MyException::~MyException() throw () {
}

const char* MyException::what() const throw () {
	return (error_.c_str());
}

void MyException::set(const std::string& err) {
	error_ = err;
}

MyException msg::globalerror_("none");
std::map<std::string, int> msg::warningMessages_;
int msg::verboseThreshHold_(0);

int msg::getVerboseLevel() {
	return (verboseThreshHold_);
}
void msg::setVerboseLevel(const int & lev) {
	verboseThreshHold_ = lev;
}

void msg::warning(const std::string & msg, int verbose) {
	if (verbose > msg::verboseThreshHold_)
		return;
	std::map<std::string, int>::iterator it(msg::warningMessages_.find(msg));
	if (it == msg::warningMessages_.end()) {
		msg::warningMessages_[msg] = 1;
		if(msg::warningsToScreen_) std::cout<<std::endl<<ANSI_BlueOnWhite<<msg<<ANSI_BlackOnWhite<<std::endl;
	} else {
		it->second++;
	}
}

std::map<std::string, int> &msg::getWarnings() {
	return (msg::warningMessages_);
}

void msg::error(const std::string &msg) {
	if(errorIsWarning_){
		msg::warning(msg);
		errorToWarning_=true;
	}else{
		msg::warning("FATAL ERROR: " + msg);
		msg::globalerror_.set(msg);
		throw msg::globalerror_;
	}
}

bool msg::errorIsWarning_(false);
bool msg::errorToWarning_(false);
bool msg::warningsToScreen_(false);

void msg::setErrorIsWarningOn(){
	errorIsWarning_=true;
}
void msg::setErrorIsWarningOff(const std::string& msg){
	errorIsWarning_=false;
	if(errorToWarning_){
		errorToWarning_=false;
		msg::globalerror_.set(msg);
		throw msg::globalerror_;
	}
}
void msg::warningsToScreen(){
	msg::warningsToScreen_=true;
};

bool msg::errorOccured(){
	return(errorToWarning_);
}

/*extern "C" {
void errormessage(char msg[300], int length) {
	//todo this causes segmentation fault
	//if(length>=300)length=299;
	//msg[length]='\0';
	msg::error(msg);
}
void warningmessage(char msg[300], int length) {
	//todo this causes segmentation fault
	//if(length>=300)length=299;
	//msg[length]='\0';
	msg::warning(msg);
}
}*/

