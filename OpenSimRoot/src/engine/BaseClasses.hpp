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

#ifndef BASECLASSES_HPP_
#define BASECLASSES_HPP_

#include "ObjectGenerator.hpp"
#include "SimulaVariable.hpp"
#include "SimulaPoint.hpp"

//class SimulaVariable;
//class SimulaPoint;
///@todo derivativebase and those of variable and coordinate can probably be merged into one function base.
///@todo this could be a template - but than registration has to occur in different tables which makes it difficult
class DerivativeBase{
protected:
	SimulaDynamic* pSD;
public:
	DerivativeBase(SimulaDynamic* const new_pSD);
	virtual ~DerivativeBase();
	//void initiate();
	virtual void calculate(const Time &t,double &var);
	virtual void calculate(const Time &t, const Coordinate &pos, double &var);
	virtual void calculate(const Time &t,int &var);
	virtual void calculate(const Time &t,std::string &var);
	virtual void calculate(const Time &t,Coordinate &var);
	virtual bool postIntegrationCorrection(SimulaVariable::Table &table);
	virtual bool postIntegrationCorrection(SimulaPoint::Table &table);
	virtual SimulaBase* getNext(const Time & t);
	virtual std::string getName()const;
	virtual void addObject(SimulaBase *newObject);
};

//instantiation of map that lists all rate classes
extern std::map<std::string, derivativeBaseInstantiationFunction > derivativeBaseClassesMap;


//structure for predictor data used in integration methods
class SimplePredictor{
public:
	SimplePredictor(SimulaVariable::Table & data, const Time& deltaT);
	SimplePredictor(SimulaPoint::Table & data, const Time& deltaT);
	void resetTimeStep(double dt);
	const Time t0,t1,newTime;
	Time t2;
	const StateRate s0;
	StateRate s1,s2;
	const MovingCoordinate p0;
	MovingCoordinate p1,p2;
	int step;
};

class IntegrationBase{
public:
	IntegrationBase();
	virtual ~IntegrationBase();
	void initiate(SimulaTimeDriven* pSP, DerivativeBase & rateCalculator);
	virtual void predict(const Time&,Coordinate&);
	virtual void predict(const Time&,double &);
	virtual void predictRate(const Time&,Coordinate&);
	virtual void predictRate(const Time&,double &);
	virtual std::string getName()const=0;
	virtual void integrate(SimulaPoint::Table & data, DerivativeBase & rateCalculator);
	virtual void integrate(SimulaVariable::Table & data, DerivativeBase & rateCalculator);
	virtual void getTimeStepInfo(Time & st, Time & rt)const;
	virtual void setTolerance(const double);//used by some to set an error tolerance level
	const SimulaTimeDriven* getOwner()const{return pSD;};
protected:
	SimulaTimeDriven* pSD;
	virtual void timeStep(const Time & lastTime, Time & newTime, Time & deltaT, const double & l=0);
	SimplePredictor* predictor;
	static double defaultSpatialIntegrationLength;
	SimulaBase* multiplier;
	SimulaBase* rateMultiplier;
};

typedef IntegrationBase* (*integrationInstantiationFunction)();

class BaseClassesMap{
private:

public:
	static std::map<std::string, derivativeBaseInstantiationFunction > & getDerivativeBaseClasses();
	static std::map<std::string, objectGeneratorInstantiationFunction > & getObjectGeneratorClasses();
	static std::map<std::string, integrationInstantiationFunction> & getIntegrationClasses();
};



#endif /*SIMULADYNAMIC_HPP_*/
