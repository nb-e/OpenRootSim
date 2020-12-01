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

#include "TagClass.hpp"

//Constructors for Tag
Tag::Tag():closed(false){}
std::map<std::string,std::string> Tag::entities;

//overload == operator for Tag==std::string comparison
bool Tag::operator== (const std::string &s)const {
  return this->name==s;
}
bool Tag::operator!= (const std::string &s)const {
  return this->name!=s;
}

//overload == operator for Tag==Tag comparison
//note: attributes are compared one sided! attributes present in t, but not in *this are ignored
bool Tag::operator== (const Tag &t)const {
	//compare names
	bool r= (this->name==t.name);
	//compare shared attributes is names are equal as long as r=tru
	for(Tag::Attributes::const_iterator it=t.attributes.begin(); r && it!=t.attributes.end();++it){
		//find corresponding attribute
		Tag::Attributes::const_iterator ithis=this->attributes.find(it->first);
		//if present r is true, else false
		r=(ithis!=this->attributes.end());
		//check wether attribute values are the same
		if (r) r=(ithis->second==it->second);
	}
	return r;
}
bool Tag::operator!= (const Tag &t)const {
	return !(*this==t);
}


//==================writing tags==================================
//write a tag to os stream
std::ostream &operator<<(std::ostream &os, const Tag &obj){
	//write opening bracket and name
	os<<"<"<<obj.name;
	//write attributes
	std::string d;
	for (auto & it:obj.attributes){
		if(it.first!="_DATA_"){
			os<<" "<<it.first<<"=\""<<it.second<<"\"";
		}else{
			d=it.second;
		}
	}
	//write closing bracket
	if(obj.closed){
		os<<"/>";
	}else{
		os<<">"<<std::endl<<d;
	}
	//return the stream
	return os;
}



