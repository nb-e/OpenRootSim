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

#include "BaseClasses.hpp"
#include "../cli/Messages.hpp"
#include "../math/MathLibrary.hpp"
#include "../math/InterpolationLibrary.hpp"
#include "Origin.hpp"
#if _WIN32 || _WIN64
#include <algorithm>
#endif

DerivativeBase::DerivativeBase(SimulaDynamic* const new_pSD) :
		pSD(new_pSD)
{
}

DerivativeBase::~DerivativeBase() {
}

void DerivativeBase::calculate(const Time &t, double &var) {
	msg::error(
			"DerivativeBase::calculate: basefunction called. Object name: "
					+ pSD->getName());
}

void DerivativeBase::calculate(const Time &t, const Coordinate &pos, double &var) {
	calculate(t,var);
}

void DerivativeBase::calculate(const Time &t, int &var) {
	msg::error("DerivativeBase::calculate: basefunction called");
}

void DerivativeBase::calculate(const Time &t, std::string &var) {
	msg::error("DerivativeBase::calculate: basefunction called");
}

void DerivativeBase::calculate(const Time &t, Coordinate &var) {
	msg::error("DerivativeBase::calculate: basefunction called");
}

bool DerivativeBase::postIntegrationCorrection(SimulaVariable::Table &table) {
	return false;
}

bool DerivativeBase::postIntegrationCorrection(SimulaPoint::Table &table) {
	return false;
}

void DerivativeBase::addObject(SimulaBase *newObject){
	msg::error("DericatiBase::addObject called, but not implemented for class "+getName());
}

SimulaBase* DerivativeBase::getNext(const Time & t) {
	msg::error(
			"DerivativeBase: no getNext function implemented for this class with name "
					+ getName());
	return nullptr;
}

/* Instead of locking - we could try to switch to an estimate mode - which would allow for a more iterative method to resolve
 * note that when using such methods user much be sure that the 'estimate' tag is preserved throughout the calculations
 * note that we can still try to detect loop by counting how many estimates are asked for - this number should be small (4 for rungkuta, 1 for heuns etc).
 */

std::string DerivativeBase::getName() const {
	msg::error(
			"DerivativeBase::getName: programming error: forgot to include a getName() function in class used by "
					+ pSD->getName());
	return "";
}


SimplePredictor::SimplePredictor(SimulaVariable::Table & data,
		const Time& deltaT):
	t0(data.size()>1?(++(data.rbegin()))->first:(data.rbegin())->first),
	t1(data.rbegin()->first),
	newTime(data.rbegin()->first+deltaT),
	s0(data.size()>1?(++(data.rbegin()))->second:(data.rbegin())->second)
{
	//last state rate couple
	SimulaVariable::Table::reverse_iterator it(data.rbegin());
	s1 = (it->second);
	//t1 = (it->first);

	//last state rate couple
	if (data.size() > 1)
		++it;
	//s0 = (it->second);
	//t0 = (it->first);

	//set predictor
	t2 = t1 + deltaT;
	//newTime=t2;
	s2 = s1;
	if (data.size() > 1) {
		linearInterpolation(t0, t1, t2, s0.rate, s1.rate, s2.rate);
		linearInterpolation(t0, t1, t2, s0.state, s1.state, s2.state);
		//second order correction
		if(t1>t0) s2.state += (deltaT/(t1-t0))*(s2.rate - s1.rate) / 2.;
	}

	//set step
	step = 1;
}
SimplePredictor::SimplePredictor(SimulaPoint::Table & data,
		const Time& deltaT):
			t0(data.size()>1?(++(data.rbegin()))->first:(data.rbegin())->first),
			t1(data.rbegin()->first),
			newTime(data.rbegin()->first+deltaT),
			p0(data.size()>1?(++(data.rbegin()))->second:(data.rbegin())->second)
{
	//last state rate couple
	SimulaPoint::Table::reverse_iterator it(data.rbegin());
	p1 = (it->second);
	//t1 = (it->first);

	//second last state rate couple
	if (data.size() > 1)
		++it;
	//p0 = (it->second);
	//t0 = (it->first);

	//set predictor
	t2 = t1 + deltaT;
	//newTime=t2;
	p2 = p1;
	if (data.size() > 1) {
		//first order
		linearInterpolation(t0, t1, t2, p0.rate, p1.rate, p2.rate, true);
		linearInterpolation(t0, t1, t2, p0.state, p1.state, p2.state);
	}

	//set step
	step = 1;
}

