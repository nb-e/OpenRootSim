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
#include "Respiration.hpp"
#include "../../cli/Messages.hpp"
//#include "../../MathLibrary/VectorMath.hpp"
//#include "../../FunctionLibrary/InterpolationLibrary.hpp"
#include <math.h>
#include "../../engine/Origin.hpp"
#include "../PlantType.hpp"

/* Respiration should be calculated from mass, although the steel is thought to be
 * less active than the cortex. So a volume->mass-> respiration conversion may
 * not be accurate. */

//Respiration rate at any point in the rootsystem in g CO2/cm root/day
bool RootSegmentRespirationRate::issueMessage(true);
RootSegmentRespirationRate::RootSegmentRespirationRate(SimulaDynamic* pSD):DerivativeBase(pSD),sizeSimulator(nullptr), relativeRespirationSimulator(nullptr),
		factor(nullptr), aerenchymaSimulator(nullptr), aerenchymaCorrectionSimulator(nullptr),mode(0)
{
	//check if unit given in input file agrees with this function
	if (pSD->getUnit() != "g" && pSD->getUnit() != "g/day")
		msg::error("RootSegmentRespirationRate: unit should be in g or g/day");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType, pSD);
	//get the root type parameters
	std::string rootType;
	pSD->getParent(3)->getChild("rootType")->get(rootType);
	SimulaBase *parameters(GETROOTPARAMETERS(plantType, rootType));
	//find simulators in database
	std::string section(""),section2("");
	if (pSD->getName().find("Steel") != std::string::npos) {
		section = "Steel";
		section2 = "Steel";
	} else if (pSD->getName().find("Cortex") != std::string::npos) {
		//check if RCS effects respiration
		SimulaBase* contr(
				ORIGIN->getChild("simulationControls")->existingChild(
						"corticalSenescence"));
		if (contr)	contr = contr->getChild("reduceRespiration");
		bool flag(true);
		if (contr) contr->get(flag);
		if (flag) {
			section = "Cortex";
			if(pSD->getName().find("CortexNoRCS") != std::string::npos){
				section2 = "CortexNoRCS";
			}else{
				section2 = "Cortex";
			}
			if(contr) msg::warning("RootRespiration: respiration affected by RCS");
		}else{
			section = "Cortex";
			section2 = "CortexNoRCS";
			if(contr) msg::warning("RootRespiration: respiration not affected by RCS");
		}
	}
	//msg::warning("section is "+section+" "+section2+" for "+pSD->getName());

	//if steel and cortex are simulated, change the mode to adding upt two values
	relativeRespirationSimulator = pSD->existingSibling(
			"rootSegmentRespirationSteel");
	if (pSD->getName() == "rootSegmentRespiration" && relativeRespirationSimulator) {
		//add up steel and cortex
		mode = 1;
		sizeSimulator = pSD->existingSibling(
				"rootSegmentRespirationCortex");
	} else {
		relativeRespirationSimulator = parameters->existingChild(
				"relativeRespiration" + section);
		if (relativeRespirationSimulator) {
			if (relativeRespirationSimulator->getUnit() == "g/g/day") {
				sizeSimulator = pSD->getSibling(
						"rootSegmentDryWeight" + section2);
			} else if (relativeRespirationSimulator->getUnit() == "g/cm/day") {
				sizeSimulator = pSD->getSibling(
						"rootSegmentLength" + section2);
			} else if (relativeRespirationSimulator->getUnit() == "g/cm3/day") {
				sizeSimulator = pSD->getSibling(
						"rootSegmentVolume" + section2);
			} else {
				msg::error(
						"RootRespirationRate: relativeRespiration should be in either g/g/day or in g/cm/day or in g/cm3/day");
			}
		} else {
			msg::warning(
					"RootSegmentRespirationRate: relativeRespiration parameter missing for "
							+ rootType + ", setting respiration to 0 for "+pSD->getName());
		}
		//check for nutrient effects on respiration
		factor = pSD;
		PLANTTOP(factor);
		factor = factor->existingChild(
				"stressFactor:impactOn:rootSegmentRespiration");
		//Aerenchyma simulator
		SimulaBase* contr(
				ORIGIN->getChild("simulationControls")->existingChild(
						"aerenchyma"));
		if (contr)
			contr = contr->getChild("reduceRespiration");
		bool flag(true);
		if (contr)
			contr->get(flag);
		if (flag) {
			aerenchymaSimulator = pSD->existingSibling(
					"aerenchymaFormation", "100%");
			if (aerenchymaSimulator && issueMessage) {
				msg::warning(
						"RootSegmentRespirationRate: Reduction due to RCA simulated.");
				RootSegmentRespirationRate::issueMessage = false;
			}
		} else {
			aerenchymaSimulator = nullptr;
		}

		//Aerenchyma respiration Correction Factor
		aerenchymaCorrectionSimulator = parameters->existingChild(
				"reductionInRespirationDueToAerenchyma", "100%");
		if (aerenchymaSimulator && !aerenchymaCorrectionSimulator)
			msg::warning(
					"RootSegmentRespiration: aerenchymaFormation found in database but 'reductionInRespirationDueToAerenchyma' not found in parameter section, assuming it 1:1.");
		if (aerenchymaSimulator && section == "Steel") {
			aerenchymaSimulator = nullptr; //turn off RCA for respiration of steel.
		}

		//Aerenchyma respiration Correction Factor
		/*if(section=="Cortex"){
			SimulaBase *p=parameters->existingChild("reductionInRespirationDueToCorticalSenesence", "100%");
			if(aerenchymaCorrectionSimulator && p) msg::error("RootSegmentRespirationRate: Simultaneous simulation of RCA and RCS not implemented");
			aerenchymaCorrectionSimulator=pSD->getSibling("rootCorticalSenescenceStage");
			aerenchymaCorrectionSimulator=p;
		}*/
	}
}
void RootSegmentRespirationRate::calculate(const Time &t, double &rate) {
	//return 0 if respiration parameters are not set
	if (!relativeRespirationSimulator) {
		rate = 0;
		return;
	}

	if (mode == 1) {
		//just add up cortex and steel respiration
		double steel, cortex;
		relativeRespirationSimulator->get(t, steel);
		sizeSimulator->get(t, cortex);
		rate = steel + cortex;
		return;
	}

	//lenght of the root (cm) ->volume of 1 cm root in cm3
	sizeSimulator->get(t, rate);

	//current respiration rate in g CO2 /g/day or g CO2 /cm/day
	double r;
	relativeRespirationSimulator->get((t - pSD->getStartTime()), r);

	//current root segment respiration rate in g CO2/day
	rate *= r;

	//multiplication factor
	if (factor) {
		factor->get(t, r);
		rate *= r;
	}

	//aerenchyma formation
	if (aerenchymaSimulator) {
		double a;
		aerenchymaSimulator->get(t, a);
		if (aerenchymaCorrectionSimulator) {
			aerenchymaCorrectionSimulator->get(a, r);
			r = 1 - r;
		} else {
			r = a;
		}
		if (r > 1 || r < 0)
			msg::error(
					"RootSegmentRespirationRate: "+aerenchymaCorrectionSimulator->getName()+" not between 1-0.");
		rate *= r;
	}
}
std::string RootSegmentRespirationRate::getName() const {
	return "rootSegmentRespirationRate";
}

