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

#include "IntegrationLibrary.hpp"
#include "../../math/InterpolationLibrary.hpp"
#include "../../engine/SimulaVariable.hpp"
#include "../../engine/SimulaPoint.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../math/VectorMath.hpp"
#include <math.h>
#include "../../engine/Origin.hpp"
#include "../../math/MathLibrary.hpp"


#define INT_INIT 	Time lastTime = data.rbegin()->first, newTime, deltaT;\
					timeStep(lastTime, newTime, deltaT, l);\
					predictor = new SimplePredictor(data, deltaT);\
					bool e=false;\

#define DO_STEP(T,R)	rateCalculator.calculate(T, R);\
					if (rateMultiplier) {\
						double temp;\
						rateMultiplier->get(T, temp);\
						R *= temp;\
					}\
					e?pSD->keepEstimate(T,false):e=pSD->keepEstimate(T,false);\


#define DO_FINAL_STEP(T,R)	rateCalculator.calculate(T, R);\
					if (rateMultiplier) {\
						double temp;\
						rateMultiplier->get(T, temp);\
						R *= temp;\
					}\
					pSD->keepEstimate(T,!e);\


#define POST_INT(S0) 	int count(0);\
					while (rateCalculator.postIntegrationCorrection(data)) {\
						auto eit(data.rbegin()), peit(eit);\
						++peit;\
						eit->second.state = S0 + peit->second.rate * deltaT;\
						++count;\
						if (count > 10) {\
							msg::warning("postIntegrationCorrection: needing more than 10 iterations, continuing with imprecise results for "+ pSD->getName());\
							break;\
						}\
					}\
					delete predictor;\
					predictor = nullptr;\




//======================Integration functions===================================
//Integration using forward euler
ForwardEuler::ForwardEuler() :
		IntegrationBase(), totError(0), totChange(0), updateRate(0), callCount(
				0) {
}
void ForwardEuler::integrate(SimulaVariable::Table & data,
		DerivativeBase &rateCalculator) {

	//timestepping
	const double l=0;
	INT_INIT

	//update rate
	double dummy(predictor->s1.rate);
	DO_STEP(predictor->t1,dummy);

	//insert updated predictor
	predictor->s1.rate = dummy;
	data.rbegin()->second.rate = dummy;

	//forward euler
	StateRate newValue(predictor->s1.state + dummy * deltaT, dummy);
	data.insert(std::pair<Time, StateRate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime,dummy);
	data.rbegin()->second.rate = dummy;

	//post integration correction
	POST_INT(predictor->s1.state)
}

void ForwardEuler::integrate(SimulaPoint::Table & data,
		DerivativeBase & rateCalculator) {
	//timestepping
	const double l=data.rbegin()->second.rate.length();
	INT_INIT

	//update rate
	Coordinate dummy(predictor->p1.rate);
	DO_STEP(predictor->t1,dummy);

	//insert updated predictor
	predictor->p1.rate = dummy;
	data.rbegin()->second.rate = dummy;

	//forward euler
	MovingCoordinate newValue(predictor->p1.state + dummy * deltaT, dummy);
	data.insert(std::pair<Time, MovingCoordinate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime,dummy);
	data.rbegin()->second.rate = dummy;

	//post integration correction
	POST_INT(predictor->p1.state)
}

ForwardEuler::~ForwardEuler() {
	//warn about total errors
//	if (callCount > 20) {//only report for objects that existed for significant amount of time
//		if (1e2 * totError > totChange)
//			msg::warning(
//					"ForwardEuler: results may not be accurate for " + name);
//		if (1e8 * totError < totChange)
//			msg::warning(
//					"ForwardEuler: you could probably use a lager (minimum) time step for "
//							+ name);
//	}
}
std::string ForwardEuler::getName() const {
	return "ForwardEuler";
}

BackwardEuler::BackwardEuler() :
		IntegrationBase() {
}
void BackwardEuler::integrate(SimulaVariable::Table & data,
		DerivativeBase &rateCalculator) {
	//timestepping
	const double l=0;
	INT_INIT

	//update rate
	double dummy(predictor->s1.rate);
	DO_STEP(newTime,dummy);

	//insert updated predictor
	predictor->s1.rate = dummy;

	//backward euler
	StateRate newValue(predictor->s1.state + dummy * deltaT, dummy);
	data.insert(std::pair<Time, StateRate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime,dummy);
	data.rbegin()->second.rate = dummy;

	//post integration correction
	POST_INT(predictor->s1.state)
}

void BackwardEuler::integrate(SimulaPoint::Table & data,
		DerivativeBase &rateCalculator) {
	//timestepping
	const double l=data.rbegin()->second.rate.length();
	INT_INIT

	//update rate
	Coordinate dummy(predictor->p1.rate);
	DO_STEP(predictor->t1, dummy);

	//insert updated predictor
	predictor->p1.rate = dummy;

	//backward euler
	MovingCoordinate newValue(predictor->p1.state + dummy * deltaT, dummy);
	data.insert(std::pair<Time, MovingCoordinate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime, dummy);
	data.rbegin()->second.rate = dummy;

	//post integration correction
	POST_INT(predictor->p1.state)
}


