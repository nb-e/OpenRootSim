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

#ifndef SIMULABASE_HPP_
#define SIMULABASE_HPP_

#include <string>
#include <vector>
#include "DataDefinitions.hpp"
#include "DataDefinitions/TagClass.hpp"

class Database;

class SimulaBase {
private:
	std::string & name_;
	SimulaBase* referencePoint;
	void setReference();

protected:
	typedef std::vector<SimulaBase*> PointerList;

	Unit * unit_;
	Time startTime, endTime;
	SimulaBase* parent_;

	//for management of the estimate flags
	static  PointerList predictorOrder;
	virtual void releaseDependents();

	std::string getObjectGeneratorName()const;

private:
	Database *attributes;
public:

	typedef  std::multimap<Coordinate,SimulaBase*> Positions;
	typedef std::vector<SimulaBase*> List;

	SimulaBase();
	SimulaBase(SimulaBase* const newAttributeOf, const Unit &newUnit, const Time &newstartTime);
	SimulaBase(const std::string &newName, SimulaBase* newAttributeOf, const Unit &newUnit, const Time &newstartTime);
	SimulaBase(const std::string &newName, SimulaBase* newAttributeOf, const Unit &newUnit, const Time &newstartTime, const Time &newendTime);
	SimulaBase(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaBase* const copyThis);
	virtual ~SimulaBase();

	static int numberOfInstantiations;

	std::string getName()const;
	std::string getPrettyName()const;
	std::string getPath()const;
	void setName(const std::string &newName);
	virtual std::string getType()const;

	SimulaBase* getParent()const;
	void setParent(SimulaBase* p);
	SimulaBase* getParent(const unsigned int i) const;
	//Database const * getChildren()const;
	Database * getChildren();
	Database* hasChildren();
	int getNumberOfChildren()const;//does not update!
	int getNumberOfChildren(const Time &t);//does update
	SimulaBase* getChild(const std::string & name,const Time & t);
	SimulaBase* existingChild(const std::string & name,const Time & t);
	SimulaBase* getChild(const std::string & name);
	SimulaBase* existingChild(const std::string & name);
	SimulaBase* getChild(const std::string & name,const Unit & u);
	SimulaBase* existingChild(const std::string & name,const Unit & u);
	SimulaBase* getSibling(const std::string & name,const Time & t);
	SimulaBase* existingSibling(const std::string & name,const Time & t);
	SimulaBase* getSibling(const std::string & name);
	SimulaBase* existingSibling(const std::string & name);
	SimulaBase* getSibling(const std::string & name,const Unit & u);
	SimulaBase* existingSibling(const std::string & name,const Unit & u);


	SimulaBase* getPath(const std::string &name);
	SimulaBase* getPath(const std::string &name, const Time &t);
	SimulaBase* getPath(const std::string &name, const Unit &u);

	SimulaBase* existingPath(const std::string &name);
	SimulaBase* existingPath(const std::string &name, const Time &t);
	SimulaBase* existingPath(const std::string &name, const Unit &u);

	SimulaBase* getNextSibling(const Time &t);
	SimulaBase* getNextSibling()const;
	SimulaBase* getPreviousSibling(const Time &t);
	SimulaBase* getPreviousSibling()const;
	SimulaBase* getFirstChild(const Time &t);
	SimulaBase* getFirstChild()const;
	SimulaBase* getLastChild()const;

	void getAllChildren(List&, const Time &t);
	void getAllChildren(List&)const;

	void stopUpdatefunction();
	static void updateAll(const Time &);
	void updateRecursively(const Time &);

	static void getAllPositions(const Time & t, Positions& list);
	static void getAllPositions(Positions& list);
	void getYSlice(const Time &, const double, const double, SimulaBase::Positions&);
	void getPositionsWithinRadius(const Time &, const Coordinate& c, const double & r, SimulaBase::Positions&);
	void getPositionsInsideBox(const Time &, const Coordinate&, const Coordinate &, SimulaBase::Positions&);

	static void storePosition(SimulaBase *);
	static void broadcast(SimulaBase *pSB);
	static void signalMeAboutNewObjects(SimulaBase* me);
	void setFunctionPointer(const std::string &name="");


	void changeParent(SimulaBase* newAttributeOf);
	virtual SimulaBase* getReference();
	virtual void databaseSignalingHook(SimulaBase * newObject);
	virtual void registerSignalingHook();


	friend std::ostream &operator<<(std::ostream &os, const SimulaBase* obj);

	bool evaluateTime(const Time &t)const;
	void setEndTime(const Time &t);
	void setStartTime(const Time &t);
	Time getEndTime()const;
	Time getStartTime()const;
//	void setLifeTime(const Time &bt, const Time &et);
///@todo if the order was, data, time, coordinate we could reduce this list using default values for arguments
	virtual void get(const Time &t, int &returnConstant);
	virtual void get(const Time &t, std::string &returnConstant);
	virtual void getRate(const Time &t, Time &var);

	virtual void get(const Time &t, Coordinate &point);
	virtual void get(const Time &t, MovingCoordinate &point);
	virtual void getAbsolute(const Time &t, Coordinate &point);
	virtual void getBase(const Time &t, Coordinate &point);
	virtual void getRate(const Time &t, Coordinate &point);
	virtual void getAbsolute(const Time &t, MovingCoordinate &point);
	virtual void get(int &returnConstant);
	virtual void get(std::string &returnConstant);
	virtual void get(bool &returnConstant);
	virtual void get(const Time &x, Time &y);
	virtual void get(Time &x);
	virtual void get(const Time &t, const Coordinate & pos, double &y);
	virtual void get(const Time &t, const Coordinate & pos, Coordinate &y);
	virtual void getRate(const Time &t, const Coordinate & pos, double &y);

	virtual void get(Coordinate &point);
	virtual Time getTimeStep(const Time& t);
	virtual void getAverageRate(const Time &t1, const Time &t2, double &var);
	virtual void getAverageRate(const Time &t1, const Time &t2, Coordinate &var);
	//virtual void set(const double &x, const double &y);

	virtual SimulaBase* followChain(const Time & t);

	virtual void getTime(const Coordinate &p, Time &t, Time tmin=-1, Time tmax=0);
	virtual void getTime(const double &p, Time &t, Time tmin=-1, Time tmax=0);

	virtual Unit getUnit();
	void checkUnit(const Unit& unit)const;
	void setUnit(const Unit &newUnit);

	void copyAttributes(const Time &t, const SimulaBase *original);
	void copy(SimulaBase* newAttributeOf, const Time &newstartTime, const SimulaBase* copyThis);

	virtual Time &minTimeStep();
	virtual Time &maxTimeStep();
	virtual Time &preferedTimeStep();
	virtual Time lastTimeStep();

	virtual void collectGarbage(const Time&);
	virtual void garbageCollectionOff();
	virtual void garbageCollectionOn();

	virtual void unLockDependency();
	virtual bool setDependent(SimulaBase* me);

	virtual void getXMLtag(Tag &tag);

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const;

	static std::size_t getPredictorSize();
	static void clearPredictions(const std::size_t &s);
	static void addToPredictorList(SimulaBase *p);

};


#endif /*SIMULABASE_HPP_*/
