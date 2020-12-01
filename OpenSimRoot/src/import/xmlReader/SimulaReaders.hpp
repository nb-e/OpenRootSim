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

#ifndef SIMULAREADERS_HPP_
#define SIMULAREADERS_HPP_

#include "StreamPositionInfo.hpp"
#include "Tag.hpp"
#include "Indenting.hpp"
#include "../../tools/StringExtensions.hpp"
#include "DataReaders.hpp"
#include "ReadXMLfile.hpp"
#include "ReadString.hpp"
#include <istream>
#include "../../engine/SimulaBase.hpp"
#include "../../engine/SimulaLink.hpp"
#include "../../engine/SimulaConstant.hpp"
#include "../../engine/SimulaStochastic.hpp"
#include "../../engine/SimulaTable.hpp"
#include "../../engine/SimulaGrid.hpp"
#include "../../engine/SimulaVariable.hpp"
#include "../../engine/SimulaPoint.hpp"
#include "../../engine/SimulaDerivative.hpp"
#include "../../engine/SimulaExternal.hpp"
#include "../../engine/SimulaStochastic.hpp"

std::istream &operator>>(std::istream &is, SimulaBase* obj);
std::istream &operator>>(std::istream &is, SimulaLink* obj);
std::istream &operator>>(std::istream &is, SimulaGrid* obj);

template <class T>
std::istream &operator>>(std::istream &is, SimulaConstant<T>* obj){
	//skip comments
	skipComments(is);

	//clear fail bit
	is.clear();

	//read value
	T constant;
	is>>constant;

	//check for errors
	if(!is){
		is.clear();
		msg::error(">>SimulaConstant<>*: failed to read data."+filePosition(is));
	}

	//set value in object
	obj->setConstant(constant);

	//clear trailing white space
	is>>std::ws;

	//return
	return is;
}

//note: this needs to be inline or external to aboiv multiple definition problems
template <>
inline std::istream &operator>> <bool>(std::istream &is, SimulaConstant<bool>* obj){
	//skip comments
	skipComments(is);

	//clear fail bit
	is.clear();

	//read value
	bool constant;
	std::string s;
	is>>s;
	if(s=="0" || s=="false" || s=="False"|| s=="F"|| s=="f"){
		constant=false;
	}else{
		constant=true;
	}

	//check for errors
	if(!is){
		is.clear();
		msg::error(">>SimulaConstant<>*: failed to read data."+filePosition(is));
	}

	//set value in object
	obj->setConstant(constant);

	//clear trailing white space
	is>>std::ws;

	//return
	return is;
}

template<class TVar>
std::istream &operator>>(std::istream &is, SimulaDerivative<TVar>* obj) {
	//call to read any childeren
	//readTags(is, obj, "/SimulaDerivative");
	return is;
}

template <class T>
std::istream &operator>>(std::istream &is, SimulaTable<T>* obj){
	//skip comments
	skipComments(is);

	//clear fail bit
	is.clear();

	//read value 	//expect data in 2 colums
	std::vector<double> xy;
	is>>xy;

	//check for errors
	if(!is){
		is.clear();
		msg::error(">>SimulaTable*: failed to read data."+filePosition(is));
	}

	//even set of numbers?
	if( double (xy.size())/2 != int (xy.size()/2)) msg::error(">>SimulaTable: vector xy does not have an even number of numbers."+filePosition(is));

	//set values in table object
	for(unsigned int  i=0 ; i<xy.size() ; i+=2) obj->set(xy[i],xy[i+1]);

	//clear trailing white space
	is>>std::ws;

	//call to read any children
	readTags(is, obj, "/SimulaTable");

	//return
	return is;
}

template <class T>
std::istream &operator>>(std::istream &is, SimulaStochastic<T,std::normal_distribution<> >* obj){
	//nothing to do: return
	return is;
}
std::istream &operator>>(std::istream &is, SimulaPoint* obj);
std::istream &operator>>(std::istream &is, SimulaVariable* obj);

#endif //SIMULAREADERS_HPP_