std::string BackwardEuler::getName() const {
	return "BackwardEuler";
}

//heun's method - this is the average of a forward and backward euler.
Heuns::Heuns() :
		IntegrationBase() {
}
void Heuns::integrate(SimulaVariable::Table & data,
		DerivativeBase & rateCalculator) {
	//timestepping
	const double l=0;
	INT_INIT

	//forward step
	double dummy;
	DO_STEP(predictor->t1,dummy);

	//insert updated predictor
	predictor->s1.rate = dummy;
	data.rbegin()->second.rate = dummy;

	//backward step
	double dummy2;
	DO_STEP(newTime,dummy2);

	//Heuns
	StateRate newValue(predictor->s1.state + (dummy+dummy2)* deltaT/2. , dummy2);
	data.insert(std::pair<Time, StateRate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime,dummy);
	data.rbegin()->second.rate = dummy;

	//post integration correction
	POST_INT(predictor->s1.state)

}

void Heuns::integrate(SimulaPoint::Table & data,
		DerivativeBase & rateCalculator) {
	//timestepping
	const double l=data.rbegin()->second.rate.length();
	INT_INIT

	//forward step
	Coordinate dummy;
	DO_STEP(predictor->t1,dummy);

	//insert updated predictor
	predictor->p1.rate = dummy;
	data.rbegin()->second.rate = dummy;

	//backward step
	Coordinate dummy2;
	DO_STEP(newTime,dummy2);

	//Heuns
	MovingCoordinate newValue(predictor->p1.state + (dummy+dummy2)* deltaT/2. , dummy2);
	data.insert(std::pair<Time, MovingCoordinate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime,dummy);
	data.rbegin()->second.rate = dummy;

	//post integration correction
	POST_INT(predictor->p1.state)
}
std::string Heuns::getName() const {
	return "Heuns";
}

//HeunsII
///@todo numerical error estimate: HeunsII error comes from the assumption that second derivative is linear ( not constant as in forward euler)
HeunsII::HeunsII() :
		IntegrationBase() {
}
void HeunsII::integrate(SimulaVariable::Table & data,
		DerivativeBase & rateCalculator) {
	msg::error(
			"HeunsII: HeunsII is deprecated. Use a different method for "
					+ pSD->getPath());
}

std::string HeunsII::getName() const {
	return "HeunsII";
}

//Runge-kutta 4
///@todo - it would be better if we used a higher order interpolation method with this.
RungeKutta4::RungeKutta4() :
		IntegrationBase() {
}
void RungeKutta4::integrate(SimulaVariable::Table & data,
		DerivativeBase & rateCalculator) {
	//timestepping
	const double l=0;
	INT_INIT

	//step 1
	double r1;
	DO_STEP(predictor->t1,r1);

	//insert updated predictor
	predictor->s1.rate = r1;
	data.rbegin()->second.rate = r1;

	//half time
	Time ht(lastTime + deltaT / 2);

	//step 2
	double r2;
	DO_STEP(ht,r2);
	predictor->s2.rate = r2;
	predictor->step = 2;
	predictor->t2 = ht;	//for extrapolation of predicted rates - but also reduces the timestep of called models

	//step 3
	double r3;
	DO_STEP(ht,r3);
	predictor->s2.rate = r3;

	//step 4
	double r4;
	DO_STEP(newTime,r4);
	predictor->s2.rate = r4;
	predictor->t2 = newTime;

	//RK4
	StateRate newValue(predictor->s1.state + (r1 + ((r2 + r3) * 2) + r4)*deltaT / 6. , r4);
	bool insert=true;
	if(data.size()>3){
		auto it=data.rbegin();++it;
		if(r1==r4 && r4==it->second.rate ) {//constant rates
			if(newTime-it->first < 0.5 && pSD->getProgressTime()!=data.rbegin()->first) insert=false;
		}
	}
	if(insert){
		data.insert(std::pair<Time, StateRate>(newTime, newValue));
	}else{
#ifdef USE_CUSTOM_MAP
		data.erase(--data.end());//normal maps do not allow this as it could destroy the sorting. Here we are fine.
		data.insert(std::pair<Time, StateRate>(newTime, newValue));
#else
		data.rbegin()->first=newTime;
		data.rbegin()->second.state=newValue.state;
#endif
	}

	//rate correction step
	DO_FINAL_STEP(newTime,r1);
	data.rbegin()->second.rate = r1;

	//post integration correction
	POST_INT(predictor->s1.state)
}