void SimplePredictor::resetTimeStep(double dt){
	//set predictor
	double oldt2=t2;
	t2 = t1 + dt;
	//set step
	//todo, not safe for simulapoint
	linearInterpolation(t1, oldt2, t2, s1.rate, s2.rate, s2.rate);
	linearInterpolation(t1, oldt2, t2, s1.state, s2.state, s2.state);
}


IntegrationBase::IntegrationBase() :
		pSD(nullptr), predictor(nullptr), multiplier(nullptr), rateMultiplier(nullptr) {
}

IntegrationBase::~IntegrationBase() {
}




void IntegrationBase::initiate(SimulaTimeDriven* pSP,
		DerivativeBase & rateCalculator) {
	if (!pSD) {
		pSD = pSP;
		//set defaultSpatialIntegrationLength
		if (defaultSpatialIntegrationLength < 0) {
			//read default precision for spatial integration - this is a parameter in cm
			SimulaBase* o(
					ORIGIN->getChild("simulationControls")->existingChild(
							"integrationParameters"));
			if(o) o = o->existingChild("defaultSpatialIntegrationLength");
			if (o) {
				o->get(defaultSpatialIntegrationLength);
				if (o->getUnit().order != Unit("cm").order)
					msg::error(
							"IntegrationBase: Unit for IntegrationParameters/defaultSpatialIntegrationLength must be of order length i.e. use cm or mm etc");
				//check this is unit is of order length
				//convert to meters
				defaultSpatialIntegrationLength *= o->getUnit().factor;
				//convert to cm
				defaultSpatialIntegrationLength /= Unit("cm").factor;
			}
			if (defaultSpatialIntegrationLength <= 0) {
				defaultSpatialIntegrationLength = 1;
			}
			msg::warning("IntegrationBase:: setting defaultSpatialIntegrationLength to "+std::to_string(defaultSpatialIntegrationLength)+".",1);
		}
	}
	if(pSP->existingChild("multiplier")){
		multiplier = pSP->getChild("multiplier");
	}
	if(pSP->existingChild("rateMultiplier")){
		rateMultiplier = pSP->getChild("rateMultiplier");
	}
}


void IntegrationBase::integrate(SimulaVariable::Table & data,
		DerivativeBase & rateCalculator) {
	msg::error(
			"IntegrationBase:: integrate (SimulaVariable) is not implemented, please provide implementation for "
					+ this->getName()
					+ " or use different integration method for "
					+ pSD->getPath());
}

void IntegrationBase::integrate(SimulaPoint::Table & data,
		DerivativeBase & rateCalculator) {
	msg::error(
			"IntegrationBase:: integrate (SimulaPoint) is not implemented, please provide implementation for "
					+ this->getName()
					+ " or use different integration method for "
					+ pSD->getPath());
}

void IntegrationBase::predict(const Time&t, double &ps) {//, SimulaVariable::Table & data){
	//timestep
	Time deltaT = t - predictor->t1;
	//integration
	if (predictor->step == 1) {				//forward
		ps = predictor->s1.state + predictor->s1.rate * deltaT;
	} else {				//backward
		ps = predictor->s1.state + predictor->s2.rate * deltaT;
	}
}

void IntegrationBase::predictRate(const Time&t, double &ps) {
	//interpolate the rate
	const double t0= predictor->step==1? predictor->t0 : predictor->t1;
	const double t1= predictor->step==1? predictor->t1 : predictor->t2;
	const double r0= predictor->step==1? predictor->s0.rate : predictor->s1.rate;
	const double r1= predictor->step==1? predictor->s1.rate : predictor->s2.rate;
	/*const double t0= predictor->t1;
	const double t1= predictor->t2;
	const double r0= predictor->s1.rate;
	const double r1= predictor->s2.rate;*/
	if (t<t0) msg::error("IntegrationBase: Asked to predict backwards in time");

	//linear extrapolation based on the change in the previous timestep
	if(t1>t0){
		linearInterpolation(t0, t1, t,r0, r1, ps);
		//avoid sign changes in case of extrapolation
		if (r0 >= 0. && r1 >= 0. && ps < 0.)	ps = 0.;
		if (r0 <= 0. && r1 <= 0. && ps > 0.)	ps = 0.;
	}else{
		ps=r0;
	}
}

