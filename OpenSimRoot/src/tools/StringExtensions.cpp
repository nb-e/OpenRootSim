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

#include "StringExtensions.hpp"


bool LessStringPointer::operator()(const std::string* a, const std::string* b)const{
	return (*a)<(*b); ///todo this can be changed back to (a) > (b) if vtu code does not depend on alphabetic order anymore
}

bool findWord(const std::string &longs, const std::string &shorts){
	return (longs.find( shorts, 0 ) != std::string::npos);
}

std::string nextWord(const std::string &s, std::string::size_type &pos, const char sep){
	if (pos>s.size()) msg::error("nextWord: starting position is outside string");
	std::string::const_iterator it(s.begin());
	std::string r;
	for (it+=pos ; it!=s.end() ;++it) {
		++pos;
		if(*it==sep) break;
		if(*it!=' ' && *it!='\n' && *it!='\t' && *it!='\r') r+= *it;
	}
	return r;
}

bool string2bool(const std::string &s){
	if (s=="T") return true;
	if (s=="TRUE") return true;
	if (s=="t") return true;
	if (s=="true") return true;
	if (s=="Y") return true;
	if (s=="YES") return true;
	if (s=="y") return true;
	if (s=="yes") return true;
	if (s=="1") return true;
	return false;
}

void stripQuotationMarks(std::string &s){
	if(s[0]=='"' || s[0]=='\'') s=s.substr(1);
	if(s[s.size()-1]=='"' || s[s.size()-1]=='\'') s=s.substr(0,s.size()-1);
}


//strip white space from string
std::string &ltrim(std::string &s) {
	//s.erase(s.begin(),std::find_if_not(s.begin(), s.end(), [](int c) {return isspace(c);}));
	//todo above will only work in newer gcc (4.8?)
	for(std::string::iterator it(s.begin()); it!=s.end(); ++it){
		if(*(it)=='\0' || isspace(*it)) continue;
		s.erase(s.begin(),it);
		break;
	}

	return s;
}

std::string &rtrim(std::string &s) {
	//s.erase(std::find_if_not(s.rbegin(), s.rend(), [](int c) {return isspace(c);}).base(),s.end());
	//todo above will only work in newer gcc (4.8?)
	for(std::string::reverse_iterator it(s.rbegin()); it!=s.rend(); ++it){
		if(*(it)=='\0' || isspace(*it)) continue;
		s.erase(it.base(),s.end());
		break;
	}
	return s;
}

std::string & trim(std::string &s) {
	return ltrim(rtrim(s));
}