DerivativeBase * newInstantiationRootSegmentRespirationRate(
		SimulaDynamic* const pSD) {
	return new RootSegmentRespirationRate(pSD);
}

//shoot respiration
LeafRespirationRate::LeafRespirationRate(SimulaDynamic* pSD) :
		DerivativeBase(pSD),
sizeSimulator(NULL), leafSenescenceSimulator(NULL), relativeRespirationSimulator(NULL), factor(NULL)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType, pSD);
	//initiation time
	Time t(pSD->getStartTime());
	//get the root type parameters
	SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));
	//find simulators in database
	relativeRespirationSimulator = parameters->existingChild(
			"relativeRespirationRateLeafs", t);
	if (relativeRespirationSimulator) {
		if (relativeRespirationSimulator->getUnit() == "g/g/day") {
			sizeSimulator = pSD->getSibling(
					"leafDryWeight");
			leafSenescenceSimulator = pSD->existingSibling("senescedLeafDryWeight");
		} else if (relativeRespirationSimulator->getUnit() == "g/cm2/day") {
			sizeSimulator = pSD->getSibling("LeafArea");
			leafSenescenceSimulator = pSD->existingSibling("senescedLeafArea");
		} else {
			msg::error(
					"LeafRespirationRate: relativeRespirationRateLeafs should be in either g/g/day (based on shoot dryweight) or in g/cm2/day (based on shoot leaf area)");
		}
	} else {
		msg::warning(
				"LeafRespirationRate: relativeRespirationRateLeafs parameter missing, setting respiration to 0.");
	}
	//check for nutrients effects on respiration
	factor = pSD;
	PLANTTOP(factor);
	factor = factor->existingChild(
			"stressFactor:impactOn:leafRespiration");
}
void LeafRespirationRate::calculate(const Time &t, double &rate) {
	//return 0 if repiration parameters are not set
	if (!relativeRespirationSimulator) {
		rate = 0;
		return;
	}

	//shoot size in cm2 leaf area or g dryweight
	sizeSimulator->get(t, rate);
	if(leafSenescenceSimulator){
		double s;
		leafSenescenceSimulator->get(t,s);
		rate-=s;
	}

	//current respiration rate in g CO2 /g/day or g CO2 /cm2/day
	double res;
	relativeRespirationSimulator->get(t, res);

	//current shoot respiration rate in g CO2/day
	rate *= res;

	//multiplication factor
	if (factor) {
		factor->get(t, res);
		rate *= res;
	}
}