void IntegrationBase::predict(const Time&t, Coordinate& pp) {
	//delta t
	Time deltaT = t - predictor->t1;

	//integration
	if (predictor->step == 1) {				//forward
		pp = predictor->p1.state + predictor->p1.rate * deltaT;
	} else {				//backward
		pp = predictor->p1.state + predictor->p2.rate * deltaT;
	}
}

void IntegrationBase::predictRate(const Time&t, Coordinate&pp) {
	//interpolate the rate
	/*const double t0= predictor->step==1? predictor->t0 : predictor->t1;
	const double t1= predictor->step==1? predictor->t1 : predictor->t2;
	const Coordinate r0= predictor->step==1? predictor->p0.rate : predictor->p1.rate;
	const Coordinate r1= predictor->step==1? predictor->p1.rate : predictor->p2.rate;*/
	const double t0= predictor->t1;
	const double t1= predictor->t2;
	const Coordinate r0= predictor->p1.rate;
	const Coordinate r1= predictor->p2.rate;
	if (t<t0) msg::error("IntegrationBase: Asked to predict backwards in time");

	//linear extrapolation based on the change in the previous timestep
	linearInterpolation(t0, t1, t,r0, r1, pp);
}

void IntegrationBase::getTimeStepInfo(Time & st, Time & rt) const {
	if (predictor) {
		///todo this is not safe as the predictor could easily have another implementation.
		st = predictor->t1;
		rt = predictor->newTime;
	} else {
		st = -1;
		rt = -1;
	}
}

void IntegrationBase::setTolerance(const double tol){
	//do nothing
};


void IntegrationBase::timeStep(const Time & lastTime, Time & newTime,
		Time & deltaT, const double & l) {

	//adjust to spatial resolution
	if (l > 1e-5) {
		//scale in reference to l
		deltaT = std::min(pSD->preferedTimeStep(),defaultSpatialIntegrationLength / l);
	}else{
		deltaT = pSD->preferedTimeStep();
	}

	//maxmimum and minimum timesteps
	deltaT = std::max(pSD->minTimeStep(), deltaT);
	deltaT = std::min(pSD->maxTimeStep(), deltaT);

	//newTime
	newTime = lastTime + deltaT;

	//round off timesteps to multiple of sync time step
	Time synctime = SYNCTIMESTEP * floor((newTime + TIMEERROR) / SYNCTIMESTEP);
	if (synctime - lastTime > TIMEERROR) {
		//snapping the time step to the default rhythm at which the other processes synchronize.
		newTime = synctime;
		deltaT = newTime - lastTime;
	} else {
		//trying to balance the time step so that the time step snapping does not cause large oscillations in the time step.
		double steps = ceil(SYNCTIMESTEP / deltaT);
		deltaT = SYNCTIMESTEP / steps;
		newTime = lastTime + deltaT;
	}

	//do not run ahead of objects that are calling this.
	Time wt = pSD->getWallTime(newTime);
	newTime = wt;

	//first finish "non" prediction timesteps
	Time t0,t1;
	pSD->getGlobalTimeStepInfo(t0,t1);
	if(newTime> t0 && lastTime<t0-TIMEERROR){
		newTime=t0;
	}

	deltaT = newTime - lastTime;
}

double IntegrationBase::defaultSpatialIntegrationLength(-1);

std::map<std::string, derivativeBaseInstantiationFunction> & BaseClassesMap::getDerivativeBaseClasses() {
	static std::map<std::string, derivativeBaseInstantiationFunction> derivativeBaseClasses;
	return derivativeBaseClasses;
}
std::map<std::string, objectGeneratorInstantiationFunction> & BaseClassesMap::getObjectGeneratorClasses() {
	static std::map<std::string, objectGeneratorInstantiationFunction> objectGeneratorClasses;
	return objectGeneratorClasses;
}
std::map<std::string, integrationInstantiationFunction> & BaseClassesMap::getIntegrationClasses() {
	static std::map<std::string, integrationInstantiationFunction> integrationClasses;
	return integrationClasses;
}
