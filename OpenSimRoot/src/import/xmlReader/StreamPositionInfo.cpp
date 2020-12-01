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
#include "StreamPositionInfo.hpp"
#include "../../cli/Messages.hpp"
//#include <sstream>
#include <iostream>
#include "../../tools/StringExtensions.hpp"
#include "../../cli/ANSImanipulators.hpp"

//return the linenumber the cursor is on in stream is
int lineNumber(std::istream &is){
	//store original position
	const std::streamsize op=is.tellg();
	//move to begin
	is.seekg(0,std::ios::beg);

	//count number of lines
	int count=0;
	std::string s;
	while(is.tellg() < op){
		getline(is,s);
		count++;
	}

	//restore original position
	is.seekg(op);
	
	return count;
}

//return a string containing info about the position the cursor is on
//return string contains linenumber, and cursor position and the actual line. 
std::string filePosition(std::istream &is, const std::string &name){
 	//clear fail and eof bits
 	is.clear();

	//move backwards 1 if already at end of line use a loop because of empty lines
	while(is.peek()=='\n' || is.peek()==' ' || is.peek()=='\t') is.seekg (-1, std::ios::cur);

	//store original position
	const std::streamsize op(is.tellg());

	//move to beginning of file
	is.seekg(0,std::ios::beg);

	//count number of lines
	unsigned int ln(0);
	std::string line;
	std::streamsize bl(is.tellg());
	while(is.tellg() < op){
		bl=is.tellg();
		getline(is,line);
		ln++;
	}

	//restore original position
	is.seekg(op);

	//insert cursor sign
	const std::size_t cn(op-bl);
	if(cn<=line.size()) {
		line.insert(cn,"||");
		//shorten line
		if(line.length()>cn+15) {
			line.erase(cn+15);
			line+="....";
		}
		if(line.length()>33) {
			line.erase(0,line.length()-33);
			line.insert(0,"...");
		}
		trim(line);
	}
	
	//construct final message with line number
	if(name.empty()){
		line= "\nLine "+std::to_string(ln)+" reads:\n '"+line+"'";
	}else{
		line= "\nLine "+std::to_string(ln)+" in '"+name+"' reads:\n '"+line+"'";
	}
	
	return line;
}