std::string LeafRespirationRate::getName() const {
	return "leafRespirationRate";
}
DerivativeBase * newInstantiationLeafRespirationRate(SimulaDynamic* const pSD) {
	return new LeafRespirationRate(pSD);
}

//shoot respiration
StemRespirationRate::StemRespirationRate(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plant type
	std::string plantType;
	PLANTTYPE(plantType, pSD);
	//get the root type parameters
	SimulaBase *parameters(GETSHOOTPARAMETERS(plantType));
	//find simulators in database
	relativeRespirationSimulator = parameters->existingChild(
			"relativeRespirationRateStems", "g/g/day");
	if (relativeRespirationSimulator) {
		sizeSimulator = pSD->getSibling("stemDryWeight");
	} else {
		msg::warning(
				"StemRespirationRate: relativeRespirationRateStems parameter missing, setting respiration to 0.");
	}
	//check for nutrients effects on respiration
	factor = pSD;
	PLANTTOP(factor);
	factor = factor->existingChild(
			"stressFactor:impactOn:stemRespiration");
}
void StemRespirationRate::calculate(const Time &t, double &rate) {
	//return 0 if repiration parameters are not set
	if (!relativeRespirationSimulator) {
		rate = 0;
		return;
	}

	//shoot size in cm2 leaf area or g dryweight
	sizeSimulator->get(t, rate);

	//current respiration rate in g CO2 /g/day or g CO2 /cm2/day
	double res;
	relativeRespirationSimulator->get(t, res);

	//current shoot respiration rate in g CO2/day
	rate *= res;

	//multiplication factor
	if (factor) {
		factor->get(t, res);
		rate *= res;
	}
}
std::string StemRespirationRate::getName() const {
	return "stemRespirationRate";
}

DerivativeBase * newInstantiationStemRespirationRate(SimulaDynamic* const pSD) {
	return new StemRespirationRate(pSD);
}

//Register the module
class AutoRegisterRespirationInstantiationFunctions {
public:
	AutoRegisterRespirationInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentRespirationRate"] =
				newInstantiationRootSegmentRespirationRate;
		BaseClassesMap::getDerivativeBaseClasses()["leafRespirationRate"] =
				newInstantiationLeafRespirationRate;
		BaseClassesMap::getDerivativeBaseClasses()["stemRespirationRate"] =
				newInstantiationStemRespirationRate;
	}
};

// our one instance of the proxy
static AutoRegisterRespirationInstantiationFunctions p;

