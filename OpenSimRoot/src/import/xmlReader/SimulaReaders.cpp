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
#include "SimulaReaders.hpp"

std::istream &operator>>(std::istream &is, SimulaBase* obj){
	//skip comments
	skipComments(is);

	//nothing to read
	//call to read any children
	readTags(is, obj, "/SimulaBase");
	//return
	return is;
}


std::istream &operator>>(std::istream &is, SimulaPoint* obj){
	//skip comments
	skipComments(is);

	//clear fail bit
	is.clear();

	//read data
	MovingCoordinate co;
	std::vector<double> pair;
	READTAG(is,pair)

	//check if there were 3 or 6 numbers and store vector
	switch (pair.size()) {
		case 0:
			co.state.x=0;
			co.state.y=0;
			co.state.z=0;
			obj->setInitialValue(co);
			break;
		case 3:
			co.state.x=pair[0];
			co.state.y=pair[1];
			co.state.z=pair[2];
			obj->setInitialValue(co);
			break;
		default:
			Time t;
			unsigned int i;
			for(i=0;i<pair.size(); i+=7){
				t=pair[i];
				co.state.x=pair[i+1];
				co.state.y=pair[i+2];
				co.state.z=pair[i+3];
				co.rate.x=pair[i+4];
				co.rate.y=pair[i+5];
				co.rate.z=pair[i+6];
				obj->table[t]=co;
				obj->t_solid_=t;
			}
			i=pair.size()-i;
			if(i>0) msg::error(">>SimulaPoint: Expected 3 values, or multiple of 7."+filePosition(is));
			break;
	}

	//check for errors
	if(!is){
		is.clear();
		msg::error(">>SimulaPoint*: failed to read data"+filePosition(is));
	}


	//clear trailing white space
	is>>std::ws;

	//call to read any children
	readTags(is, obj, "/SimulaPoint");

	//return
	return is;
}

std::istream &operator>>(std::istream &is, SimulaVariable* obj){
	//skip comments
	skipComments(is);

	//clear fail bit
	is.clear();

	//read data
	std::vector<double> pair;
	READTAG(is,pair)

	//read initial state and possibly rate
	StateRate sr;
	switch (pair.size()) {
		case 1:
			obj->setInitialValue(pair[0]);
			break;
		case 2:
			obj->setInitialValue(pair[0]);
			obj->setInitialRate(pair[1]);
			break;
		default:
			Time t;
			unsigned int i(0);
			for(i=0;i<pair.size(); i+=3){
				t=pair[i];
				sr.state=pair[i+1];
				sr.rate=pair[i+2];
				obj->table[t]=sr;
				obj->t_solid_=t;
			}
			if(i>pair.size()) msg::error(">>SimulaVariable: Expected 1 OR 2 values, or multiple of 3."+filePosition(is));
			break;
	}

	//check for errors
	if(!is){
		is.clear();
		msg::error(">>SimulaVariable*: failed to read initial state."+filePosition(is));
	}

	//clear trailing white space
	is>>std::ws;

	//call to read any children
	readTags(is, obj, "/SimulaVariable");

	//return
	return is;
}

std::istream &operator>>(std::istream &is, SimulaGrid* obj) {
	//skip comments
	skipComments(is);

	//clear fail bit
	is.clear();

	//read value 	//expect data in 2 colums
	std::vector<double> xy;
	is >> xy;

	//check for errors
	if (!is) {
		is.clear();
		msg::error(">>SimulaGrid*: failed to read data." + filePosition(is));
	}

	//even set of numbers?
	if (double(xy.size()) / 4 != int(xy.size() / 4))
		msg::error(">>SimulaGrid: list of numbers is not a multiple of 4."
				+ filePosition(is));

	//set values in table object
	for (std::size_t i = 0; i < xy.size(); i += 4)
		obj->set(Coordinate(xy[i],xy[i+1],xy[i+2]), xy[i + 3]);

	//clear trailing white space
	is >> std::ws;

	//call to read any children
	readTags(is, obj, "/SimulaGrid");

	//return
	return is;
}

std::istream &operator>>(std::istream &is, SimulaLink* obj){
	//skip comments
	skipComments(is);

	//nothing to read
	//call to read any children
	readTags(is, obj, "/SimulaLink");
	//return
	return is;
}
