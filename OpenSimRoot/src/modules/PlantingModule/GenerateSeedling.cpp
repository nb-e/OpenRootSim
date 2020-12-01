/*
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
#include "GenerateSeedling.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaConstant.hpp"
#include "../PlantType.hpp"
#include <math.h>

Seedling::Seedling(SimulaBase* const pSB) :
		ObjectGenerator(pSB) {
}
void Seedling::initialize(const Time &t) {
	//plant type
	std::string plantType;
	pSB->getChild("plantType")->get(t, plantType);

	//time of generation of seedling
	Time plantingTime;
	if (pSB->existingChild("plantingTime")) {
		//read planting time from file
		pSB->getChild("plantingTime")->get(t, plantingTime);
	} else {
		//copy from parent
		plantingTime = pSB->getStartTime();
	}

	//insert whole plant parameters from template
	pSB->copyAttributes(plantingTime, ORIGIN->getChild("plantTemplate"));

	//pointer to plant position object
	SimulaBase* plantposition(pSB->getChild("plantPosition"));

	//generate shoot
	if (ORIGIN->existingChild("shootTemplate")
			&& !plantposition->existingChild("shoot")) {
		SimulaBase* pNewShoot = new SimulaConstant<Coordinate>("shoot",
				plantposition, Coordinate(0, 0, 0), plantingTime);
		//copy standard root children from the root template
		pNewShoot->copyAttributes(plantingTime,
				ORIGIN->getChild("shootTemplate"));
	}

	//generate hypocotyl
	if (ORIGIN->existingChild("hypocotylTemplate")
			&& !plantposition->existingChild("hypocotyl")) {
		SimulaBase* pNewHypocotyl = new SimulaConstant<Coordinate>("hypocotyl",
				plantposition, Coordinate(0, 0, 0), plantingTime);
		pNewHypocotyl->setFunctionPointer("generateRoot");
		new SimulaConstant<std::string>("rootType", pNewHypocotyl, "hypocotyl",
				UnitRegistry::noUnit(), plantingTime);
	}

	//generate root
	if (ORIGIN->existingChild("siblingRootTemplate")
			&& !plantposition->existingChild("primaryRoot")) {
		SimulaBase* pNewRoot = new SimulaConstant<Coordinate>("primaryRoot",
				plantposition, Coordinate(0, 0, 0), plantingTime);
		pNewRoot->setFunctionPointer("generateRoot");
		new SimulaConstant<std::string>("rootType", pNewRoot, "primaryRoot",
				UnitRegistry::noUnit(), plantingTime);
	}

	//stop the update function after initialization.
	pSB->stopUpdatefunction();
	pSB->updateRecursively(t);

}
void Seedling::generate(const Time &t) {
}

//the maker function for instantiation of the class
ObjectGenerator * newInstantiationSeedling(SimulaBase* const pSB) {
	return new Seedling(pSB);
}

FieldPlanting::FieldPlanting(SimulaBase* const pSB) :
		ObjectGenerator(pSB) {

}
void FieldPlanting::initialize(const Time &t) {
	//parameters
	double prs, irs;
	SimulaBase* p = ORIGIN->getChild("environment")->getChild("plantingScheme");
	p->getChild("rowSpacing", "cm")->get(prs);
	p->getChild("inTheRowSpacing", "cm")->get(irs);

	double plantingDepth, plantingTime, pd(0);
	int nopir(1), nor(1);
	std::string plantType;
	p->getChild("plantingDepth", "cm")->get(plantingDepth);
	p->getChild("plantingTime")->get(plantingTime);
	p->getChild("plantType")->get(plantType);
	SimulaBase* pp = p->existingChild("numberOfPlantsInTheRow");
	if (pp)
		pp->get(nopir);
	pp = p->existingChild("numberOfRows");
	if (pp)
		pp->get(nor);
	pp = p->existingChild("profileDepth");
	if (pp)
		pp->get(pd);

	std::string dir;
	p->getChild("rowDirection")->get(dir);
	if (dir == "x") {
		std::swap(irs, prs);
		std::swap(nopir, nor);
	} else {
		if (dir != "z")
			msg::warning(
					"FieldPlanting: ignoring unknown rowDirection. Use x of z. Assuming z");
	}

	pp = GETSHOOTPARAMETERS(plantType)->existingChild("areaPerPlant");
	if (pp) {
		double app;
		pp->get(app);
		if (fabs(app - (prs * irs)) > 0.01) {
			msg::warning(
					"FieldPlanting: adjusting area per plant from "
							+ std::to_string(app) + " to "
							+ std::to_string(prs * irs));
			SimulaConstant<double>* ppp =
					dynamic_cast<SimulaConstant<double>*>(pp);
			ppp->setConstant(prs * irs);
		}
	} else {
		new SimulaConstant<Time>("areaPerPlant", p, prs * irs, "cm2",
				p->getStartTime());
	}

	//midpoint and edges of the the field
	double	rlen(prs*(double)nor),	irlen(irs*(double)nopir);
	Coordinate cmin(-rlen/2., pd, -irlen/2.),
			   cmax( rlen/2., 0.,  irlen/2.);
	p = p->getSibling("dimensions");
	pp = p->existingChild("minCorner", "cm");
	if (pp) {
		pp->get(cmin);
	} else {
		msg::warning(
				"FieldPlanting: setting minCorner to "
						+ convertToString<Coordinate>(cmin));
		new SimulaConstant<Coordinate>("minCorner", p, cmin, p->getStartTime());
	}
	pp = p->existingChild("maxCorner", "cm");
	if (pp) {
		pp->get(cmax);
	} else {
		msg::warning(
				"FieldPlanting: setting maxCorner to "
						+ convertToString<Coordinate>(cmax));
		new SimulaConstant<Coordinate>("maxCorner", p, cmax, p->getStartTime());
	}
	Coordinate cori((cmax + cmin) / 2.);
	if (fmod((cmax.x - cmin.x), prs) > 0.001
			|| fmod((cmax.z - cmin.z), irs) > 0.1) {
		msg::warning(
				"FieldPlanting: soil dimensions are not a multiple of the plant spacing.");
	}

	//shift so we have a plant in the middle
	cmin.x += prs/2.;
	cmin.z += irs/2.;

	//planting depth and time
	unsigned int rcount(0);
	for (double x(cmin.x); x < cmax.x; x += prs) {
		++rcount;
		unsigned int count(0);
		for (double z(cmin.z); z < cmax.z; z += irs) {
			count++;
			p = new SimulaBase(
					plantType + "_" + std::to_string(rcount) + "_"
							+ std::to_string(count), pSB,
					UnitRegistry::noUnit(), plantingTime);
			//seedling
			p->setFunctionPointer("seedling");
			//insert position, type and time
			new SimulaConstant<Time>("plantingTime", p, plantingTime, "day",
					p->getStartTime());
			new SimulaConstant<std::string>("plantType", p, plantType,
					UnitRegistry::noUnit(), p->getStartTime());
			new SimulaConstant<Coordinate>("plantPosition", p,
					Coordinate(x, plantingDepth, z), p->getStartTime());

		}
	}

	pSB->stopUpdatefunction();
}
void FieldPlanting::generate(const Time &t) {

}
ObjectGenerator * newInstantiationFieldPlanting(SimulaBase* const pSB) {
	return new FieldPlanting(pSB);
}

//register the classes
class AutoRegisterGenerateSeedlingInstantiationFunctions {
public:
	AutoRegisterGenerateSeedlingInstantiationFunctions() {
		BaseClassesMap::getObjectGeneratorClasses()["seedling"] =
				newInstantiationSeedling;
		BaseClassesMap::getObjectGeneratorClasses()["fieldPlanting"] =
				newInstantiationFieldPlanting;
	}
};

static AutoRegisterGenerateSeedlingInstantiationFunctions p4595582386;
