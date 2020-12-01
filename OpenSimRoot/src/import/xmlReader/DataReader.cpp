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

#include "DataReaders.hpp"
#include "Tag.hpp"
#include "ReadString.hpp"
#include "../../cli/Messages.hpp"

//todo convert these to simple readData functions, so we can pass the filename, and have better info in the error message.

//unit reading functions
std::istream &operator>>(std::istream &is, Unit &obj){
	//read string which presents unit
	std::string us;
	char ch;
	getWord(is,us,ch);
	//unget delimiter (which is < beginning of closing tag)
	is.unget();
	//convert string to unit and store it
	obj=Unit(us);
	//return stream
	return is;
}

/*std::istream &operator>>(std::istream &is, double &obj) {
	//read vector of numbers
	std::vector<double> pair;
	READTAG(is,pair)

	//check if there were two numbers and store vector
	switch (pair.size()) {
	case 1:
		obj = pair[0];
		break;
	default:
		msg::error(">>State: Expected 1 or 2 values." + filePosition(is));
		break;
	}

	//return stream
	return is;
}*/

//reading of StateRate from stream
std::istream &operator>>(std::istream &is, StateRate &obj) {
	//read vector of numbers
	std::vector<double> pair;
	READTAG(is,pair)

	//check if there were two numbers and store vector
	switch (pair.size()) {
	case 2:
		obj.state = pair[0];
		obj.rate = pair[1];
		break;
	case 1:
		obj.state = pair[0];
		obj.rate = 0;
		break;
	default:
		msg::error(">>StateRate: Expected 1,2 or 3 values." + filePosition(is));
		break;
	}

	//return stream
	return is;
}


//reading of Coordinate from stream
std::istream &operator>>(std::istream &is, Coordinate &obj) {
	//read vector of numbers
	std::vector<double> pair;
	READTAG(is,pair)

	//check if there were 3  numbers and store vector
	switch (pair.size()) {
		case 3:
			obj.x=pair[0];
			obj.y=pair[1];
			obj.z=pair[2];
			break;
		default:
			msg::error(">>Coordinate: Expected 3 values."+filePosition(is));
			break;
	}

	//return stream
	return is;
}

//reading of MovingCoordinate from stream
std::istream &operator>>(std::istream &is, MovingCoordinate &obj) {
	//read vector of numbers
	std::vector<double> pair;
	READTAG(is,pair)

	//check if there were 3 or 6 numbers and store vector
	switch (pair.size()) {
		case 3:
			obj.state.x=pair[0];
			obj.state.y=pair[1];
			obj.state.z=pair[2];
			break;
		case 6:
			obj.state.x=pair[0];
			obj.state.y=pair[1];
			obj.state.z=pair[2];
			obj.rate.x=pair[3];
			obj.rate.y=pair[4];
			obj.rate.z=pair[5];
			break;
		default:
			msg::error(">>MovingCoordinate: Expected 3 or 6 values."+filePosition(is));
			break;
	}

	//return stream
	return is;
}


//the vector is used as intermediate in reading coordinates
std::istream &operator>>(std::istream &is, std::vector<double> &d){
	//check stream
	if(!is)msg::error(">>vector<double>: error on received input stream");

	//erase all elements of d
	d.clear();
	double k;
	// remove white space
	is>>std::ws;
	//read elements till closing tag is found
	while(!is.eof() ){
		is>>std::ws;
		if(is.peek()=='<'){
			long int fw;
			if(peek4comment(is,fw)){
				is.seekg(fw,std::ios::cur);
			}else{
				break;
			}
		}else {
			//read a number
			READTAG(is,k)
			d.push_back(k);
		}
	}
	//return stream
	return is;
}

