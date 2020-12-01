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

#ifndef PLANTTYPE_HPP_
#define PLANTTYPE_HPP_
#include "../engine/Origin.hpp"

///@todo this is done many times, so I used a macro, however it's an ugly way of finding the plant type and I hope when the database gets replaced we'll be able to use nicer quieries
#define PLANTTYPE(STRING,POINTER)\
	for(SimulaBase* pp(POINTER); pp->getName()!="origin" ; pp=pp->getParent()){\
		if(pp->getName()=="plantPosition"){\
			pp->getSibling("plantType")->get(STRING); \
			break;\
		}\
	}

#define PLANTNAME(STRING,POINTER)\
	for(SimulaBase* pp(POINTER); pp->getName()!="origin" ; pp=pp->getParent()){\
		if(pp->getName()=="plantPosition"){\
			STRING=pp->getParent()->getName(); \
			break;\
		}\
	}

#define PLANTTOP(POINTER)\
	while (POINTER->getName()!="plantPosition" && POINTER->getName()!="origin")	POINTER=POINTER->getParent();\
	if(POINTER->getParent()) POINTER=POINTER->getParent();


#define GETPLANTPARAMETERS(PLANTTYPE)\
	ORIGIN->getChild("rootTypeParameters")->getChild(PLANTTYPE)

#define GETSHOOTPARAMETERS(PLANTTYPE)\
	ORIGIN->getChild("rootTypeParameters")->getChild(PLANTTYPE)->getChild("shoot")

#define GETROOTPARAMETERS(PLANTTYPE,ROOTTYPE)\
	ORIGIN->getChild("rootTypeParameters")->getChild(PLANTTYPE)->getChild(ROOTTYPE)

#define GETRESOURCEPARAMETERS(PLANTTYPE)\
	ORIGIN->getChild("rootTypeParameters")->getChild(PLANTTYPE)->getChild("resources")

#endif /*PLANTTYPE_HPP_*/
