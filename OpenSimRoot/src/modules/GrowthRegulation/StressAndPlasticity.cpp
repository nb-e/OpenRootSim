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
#include "StressAndPlasticity.hpp"
#include "../../cli/Messages.hpp"
#include "../../math/MathLibrary.hpp"
#include <math.h>
#include "../PlantType.hpp"

///@TODO include unit checking
NutrientStressFactor::NutrientStressFactor(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	uptake = pSD->getChild("plantNutrientUptake");
	target = pSD->getChild("plantOptimalNutrientContent");
	minimum = pSD->getChild("plantMinimalNutrientContent");
	msg::warning(
			"NutrientStressFactor:: You are using the first version which is much slower. Please use .V2 which is faster and produces more or less the same results, using integration.");
}
void NutrientStressFactor::calculate(const Time &t, double &result) {
	//TODO we may want to convert this to change in nutrient stress factor.
	//uptake and target nutrient content
	double u, o, m;
	uptake->get(t, u);
	target->get(t, o);
	minimum->get(t, m);

	// stress factor 1=no stress 0=maximum stress
	if (o != m) {
		result = (u - m) / (o - m);
	} else if (u < o) {
		result = 0.;
	} else {
		//TODO check this logic. It at least avoids problems at time 0
		result = 1.;
	}

	//Set boundaries
	if (result < 1.e-3) {
		msg::warning(
				"NutrientStressFactor: stress is severe, plant responses by the model may not be accurate");
		result = 1.e-3;
	} else if (result > 1.) {
		result = 1.;
	} else if (!std::isnormal(result)) {
		msg::error("NutrientStressFactor: value is NaN of inf");
	}
}

std::string NutrientStressFactor::getName() const {
	return "nutrientStressFactor";
}
DerivativeBase * newInstantiationNutrientStressFactor(
		SimulaDynamic* const pSD) {
	return new NutrientStressFactor(pSD);
}

NutrientStressFactorV2::NutrientStressFactorV2(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	uptake = pSD->getSibling("plantNutrientUptake");
	fixation = pSD->existingSibling("plantNutrientFixation");
	target = pSD->getSibling("plantOptimalNutrientContent");
	minimum = pSD->getSibling("plantMinimalNutrientContent");
	pSD->setInitialValue(1);	// no stress initially
	corrected = true;
}
void NutrientStressFactorV2::calculate(const Time &t, double &result) {
	//just set to zero, no change and work it out with the postIntegrationCorrection
	result = 0;
	corrected = false;
}
bool NutrientStressFactorV2::postIntegrationCorrection(
		SimulaVariable::Table & data) {
	//done post integration, so it is remembered, but not integrated. better to change this to another integratin method.
	bool r(false);
	//iterators
	SimulaVariable::Table::iterator eit(data.end());
	--eit;
	SimulaVariable::Table::iterator pit(eit);
	--pit;
	if (!corrected) {
		//target result
		double u, o, m;
		uptake->get(eit->first, u);
		if (fixation) {
			fixation->get(eit->first, o);
			u += o;
		}
		target->get(eit->first, o);
		minimum->get(eit->first, m);
		double res(o - m);
		if (res != 0) {
			res = (u - m) / res;
			res = fmin(1., fmax(0., res));
		} else {
			res = 1;
		}
		//calculate the change in stress factor using finite difference
		pit->second.rate = (res - pit->second.state)
				/ (eit->first - pit->first);
		if (u > 0) {		//&& u-o<u*1.2){
			uptake->getRate(eit->first, u);
			if (fixation) {
				fixation->getRate(eit->first, o);
				u += o;
			}
			target->getRate(eit->first, o);
			if (u > o) {
				//plevels are increasing be conservative
				//note that than in the next timestep the above calculated difference will be larger
				pit->second.rate *= 0.6;
			} else {
				//plevels are decreasing
				if (pit->second.rate >= 0) {
					//if(pit->second.state>0.999){
					//there is no stress, but we are moving towards it.
					//pit->second.rate=0;///-0.1;
					//}else{
					//there is stress, the stress is increasing based on rates, but decreasing based on last integration.
					//avoid correction in wrong direction
					//pit->second.rate=0;
					//}
				} else {
					//be aggressive so next timestep we expect less adjustment
					pit->second.rate *= 1.5;
				}
			}
		}

		//done redo integration
		corrected = true;
		r = true;
	} else {
		//Set boundaries between 0 and 1
		if (eit->second.state < 0) {
			//change rate
			pit->second.rate = (1e-3 - pit->second.state)
					/ (eit->first - pit->first);
			r = true;
		} else if (eit->second.state > 1) {
			//change rate
			pit->second.rate = (0.99999 - pit->second.state)
					/ (eit->first - pit->first);
			r = true;
		};
	}
	eit->second.rate = pit->second.rate;
	return r;
}

