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
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "TillerFormation.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaConstant.hpp"
#include "../../engine/SimulaStochastic.hpp"
#include "../PlantType.hpp"
#include <math.h>
#include <algorithm>

//create tillers
TillerFormation::TillerFormation(SimulaBase* const pSB) :
	ObjectGenerator(pSB), tillerStress(nullptr), tillerOrderStress(nullptr), gp(nullptr), maxNumberOfTillers(6), maxTillerOrder(1),
	tillerPosDegree(0), timingOfLastTillerOrder(0), newPosition(0, 0, 0), pot(false), readTillerPositionsFromFile(false), relativeTillerPositions(false), timeDelayBetweenTillerOrders(nullptr) {
}
TillerFormation::~TillerFormation() {
}
void TillerFormation::initialize(const Time &t) {
	auto found=pSB->getName().find("potential");
	if(found!=std::string::npos) pot=true;
	//top of plant
	SimulaBase *top(pSB);
	PLANTTOP(top);
	//plant type
	std::string plantType;
	top->getChild("plantType")->get(plantType);
	// Pointer to root parameters
	SimulaBase *tillerPar(ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot")->existingChild("tillers"));
	if (!tillerPar){
		msg::warning("TillerFormation: Using outdated tiller parameter format. Consider updating to the new standard.");
		tillerPar = ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot");
	}
	// Number of tiller orders
	if (tillerPar->existingChild("maxTillerOrder")){
		tillerPar->getChild("maxTillerOrder")->get(maxTillerOrder);
	}
	// Vector with pointers to the object calculating the time delay between tillers, one for each tiller order
	for (int i = 0; i < maxTillerOrder; i++){
		if (tillerPar->existingChild("potentialTimeDelayBetweenTillers")){
			timingOfTillers.push_back(tillerPar->getChild("potentialTimeDelayBetweenTillers"));
			if (pSB->getParent(3)->existingChild("stressFactor:impactOn:tillerFormation")){
				if (!pot) tillerStress = pSB->getParent(3)->existingChild("stressFactor:impactOn:tillerFormation");
			} else msg::warning("TillerFormation: potentialTimeDelayBetweenTillers1 used but stress factor not present");
		}
		else if (tillerPar->existingChild("potentialTimeDelayBetweenTillers1")){
			timingOfTillers.push_back(tillerPar->getChild("potentialTimeDelayBetweenTillers" + std::to_string(i+1)));
			if (pSB->getParent(3)->existingChild("stressFactor:impactOn:tillerFormation")){
				if (!pot) tillerStress = pSB->getParent(3)->existingChild("stressFactor:impactOn:tillerFormation");
			} else msg::warning("TillerFormation: potentialTimeDelayBetweenTillers1 used but stress factor not present");
		} else msg::warning("TillerFormation: potentialTimeDelayBetweenTillers not found, using timeDelayBetweenTillers");
	}
	if (timingOfTillers.size() == 0){
		for (int i = 0; i < maxTillerOrder; i++){
			if (tillerPar->existingChild("timeDelayBetweenTillers")){
				timingOfTillers.push_back(tillerPar->getChild("timeDelayBetweenTillers"));
			} else if (tillerPar->existingChild("timeDelayBetweenTillers1")){
				timingOfTillers.push_back(tillerPar->getChild("timeDelayBetweenTillers" + std::to_string(i+1)));
			} else msg::error("TillerFormation: timeDelayBetweenTillers not found.");
		}
	}
	double firstTillerTime;
	timingOfTillers[0]->get(0, firstTillerTime);
	// Time between tiller orders
	if (tillerPar->existingChild("potentialTimeDelayBetweenTillerOrders")){
		timeDelayBetweenTillerOrders = tillerPar->getChild("potentialTimeDelayBetweenTillerOrders");
		if (pSB->getParent(3)->existingChild("stressFactor:impactOn:tillerFormation")){
			if (!pot) tillerOrderStress = pSB->getParent(3)->existingChild("stressFactor:impactOn:tillerFormation");
		} else 	msg::warning("TillerFormation: potentialTimeDelayBetweenTillerOrders used but stress factor not present");
	}
	if (!timeDelayBetweenTillerOrders){
		if (tillerPar->existingChild("timeDelayBetweenTillerOrders")){
			timeDelayBetweenTillerOrders = tillerPar->getChild("timeDelayBetweenTillerOrders");
		}
	}
	// Time of first tiller emergence
	if (tillerPar->existingChild("firstTillerTime")){
		tillerPar->getChild("firstTillerTime")->get(firstTillerTime);
	}
	// Number of tillers per order
	for (int i = 0; i < maxTillerOrder; i++){
		int temp;
		if (tillerPar->existingChild("numberOfTillers")){
			tillerPar->getChild("numberOfTillers")->get(temp);
			maxTillerNumber.push_back(temp);
		}
		else if (tillerPar->existingChild("numberOfTillers1")){
			tillerPar->getChild("numberOfTillers" + std::to_string(i+1))->get(temp);
			maxTillerNumber.push_back(temp);
		} else{
			msg::warning("TillerFormation: number of tillers per order not specified, defaulting to 12");
			maxTillerNumber.push_back(12);
		}
	}
	maxNumberOfTillers = *std::max_element(maxTillerNumber.begin(), maxTillerNumber.end());
	// Vertical offset of tillers, w.r.t. the seed, one for each tiller order
	if (tillerPar->existingChild("verticalOffset")){
		double temp;
		tillerPar->getChild("verticalOffset")->get(temp);
		for (int i = 0; i < maxTillerOrder; i++){
			verticalOffsets.push_back(temp);
		}
	} else if (tillerPar->existingChild("verticalOffset1")){
		for (int i = 0; i < maxTillerOrder; i++){
			double temp;
			tillerPar->getChild("verticalOffset" + std::to_string(i+1))->get(temp);
			verticalOffsets.push_back(temp);
		}
	} else {
		msg::warning("TillerFormation: No vertical offset found, placing tiller location at ground surface");
		Coordinate tempCoord;
		pSB->getAbsolute(0, tempCoord);
		double temp = std::abs(tempCoord.y);
		for (int i = 0; i < maxTillerOrder; i++){
			verticalOffsets.push_back(temp);
		}
	}
	// If true, tillers of higher orders emerge in a random positions chosen from the circle with radius crowndiameter around the corresponding tiller of the previous order
	// If false, tillers of higher orders emerge in the same direction relative to the seed as those in the first tiller order (but at different distance perhaps)
	if (tillerPar->existingChild("relativeTillerPositions")) tillerPar->getChild("relativeTillerPositions")->get(relativeTillerPositions);
	// Can be used to explicitly specify the tiller positions in the XML
	if (tillerPar->existingChild("tillerPositions1")){
		readTillerPositionsFromFile = true;
		for (int i = 0; i < maxTillerOrder; i++){
			tillerPositions.push_back(tillerPar->getChild("tillerPositions" + std::to_string(i+1)));
		}
	}
	for(int i = 0; i < maxTillerOrder; i++){
		tillernumber.push_back(0);
	}
	// Root types to be used for each tiller order
	if (!tillerPar->existingChild("tillerRootType1")) msg::warning("TillerFormation: Tiller root type 1 not defined, defaulting to nodalRootsOfTillers or nodalrootsOfTillers.");
	for (int i = 1; i < maxTillerOrder; i++){
		if(!tillerPar->existingChild("tillerRootType" + std::to_string(i+1))) msg::error("tillerRootType" + std::to_string(i+1) + " not found in rootTypeParameters/" + plantType + "/shoot. Did you forget to add it?");
	}
	newPosition.x = 0;
	newPosition.y = 0;
	newPosition.z = 0;
	gp=dynamic_cast<SimulaPoint*>	( top->getChild("plantPosition")->getChild("hypocotyl")->getChild("growthpoint"));
	// Determining tiller emergence order
	if (!tillerPar->existingChild("tillerPositions1")){
		std::vector< std::vector<double> > temp(maxTillerOrder, std::vector<double>(maxNumberOfTillers));
		tillerPosX = temp;
		tillerPosZ = temp;

		for(int i = 0; i < maxNumberOfTillers; i++){
			tillerOrdering.push_back(i);
		}
		// Randomising tiller emergence order
		std::shuffle(tillerOrdering.begin(), tillerOrdering.end(), generator);
		double tempTime = pSB->getStartTime() + firstTillerTime;
		if (tillerOrderStress && timeDelayBetweenTillerOrders){
			timingOfLastTiller.push_back(tempTime);
			timingOfLastTillerOrder = tempTime;
		} else{
			for(int i = 0; i < maxTillerOrder; i++){
				if (!timeDelayBetweenTillerOrders){
					timingOfLastTiller.push_back(pSB->getStartTime() + firstTillerTime + i*5.0);
					msg::warning("TillerFormation: Time between tiller orders not set. Defaulting to 5 days.");
				}
				else{
					timingOfLastTiller.push_back(tempTime);
					double timeDelay;
					timeDelayBetweenTillerOrders->get(tempTime, timeDelay);
					tempTime += timeDelay;
				}
			}
		}
		// Stting pointers to tiller crown diameter
		if (tillerPar->existingChild("crownDiameter1")){
			for (int i = 0; i < maxTillerOrder; i++){
				crownDiameters.push_back(tillerPar->getChild("crownDiameter" + std::to_string(i + 1), "cm"));
			}
		}
		else{
			for (int i = 0; i < maxTillerOrder; i++){
				crownDiameters.push_back(tillerPar->getChild("crownDiameter", "cm"));
			}
		}
	}
}

void TillerFormation::generate(const Time &t) {
	// If tiller positions are defined in the input file, read them out
	if (readTillerPositionsFromFile){
		for (std::size_t i = 0; i < tillerPositions.size(); i++){
			for (int j = tillernumber[i]; j < tillerPositions[i]->getNumberOfChildren(); j++){
				Time tillerTime = tillerPositions[i]->getChild(std::to_string(j+1))->getStartTime();
				if (t + 1 >= tillerTime){
					Coordinate temp;
					tillerPositions[i]->getChild(std::to_string(j+1))->get(temp);
					tillernumber[i] += 1;
					std::string nameNewTiller("tiller" + std::to_string(i+1) + '_' + std::to_string(tillernumber[i]));
					SimulaConstant<Coordinate>* pNewBranch = new SimulaConstant<Coordinate>(nameNewTiller, pSB, temp, tillerTime);
					if(!pot) pNewBranch->setFunctionPointer("tillerDevelopment");
				}
			}
		}
	}
	else{
		for(int i = 0; i < maxTillerOrder; i++){
			// Check if new tiller order will emerge yet
			if (timingOfLastTiller.size() == (unsigned int) i){
				double s, timeDelay;
				tillerOrderStress->get(t,s);
				timeDelayBetweenTillerOrders->get(t, timeDelay);
				if(s<1e-3) s=1e-3;
				timeDelay/=s;
				if (timingOfLastTillerOrder + timeDelay - t < 3){
					timingOfLastTillerOrder = std::max(timingOfLastTillerOrder + timeDelay, t + 1);
					timingOfLastTiller.push_back(timingOfLastTillerOrder);
				}
			}
			if (timingOfLastTiller.size() > (unsigned int) i){
				while (t > timingOfLastTiller[i]-TIMEERROR && tillernumber[i] < maxTillerNumber[i]) {
					crownDiameters[i]->get(t - pSB->getStartTime(), currentDiameter);
					currentDiameter /= 2;
					if (!relativeTillerPositions || i == 0){
						tillerPosX[i][tillernumber[i]] = currentDiameter*cos(tillerOrdering[tillernumber[i]]*2.*M_PI/maxNumberOfTillers);
						tillerPosZ[i][tillernumber[i]] = currentDiameter*sin(tillerOrdering[tillernumber[i]]*2.*M_PI/maxNumberOfTillers);
					}
					if (relativeTillerPositions && i > 0){
						tillerPosX[i][tillernumber[i]] = currentDiameter*cos(2.*M_PI*(double)rand()/(double)RAND_MAX) + tillerPosX[i-1][tillernumber[i]];
						tillerPosZ[i][tillernumber[i]] = currentDiameter*sin(2.*M_PI*(double)rand()/(double)RAND_MAX) + tillerPosZ[i-1][tillernumber[i]];
					}
					Coordinate shift;
					shift.x = tillerPosX[i][tillernumber[i]];
					shift.z = tillerPosZ[i][tillernumber[i]];
					shift.y = -verticalOffsets[i];
					Coordinate newPositionRoot(newPosition-shift);
					tillernumber[i] += 1;
					if (tillernumber[i] > 100)
						msg::error("TillerFormation: more than 100 tillers generated. Check your tillering rules");
					std::string nameNewTiller("tiller" + std::to_string(i+1) + '_' + std::to_string(tillernumber[i]));
					//create the tiller
					//create new branch by copying it from the template
					SimulaConstant<Coordinate>* pNewBranch = new SimulaConstant<Coordinate>(nameNewTiller, pSB, newPositionRoot, timingOfLastTiller[i]);
					if(!pot) pNewBranch->setFunctionPointer("tillerDevelopment");
					//check for next time
					double delay;
					//timingOfTillers->get(timingOfLastTiller[i] - pSB->getStartTime(), delay);
					timingOfTillers[i]->get(t, delay);
					if(tillerStress){ //todo this is numerically and physiologically probably not ideal during recovery from stress.
						double s;
						tillerStress->get(timingOfLastTiller[i],s);
						if(s<1e-3) s=1e-3;
						delay/=s;
					}
					timingOfLastTiller[i] += delay;
				}
			}
		}
	}
}

//the maker function for instantiation of the class
ObjectGenerator * newInstantiationTillerFormation(SimulaBase* const pSB) {
	return new TillerFormation(pSB);
}

//have tillers form nodal roots etc.
TillerDevelopment::TillerDevelopment(SimulaBase* const pSB) :
		ObjectGenerator(pSB), timingOfNodalRoots(nullptr), hypo(nullptr), timingOfLastNodalRoot(pSB->getStartTime()), nodalRootNumber(0), newPosition(0, 0, 0) {
}
TillerDevelopment::~TillerDevelopment() {
}
void TillerDevelopment::initialize(const Time &t) {
	//top of plant
	SimulaBase *top(pSB);
	PLANTTOP(top);
	//plant type
	top->getChild("plantType")->get(plantType);
	//pointer to root parameters of the parent
	SimulaBase *tillerPar(ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot")->existingChild("tillers"));
	if (!tillerPar) tillerPar = ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("shoot");
	//hypocotyl
	hypo = top->getChild("plantPosition")->getChild("hypocotyl");

	std::string temp = "tillerRootType" + pSB->getName().substr(6,pSB->getName().find('_')-6);
	if (tillerPar->existingChild(temp)) tillerPar->getChild(temp)->get(tillerRootType);
	else {
        if (ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->existingChild("nodalrootsOfTillers")){
			tillerRootType = "nodalrootsOfTillers";
        }
        else tillerRootType = "nodalRootsOfTillers";
	}


	//pointer to root parameters of the parent
	if (!ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("hypocotyl")->getChild("branchList")->existingChild(tillerRootType)) msg::error("No branching rules can be found in rootTypeParameters/" + plantType + "/hypocotyl/branchList for the root type " + tillerRootType + " that is defined as a tiller root. Did you forget to add parameters for this root class?");
	SimulaBase *branchingPar(ORIGIN->getChild("rootTypeParameters")->getChild(plantType)->getChild("hypocotyl")->getChild("branchList")->getChild(tillerRootType));
	timingOfNodalRoots = branchingPar->getChild("branchingDelay");
	if (tillerPar->existingChild("timeBetweenTillerAndFirstTillerRoot")){
		double temp;
		tillerPar->getChild("timeBetweenTillerAndFirstTillerRoot")->get(temp);
		timingOfLastNodalRoot += temp;
	}

	//tiller template
	SimulaBase *tillerTemplate(	ORIGIN->existingChild("tillerTemplate"));
	if(tillerTemplate){
		pSB->copyAttributes(pSB->getStartTime(), tillerTemplate);
	}
	container = hypo->getChild("branches")->getChild(tillerRootType);
}

void TillerDevelopment::generate(const Time &t) {
	//generate roots on the tillers
	pSB->getAbsolute(pSB->getStartTime(), newPosition);
	Coordinate refPosition;
	container->getAbsolute(pSB->getStartTime(),refPosition);//sowing position (base of hypocotyl)
	newPosition-=refPosition;

	while (t > timingOfLastNodalRoot - TIMEERROR - 1 && nodalRootNumber == 0){
		timingOfLastNodalRoot += 1;

		//name
		nodalRootNumber += 1;
		std::string nameNewNodalRoot(pSB->getName() + ".nodal" + std::to_string(nodalRootNumber));

		//create the root
		SimulaConstant<Coordinate>* pNewBranch = new SimulaConstant<Coordinate>(nameNewNodalRoot, container, newPosition, timingOfLastNodalRoot - 1);
		pNewBranch->setFunctionPointer("generateRoot");

		//insert rootType attribute of new branch
		new SimulaConstant<std::string>("rootType", pNewBranch, tillerRootType, UnitRegistry::noUnit(), timingOfLastNodalRoot - 1);
	}

	while (t > timingOfLastNodalRoot - TIMEERROR - 1) {
		double delay;
		timingOfNodalRoots->get(timingOfLastNodalRoot - pSB->getStartTime(), delay);
		timingOfLastNodalRoot += delay;

		//name
		nodalRootNumber += 1;
		if (nodalRootNumber > 100)
			msg::error("TillerDevelopment: more than 100 nodalroots generated. Check your tillering rules");
		std::string nameNewNodalRoot(pSB->getName() + ".nodal" + std::to_string(nodalRootNumber));

		//create the root
		SimulaConstant<Coordinate>* pNewBranch = new SimulaConstant<Coordinate>(nameNewNodalRoot, container, newPosition, timingOfLastNodalRoot - 1);
		pNewBranch->setFunctionPointer("generateRoot");

		//insert rootType attribute of new branch
		new SimulaConstant<std::string>("rootType", pNewBranch, tillerRootType, UnitRegistry::noUnit(), timingOfLastNodalRoot - 1);
	}
	pSB->updateRecursively(t);

}

//the maker function for instantiation of the class
ObjectGenerator * newInstantiationTillerDevelopment(SimulaBase* const pSB) {
	return new TillerDevelopment(pSB);
}



//--------------------------------------------------------------------------------------------------------

NumberOfTillers::NumberOfTillers(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	auto found=pSD->getName().find("potential");
	if(found!=std::string::npos){
		ref=pSD->getSibling("potentialTillers");
	}else{
		ref=pSD->getSibling("tillers");
	}

}
std::string NumberOfTillers::getName() const {
	return "numberOfTillers";
}

void NumberOfTillers::calculate(const Time &t, int &ci) {
	SimulaBase::List list;
	ref->getAllChildren(list,t);
	ci=(int)list.size();
}
void NumberOfTillers::calculate(const Time &t, double &ci) {
	SimulaBase::List list;
	ref->getAllChildren(list,t);//todo implement a get size
	ci=(double)list.size();
}
DerivativeBase * newInstantiationNumberOfTillers(SimulaDynamic* const pSD) {
	return new NumberOfTillers(pSD);
}





NumberOfRoots::NumberOfRoots(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	std::string rootClass=pSD->getName().substr(8,std::string::npos);
	ref=pSD->getSibling("plantPosition")->getChild("hypocotyl")->getChild("branches")->existingChild(rootClass);
}
std::string NumberOfRoots::getName() const {
	return "numberOfRoots";
}

void NumberOfRoots::calculate(const Time &t, int &ci) {
	if(ref){
		SimulaBase::List list;
		ref->getAllChildren(list,t);
		ci=(int)list.size();
	}else{
		ci=0;
	}
}
void NumberOfRoots::calculate(const Time &t, double &ci) {
	if(ref){
		SimulaBase::List list;
		ref->getAllChildren(list,t);
		ci=(double)list.size();
	}else{
		ci=0.;
	}
}
DerivativeBase * newInstantiationNumberOfRoots(SimulaDynamic* const pSD) {
	return new NumberOfRoots(pSD);
}

class AutoRegisterTillerFormationInstantiationFunctions {
public:
	AutoRegisterTillerFormationInstantiationFunctions() {
		// register the maker with the factory
		BaseClassesMap::getObjectGeneratorClasses()["tillerFormation"] = newInstantiationTillerFormation;
		BaseClassesMap::getObjectGeneratorClasses()["tillerDevelopment"] = newInstantiationTillerDevelopment;
		BaseClassesMap::getDerivativeBaseClasses()["numberOfTillers"] = newInstantiationNumberOfTillers;
		BaseClassesMap::getDerivativeBaseClasses()["numberOfRoots"] = newInstantiationNumberOfRoots;
	}
};

static AutoRegisterTillerFormationInstantiationFunctions p679kik438382;




