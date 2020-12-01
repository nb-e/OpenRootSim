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

#ifndef STRINGEXTENSIONS_HPP_
#define STRINGEXTENSIONS_HPP_

#include <iostream>
#include <string>
#include <sstream>
#include "../cli/Messages.hpp"

/*NOTE: this works as long as the overloaded functions for >> and << are
 present in the file that this is included in. Don't include them
 here to keep this file nice and simple.
 */

//convert anything to string
template<class T>
std::string convertToString(const T &i) {
	std::stringstream s;
	s << i;
	return s.str();
}

template<class T>
std::string convertToString(const T &i, const int &fixed) {
	std::stringstream s;
	s.precision(fixed);
	if (i >= 0)
		s << '+';
	s << std::fixed << i;
	return s.str();
}

//convert from string
template<class T>
T convertFromString(const std::string &i) // convert int to string
		{
	std::stringstream s;
	s << i;
	T r;
	s >> r;
	if (!s)
		msg::error("convertFromString: Failed to convert string '" + i + "'");
	return r;
}
bool string2bool(const std::string &s);
void stripQuotationMarks(std::string &s);

//a function class which can be used with an stl container to sort string pointers on alphabetic content.
class LessStringPointer {
public:
	bool operator()(const std::string*, const std::string*) const;
};

//find a matching word anywhere in a string
bool findWord(const std::string &longs, const std::string &shorts);
//find next word in the string starting from pos
std::string nextWord(const std::string &s, std::string::size_type &pos,
		const char sep);

//strip white space c++11
std::string &ltrim(std::string &s);

std::string &rtrim(std::string &s);

std::string & trim(std::string &s);


#endif /*STRINGEXTENSIONS_HPP_*/