void RungeKutta4::integrate(SimulaPoint::Table & data,
		DerivativeBase & rateCalculator) {
	//timestepping
	const double l=data.rbegin()->second.rate.length();
	INT_INIT

	//step 1
	Coordinate r1;
	DO_STEP(predictor->t1,r1);

	//insert updated predictor
	predictor->p1.rate = r1;
	data.rbegin()->second.rate = r1;

	//half time
	Time ht(lastTime + deltaT / 2);

	//step 2
	Coordinate r2;
	DO_STEP(ht,r2);
	predictor->p2.rate = r2;
	predictor->step = 2;
	predictor->t2 = ht;	//for extrapolation of predicted rates - but also reduces the timestep of called models

	//step 3
	Coordinate r3;
	DO_STEP(ht,r3);
	predictor->p2.rate = r3;

	//step 4
	Coordinate r4;
	DO_STEP(newTime,r4);
	predictor->p2.rate = r4;
	predictor->t2 = newTime;

	//RK4
	MovingCoordinate newValue(predictor->p1.state + (r1 + ((r2 + r3) * 2) + r4)*deltaT / 6. , r4);
	bool insert=true;
	if(data.size()>3){
		auto it=data.rbegin();++it;
		if(r1==r4 && r4==it->second.rate ) {//constant rates
			if(newTime-it->first < 0.5 && pSD->getProgressTime()!=data.rbegin()->first) insert=false;
		}
	}
	if(insert){
		data.insert(std::pair<Time, MovingCoordinate>(newTime, newValue));
	}else{
#ifdef USE_CUSTOM_MAP
		data.rbegin()->first=newTime;//normal maps do not allow this as it could destroy the sorting. Here we are fine.
		data.rbegin()->second.state=newValue.state;
#else
		data.erase(--data.end());
		data.insert(std::pair<Time, MovingCoordinate>(newTime, newValue));
#endif
	}

	//rate correction step
	DO_FINAL_STEP(newTime,r1);
	data.rbegin()->second.rate = r1;

	//post integration correction
	POST_INT(predictor->p1.state)
}

std::string RungeKutta4::getName() const {
	return "RungeKutta4";
}

//convergence solver Rates
ConvergenceSolverRates::ConvergenceSolverRates() :
		IntegrationBase() {
}

std::string ConvergenceSolverRates::getName() const {
	return "explicitConvergence";
}

void ConvergenceSolverRates::integrate(SimulaVariable::Table & data,
		DerivativeBase & rateCalculator) {
	//timestepping
	const double l=0;
	INT_INIT

	//step 1
	double r1;
	DO_STEP(predictor->t1,r1);

	//insert updated predictor
	predictor->s1.rate = r1;
	data.rbegin()->second.rate = r1;

	//iteration
	int counter(0);
	int maxcount(10);
	double d(1);
	StateRate newValue(predictor->s2);
	while (d > 0.00001 && counter < maxcount) {			//
		//counter
		++counter;
		//rate calculation
		DO_STEP(newTime, newValue.rate);
		//convergence criteria
		if (newValue.rate != 0) {
			d = absolute((newValue.rate - predictor->s2.rate) / newValue.rate);
		} else if (predictor->s2.rate != 0) {
			d = absolute(
					(newValue.rate - predictor->s2.rate) / predictor->s2.rate);
		} else {
			d = 0;
		}
		//if(!newValue.rate.estimate) d=0;
		//integration
		newValue.state = predictor->s1.state + newValue.rate * deltaT;
		//Preparation for next round
		predictor->s2.rate = newValue.rate;
		//from now on do backward euler for estimates
		predictor->step = 2;
	}
	if (counter == maxcount)
		msg::warning(
				"ConvergenceSolver: needing more than "
						+ std::to_string(maxcount)
						+ " iterations, continuing with less precise results for "
						+ pSD->getName());

	//RK4
	data.insert(std::pair<Time, StateRate>(newTime, newValue));

	//rate correction step
	DO_FINAL_STEP(newTime,r1);
	data.rbegin()->second.rate = r1;

	//post integration correction
	POST_INT(predictor->s1.state)
}


//==================auto registration of the functions=================

IntegrationBase * newInstantiationDefaultIntegration() {
	return new ForwardEuler();
}
IntegrationBase * newInstantiationBackwardEuler() {
	return new BackwardEuler();
}
IntegrationBase * newInstantiationHeuns() {
	return new Heuns();
}
IntegrationBase * newInstantiationHeunsII() {
	return new HeunsII();
}
IntegrationBase * newInstantiationRungeKutta4() {
	return new RungeKutta4();
}

IntegrationBase * newInstantiationConvergenceSolverRates() {
	return new ConvergenceSolverRates();
}

//Register the module
static class AutoRegisterIntegrationFunctions {
public:
	AutoRegisterIntegrationFunctions() {
		BaseClassesMap::getIntegrationClasses()["Euler"] =
				newInstantiationDefaultIntegration;
		BaseClassesMap::getIntegrationClasses()["ForwardEuler"] =
				newInstantiationDefaultIntegration;
		BaseClassesMap::getIntegrationClasses()["BackwardEuler"] =
				newInstantiationBackwardEuler;
		BaseClassesMap::getIntegrationClasses()["Heuns"] =
				newInstantiationHeuns;
		BaseClassesMap::getIntegrationClasses()["HeunsII"] =
				newInstantiationHeunsII;
		BaseClassesMap::getIntegrationClasses()["RungeKutta4"] =
				newInstantiationRungeKutta4;
		BaseClassesMap::getIntegrationClasses()["explicitConvergence"] =
				newInstantiationConvergenceSolverRates;
	}
} p44608510843540385;
