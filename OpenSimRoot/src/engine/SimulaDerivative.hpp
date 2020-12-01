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

#ifndef SIMULADERIVATIVE_HPP_
#define SIMULADERIVATIVE_HPP_
#include "SimulaDynamic.hpp"
#include "BaseClasses.hpp"
#include "../cli/Messages.hpp"
#include "../math/InterpolationLibrary.hpp"
#include "../math/MathLibrary.hpp"
#include "../math/VectorMath.hpp"
#include "TypeTraits.hpp"
#include <typeinfo>

template<class T = double, bool _interpolate = is_interpolatable<T>::value>
class SimulaDerivative: public SimulaDynamic {
public:
	typedef T Type;
	typedef Time Colum1;
	typedef T Colum2;
	typedef std::map<Colum1, Colum2> Table;
	SimulaDerivative(SimulaBase* newAttributeOf, const Time &newstartTime) :
			SimulaDynamic(newAttributeOf, "doNothing", UnitRegistry::noUnit(),
					newstartTime), timeCache(-1) {
	}
	SimulaDerivative(const std::string &newName, SimulaBase* newAttributeOf,
			const std::string &dF, const Unit &newUnit,
			const Time &newstartTime, const Time &newEndTime) :
			SimulaDynamic(newName, newAttributeOf, dF, newUnit, newstartTime,
					newEndTime), timeCache(-1) {
	}
	SimulaDerivative(SimulaBase* newAttributeOf, const Time &startTime,
			const SimulaDerivative* copyThis) :
			SimulaDynamic(newAttributeOf, startTime, copyThis), timeCache(-1) {
	}
	~SimulaDerivative() {
	}

	void get(const Time &t, Type &var) {
		if (absolute(timeCache-t) < TIMEERROR) {
			var = cache;
			return;
		}
		/*do the calculations*/
		getRateCalculator()->calculate(t, var);
		/*store cache*/
		cache = var;
		timeCache = t;
	}
	std::string getType() const {
		std::string type = "SimulaDerivative<";
		if (typeid(Type) == typeid(double)) {
			type += "double";
		} else if (typeid(Type) == typeid(int)) {
			type += "integer";
		} else if (typeid(Type) == typeid(std::string)) {
			type += "string";
		} else if (typeid(Type) == typeid(Coordinate)) {
			type += "Coordinate";
		} else if (typeid(Type) == typeid(bool)) {
			type += "bool";
		} else {
			type += "unknown";
		}
		type += ">";
		return type;
	}

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf,
			const Time & startTime) const {
		return new SimulaDerivative<T>(attributeOf, startTime, this);
	}


	template<class TVar>
	friend std::ostream &operator<<(std::ostream &os,
			const SimulaDerivative<TVar> &obj);
	template<class TVar>
	friend std::istream &operator>>(std::istream &is,
			const SimulaDerivative<TVar> &obj);
protected:
	Time timeCache;
	Type cache;
};
//cache size: larger is more beneficial when the number of growthpoints increase while too large creates an overhead in terms of memory and cpu power
//TODO it would really help if simuladerivative high in the tree had higher number while low in the tree had smaller cache
//TODO this has can have a large impact on memory usage / cpu usage.
//TODO this sometimes makes thinks worse
#include "../tools/StringExtensions.hpp"
//#define cachesize 10
template<class T>
class SimulaDerivative<T, true> : public SimulaDynamic {
public:
	typedef T Type;
	typedef Time Colum1;
	typedef T Colum2;
	typedef std::map<Colum1, Colum2> Table;
	SimulaDerivative(const std::string &newName, SimulaBase* newAttributeOf,
			const std::string &dF, const Unit &newUnit,
			const Time &newstartTime, const Time &newEndTime) :
			SimulaDynamic(newName, newAttributeOf, dF, newUnit, newstartTime,
					newEndTime), timeCache(-1) {
	}
	SimulaDerivative(SimulaBase* newAttributeOf, const Time &startTime,
			const SimulaDerivative* copyThis) :
			SimulaDynamic(newAttributeOf, startTime, copyThis), timeCache(-1) {
	}
	~SimulaDerivative() {
	}

	bool check(const unsigned int & predictorPosition) {
		bool r = false;
		for (unsigned int j(0); j < SimulaTimeDriven::callingOrder.size();
				++j) {
			for (unsigned int k(predictorPosition); k < predictorOrder.size();
					++k) {
				if (SimulaTimeDriven::callingOrder[j] == predictorOrder[k]) {
					r = true;

					break;
				}
			}
			if (r)
				break;
		}
		return r;
	}

	void get(const Time &t, Type &var) {
		if (absolute(timeCache-t) < TIMEERROR) {
			var = cache;
			return;
		}
		/*do the calculations*/
		unsigned int np(getPredictorSize());
		getRateCalculator()->calculate(t, var);

		if(multiplier_) {
			double m;
			multiplier_->get(t,m);
			var*=m;
		}
		/*store cache*/
		if (np>=getPredictorSize()) {
			cache = var;
			timeCache = t;
		}
	}

