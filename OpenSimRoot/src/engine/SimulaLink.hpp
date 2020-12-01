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

#ifndef SIMULALINK_HPP_
#define SIMULALINK_HPP_

#include "SimulaBase.hpp"

class SimulaLink:public SimulaBase{
public:
	SimulaLink(const std::string &newName, SimulaBase* newAttributeOf, const Time &newstartTime);
	SimulaLink(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaLink* copyThis);
	void get(const Time &t, int &rc);
	void get(const Time &t, const Coordinate &pos, double &rc);
	void get(const Time &t, Time &rc);
	void get(const Time &t, Coordinate &rc);
	void get(int &rc);
	void get(Time &rc);
	void get(Coordinate &rc);
	std::string getType()const;
	virtual Unit getUnit();
	void createLink(SimulaBase*);
	void setLinkName(const std::string &);
	SimulaBase* getLink();
	void copy(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaLink* copyThis);
	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const;

	virtual void getXMLtag(Tag &tag);


protected:
	SimulaBase* link_;
	std::string linkedName_;
	bool useMultiplier;
	SimulaBase* multiplier;
	Time refTime;
};


#endif /*SimulaLink_HPP_*/