std::string NutrientStressFactorV2::getName() const {
	return "nutrientStressFactor.V2";
}
DerivativeBase * newInstantiationNutrientStressFactorV2(
		SimulaDynamic* const pSD) {
	return new NutrientStressFactorV2(pSD);
}

StressFactor::StressFactor(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//impact factor: extract from name
	std::string target(pSD->getName());
	target.erase(0, 13);					//remove 'stressFactor4' from name

	//parameter set
	SimulaBase * param(nullptr);
	std::string plantType;
	if (target.size() > 0) {
		pSD->getSibling("plantType")->get(plantType);
		param = GETPLANTPARAMETERS(plantType);
		//changed this for otherwise 1:1 relation is assumed
		param = param->getChild("stressImpactFactors")->existingChild(target);
		if (!param)
			msg::warning(
					"StressFactor: " + target
							+ " not found in stressImpactFactors for plant type "
							+ plantType);
	}

	//list of nutrient stress factors and their impact factors
	SimulaBase::List n;
	pSD->getParent()->getAllChildren(n);
	for (auto & it : n) {
		//add stress factors
		SimulaBase * l((it)->existingChild("nutrientStressFactor"));
		if (l) {
			// add to list of nutrient stress factors
			nutrients.push_back(l);
			// add corresponding impact factor
			std::string nutrient(l->getParent()->getName());
			if (param) {
				SimulaBase * p(param->getChild("impactBy:" + nutrient));
				impactFactors.push_back(p);
			} else {
				impactFactors.push_back(nullptr);
				if (pSD->getName() != "stressFactor")
					msg::warning(
							"StressFactor: no impact found for " + nutrient
									+ " and " + pSD->getName(), 1);
			}
		}
	}
	if (nutrients.empty())
		msg::warning("StressFactor: No nutrient stresses found");
}
void StressFactor::calculate(const Time &t, double &m) {
	//find the minimum factor
	m = 1e10 * (double) nutrients.size() + 1.;//1 one if there are not nutrients in the container.
	double n, e;
	for (std::size_t i(0); i < nutrients.size(); ++i) {
		nutrients[i]->get(t, n);
		if (impactFactors[i]) {
			impactFactors[i]->get(n, e);
			n = e;
		}
		m = fmin(m, n);
	}
	//if(m<0)m=0;
	//if(m>1)m=1; #turned this off so that we can have increased root growth as well.
	//hack
	//m.estimate=false;
}
std::string StressFactor::getName() const {
	return "stressFactor";
}

DerivativeBase * newInstantiationStressFactor(SimulaDynamic* const pSD) {
	return new StressFactor(pSD);
}