	void get(const Time &t, const Coordinate &pos, double &var) {
		/*if (absolute(timeCache-t) < TIMEERROR) {
			var = cache;
			return;
		}*/ //disabled cache as it needs to be position dependent.
		/*do the calculations*/
		//unsigned int np(getPredictorSize());
		getRateCalculator()->calculate(t, pos, var);//better so, as position is relevant, eventhough it can be retrieved.

		if(multiplier_) {
			double m;
			multiplier_->get(t,pos,m);
			var*=m;
		}

		/*store cache*/
		/*if (np>=getPredictorSize()) {
			cache = var;
			timeCache = t;
		}*/
	}

	void getRate(const Time &t, Type &var) {
		//estimate it using finite difference
		//note that if the finite difference is too small we may get, because of numerical errors, 0
		//todo uses global MINTIMESTEP which might be very small.
		Time d1 = MINTIMESTEP / 2.;
		Time t1 = t - d1;
		if (t1 < startTime)
			t1 = startTime;
		Time t2 = t1 + 2 * d1;
		Time rt = SimulaTimeDriven::getWallTime(t2);
		if (t2 > rt) {
			if (rt > t1) {
				t2 = rt;
			} else if (t1 < t) {
				t2 = t;
			}
		}
		Type r1, r2;
		get(t1, r1);
		get(t2, r2);
		var = (r2 - r1) / (t2 - t1);
	}

	std::string getType() const {
		std::string type = "SimulaDerivative<";
		if (typeid(Type) == typeid(double)) {
			type += "double";
		} else if (typeid(Type) == typeid(int)) {
			type += "integer";
		} else if (typeid(Type) == typeid(std::string)) {
			type += "string";
		} else if (typeid(Type) == typeid(Coordinate)) {
			type += "Coordinate";
		} else if (typeid(Type) == typeid(bool)) {
			type += "bool";
		} else {
			type += "unknown";
		}
		type += ">";
		return type;
	}

	///@todo this 'optimization algorithm' may not be completely robust when data is not approximately on a linear increase or decrease
	void getTime(const Type & var, Time & t, Time tmin = -1, Time tmax = 0) {
		//assume that t is intelligent guess (seed) but check if it is within boundaries
		if (!evaluateTime(tmin))
			tmin = startTime;
		//TODO could use midpoint between tmin and tmax if they are both within bounds
		if (!evaluateTime(t))
			msg::error(
					"SimulaDerivative::getTime: seed outside time boundaries of object");
		if (!evaluateTime(tmax))
			tmax = t;
		if (!withinBounds(tmin, tmax, t))
			t = (tmax + tmin) / 2;
		//solve this triangular
		Type rl, rs, ru;//rl is lower bound, ru is upper bound rs is midpoint estimate
		Time tl(tmin), ts(t), tu(tmax);		//idem
		get(tl, rl);		//get value at lower timepoint
		get(tu, ru);		//get value at later timepoint
		//if necessary swap times and values so that rl<ru
		if (rl > ru) {
			Time tt(tl);
			Type rt(rl);
			tl = tu;
			rl = ru;
			tu = tt;
			ru = rt;
		}
		double ref(1e-5); // allowed precision in days
		while (true) {
			if (var > ru || var < rl) {
				std::cout << std::endl << var << " " << ru << " " << rl;
				msg::error(
						"SimulaDerivative::getTime: target is out of bounds for "
								+ getName());
			}
			get(ts, rs); //get the new value for the new estimated point
			if (rs > ru || rs < rl)
				msg::error(
						"SimulaDerivative::getTime: more than one solution for "
								+ getName());
			double err(vectorlength(var - rs) * (tu - tl) / vectorlength(ru - rl)); //error estimate in terms of time
			if (err < -ref) {
				//solution between rl and rs
				ru = rs;
				tu = ts;
				linearInterpolation(rl, ru, var, tl, tu, ts);
				if (!withinBounds(tl, tu, ts))
					ts = (tl + tu) / 2;
			} else if (err > ref) {
				//solution between rs and ru
				rl = rs;
				tl = ts;
				linearInterpolation(rl, ru, var, tl, tu, ts);
				if (!withinBounds(tl, tu, ts))
					ts = (tl + tu) / 2;
			} else {
				//solved
				t = ts;
				break;
			}
		}
	}

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf,
			const Time & startTime) const {
		return new SimulaDerivative<T>(attributeOf, startTime, this);
	}


	template<class TVar>
	friend std::ostream &operator<<(std::ostream &os,
			const SimulaDerivative<TVar> &obj);
	template<class TVar>
	friend std::istream &operator>>(std::istream &is,
			const SimulaDerivative<TVar> &obj);
protected:
	Time timeCache;
	Type cache;
};


#endif /*SIMULADERIVATIVE_HPP_*/
