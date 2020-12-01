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

#ifndef SIMULAPOINT_HPP_
#define SIMULAPOINT_HPP_

#include "SimulaTimeDriven.hpp"
#include "DataDefinitions.hpp"
#include "DataDefinitions/Units.hpp"
#include <map>

class SimulaPoint:public SimulaTimeDriven{
public:
	typedef Time Colum1;
	typedef MovingCoordinate Colum2;
	typedef SimulaTimeDriven::SimulaPointTable Table;

	SimulaPoint(const std::string &newName, SimulaBase* newAttributeOf, const std::string &rateF, const std::string &integrationF, const Time &newstartTime, const Time &newEndTime );
	SimulaPoint(SimulaBase* newAttributeOf, const Time &startTime, const SimulaPoint* copyThis);
	~SimulaPoint();

	void get(const Time &t, Coordinate &point);
	void getRate(const Time &t, Coordinate &point);
	//void getAbsolute(const Time &t, MovingCoordinate &p);
	void getAbsolute(const Time &t, Coordinate &p);
	void getBase(const Time &t, Coordinate &p);
	void getTime(const Coordinate &p, Time &t, Time tmin=-1, Time tmax=0);
	void getTangent(const Coordinate &p, Coordinate &r, Coordinate &projectionPoint);
	virtual std::string getType()const;
	virtual SimulaBase* getReference();

	Table const * getTable()const;

	void setInitialValue(const MovingCoordinate &pos);
	virtual void set(const Time &t, const Colum2 &var);

	virtual Time lastTimeStep();

	virtual void collectGarbage(const Time&);
	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const;

	virtual void getXMLtag(Tag &tag);
	friend std::istream &operator>>(std::istream &is, SimulaPoint* obj);

protected:
	Table table;
};


std::ostream &operator<<(std::ostream &os, const SimulaPoint &obj);


#endif /*SIMULAPOINT_HPP_*/
