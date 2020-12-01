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

#ifndef GEOMETRY_HPP_
#define GEOMETRY_HPP_

#include "../../engine/BaseClasses.hpp"
#include "../GenericModules/Totals.hpp"
#include <map>

/*used for vtu output only*/
class RootClassID:public DerivativeBase{
public:
	RootClassID(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,int &var);
	void calculate(const Time &t,double &var);
	int id;
	static std::map<std::string,int> idList;
};

class RootCircumference:public DerivativeBase{
public:
	RootCircumference(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *rootDiameterSimulator;
};
class RootLength2Base:public DerivativeBase{
public:
	RootLength2Base(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *lg;
	double lf;
	//bool estimated;
};
class RootSegmentLength:public TotalBase{
public:
	RootSegmentLength(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *lg;
	Time ft;
	double ff,lf;
};
class RootSegmentSurfaceArea:public TotalBase{
public:
	RootSegmentSurfaceArea(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase* current,*length;
};
class RootSegmentRootHairSurfaceArea:public DerivativeBase{
public:
	RootSegmentRootHairSurfaceArea(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *length,*rhdensity,*rhlength,*rhdiameter;
};
class RootSegmentVolume:public TotalBase{
public:
	RootSegmentVolume(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *length,*diameter;
};
///calculate the cortical volume
class RootSegmentVolumeCortex:public TotalBase{
public:
	RootSegmentVolumeCortex(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *steelVolume,*rootVolume;
};
#endif /*GEOMETRY_HPP_*/