LocalNutrientResponse::LocalNutrientResponse(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//impact factor: extract from name
	std::string target(pSD->getName());
	if (target == "multiplier" || target == "rateMultiplier") {
		target = "impactOn:" + pSD->getParent()->getName();
	} else {
		target = "impactOn:" + target;
		size_t len = 10;
		target.erase(target.size() - len, len);
	}

	//parameter set
	std::string plantType;
	SimulaBase *top(pSD);
	PLANTTOP(top)
	top->getChild("plantType")->get(plantType);
	SimulaBase * param(pSD->getParent(2));
	if (param->getName() == "growthpoint")
		param = param->getParent();
	std::string rootType;
	param->getChild("rootType")->get(rootType);
	param = GETROOTPARAMETERS(plantType, rootType);
	param = param->getChild("localResourceResponses")->getChild(target);

	//moderation of the local response by the plant nutrient status
	SimulaBase * paramgl = GETPLANTPARAMETERS(plantType);
	paramgl = paramgl->getChild("stressImpactFactors")->getChild(
			target + "Multiplier");

	//aggregation function
	std::string func;
	param->getChild("aggregationFunction")->get(func);
	if (func == "minimum") {
		aggregationFunction = vectorsMinimum;
	} else if (func == "average") {
		aggregationFunction = vectorsAverage;
	} else if (func == "multiplicative") {
		aggregationFunction = elementMultiplication;
	} else if (func == "maximum") {
		aggregationFunction = vectorsMaximum;
	} else if (func == "maxRelativeDeviationFromOne") {
		aggregationFunction = vectorsMaxRelativeDeviationFromOne;
	}

	//list of nutrient stress factors and their impact factors
	SimulaBase::List n;
	SimulaBase *gp(pSD->getParent(1));
	if (gp->getName() != "growthpoint")
		gp = gp->getParent();
	if (gp->getName() != "growthpoint")
		msg::error("LocalNutrientResponse: did not find the growthpoint");
	gp->getAllChildren(n);
	position = pSD;

	SimulaBase * l;
	std::string field("nutrientConcentrationAtTheRootSurface");
	l = pSD->existingChild("nameFactor");
	if (l)
		l->get(field);
	for (auto & it : n) {
		//add stress factors
		l = ((it)->existingChild(field));
		if (l) {
			//todo this may need historical data, depending how it is used, but this will increase memory usage
			l->garbageCollectionOff();
			// add to list of nutrient stress factors
			nutrients.push_back(l);
			msg::warning(
					"LocalNutrientResponse:'" + field + "' found for '"
							+ l->getParent()->getName() + "'.", 1);
			// add corresponding impact factor
			std::string nutrient(l->getParent()->getName());//ASSUMING THIS IS USED AS MULTIPLIER
			//global nutrient status
			nutrientsMod.push_back(
					top->getChild(nutrient)->getChild("nutrientStressFactor"));
			//local impact
			nutrient.insert(0, "impactBy:");
			impactFactors.push_back(param->getChild(nutrient));
			//modulation of local response
			impactFactorsMod.push_back(paramgl->getChild(nutrient));
		}
	}

	if (nutrients.empty())
		msg::warning(
				"LocalNutrientResponse:'" + field
						+ "' not found for any nutrient.");
}

void LocalNutrientResponse::calculate(const Time &t, double &m) {
	//build array of impact factors
	if (nutrients.size() > 0) {
		Coordinate pos;
		position->getAbsolute(t, pos);
		calculate(t, pos, m);
	} else {
		m = 1;
	}
}

void LocalNutrientResponse::calculate(const Time &t, const Coordinate &pos,
		double &m) {
	//build array of impact factors
	if (nutrients.size() > 0) {
		std::vector<double> v;
		v.clear();
		double conc, impact, concmod(1), impactmod(1);
		for (unsigned int i(0); i < nutrients.size(); ++i) {
			nutrients[i]->get(t, pos, conc);
			impactFactors[i]->get(conc, impact);
			nutrientsMod[i]->get(t, concmod);
			impactFactorsMod[i]->get(concmod, impactmod);
			impact = ((impact - 1) * impactmod) + 1;
			v.push_back(impact);
			//std::cout<<std::endl<<"==impact=="<<impact<<std::endl;
		}
		//use aggregation function to aggregate this
		if (v.size() > 1) {
			m = aggregationFunction(v);
		} else {
			m = impact;
		}
	} else {
		m = 1;
	}
}

std::string LocalNutrientResponse::getName() const {
	return "localNutrientResponse";
}

DerivativeBase * newInstantiationLocalNutrientResponse(
		SimulaDynamic* const pSD) {
	return new LocalNutrientResponse(pSD);
}

BFMmemory::BFMmemory(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//multiplier
	SimulaBase *m = pSD->getParent(3)->getChild("growthpoint")->getChild(
			pSD->getName());
	//time at which to get the multipliers value
	Time mt(pSD->getStartTime());
	//get value
	m->get(mt, mem);
}

void BFMmemory::calculate(const Time &t, double &m) {
	m = mem;
}
std::string BFMmemory::getName() const {
	return "BFMmemory";
}

DerivativeBase * newInstantiationBFMmemory(SimulaDynamic* const pSD) {
	return new BFMmemory(pSD);
}

//Register the module
static class AutoRegisterNutrientStressFactorInstantiationFunctions {
public:
	AutoRegisterNutrientStressFactorInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["nutrientStressFactor"] =
				newInstantiationNutrientStressFactor;
		BaseClassesMap::getDerivativeBaseClasses()["nutrientStressFactor.V2"] =
				newInstantiationNutrientStressFactorV2;
		BaseClassesMap::getDerivativeBaseClasses()["stressFactor"] =
				newInstantiationStressFactor;
		BaseClassesMap::getDerivativeBaseClasses()["localNutrientResponse"] =
				newInstantiationLocalNutrientResponse;
		BaseClassesMap::getDerivativeBaseClasses()["BFMmemory"] =
				newInstantiationBFMmemory;
	}
	;
} p466446584123556;

