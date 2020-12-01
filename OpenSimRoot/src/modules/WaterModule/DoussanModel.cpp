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
#include "DoussanModel.hpp"

#include "../PlantType.hpp"

#include "../../math/pcgSolve.hpp"

LateralHydraulicConductivity::LateralHydraulicConductivity(SimulaDynamic* pSD) :
		DerivativeBase(pSD), inverse(false) {
	//get name
	std::string name(pSD->getName());
	name.erase(0, 11); //erase the segment word
	name[0] = tolower(name[0]);
	cond = pSD->getSibling(name);

	//inverse
	if (name == "lateralHydraulicConductivity") {
		inverse = true; //serial system
	} else if (name == "radialHydraulicConductivity") {
		inverse = false; //parallel system
	} else {
		msg::error("HydraulicConductivity: unkown type " + name);
	}

	Unit u = cond->getUnit();
	if (u == "cm/day/hPa") {
		//surface area based
		size = pSD->getSibling("rootSegmentSurfaceArea");
	} else if (u == "cm4/day/hPa") {
		//length based
		size = pSD->getSibling("rootSegmentLength");
	} else {
		msg::error(
				"LateralHydraulicConductivity: unknown unit for "
						+ cond->getName() + " use cm4/day/hpa or cm/day/hpa");
	}

	//lateral conductivity according to Hagen–Poiseuille equation
	//is (pi r^4)/(8u l) where r is radius xylem vessel, l the length and u the dynamic viscosity of water (8.90 × 10−4 Pa·s)
	//units cm3/Pa/s
	if (!inverse) {
		//plant type
		std::string plantType;
		PLANTTYPE(plantType, pSD);
		//get the root type parameters
		std::string rootType;
		pSD->getParent(3)->getChild("rootType")->get(rootType);
		SimulaBase *parameters(GETROOTPARAMETERS(plantType, rootType));
		RCSeffect =
				parameters->existingChild(
						"reductionInRadialHydraulicConductivityDueToCorticalSenescence");
		if (RCSeffect) {
			RCSstage = pSD->existingSibling("rootCorticalSenescenceStage");
			if (RCSstage) {
				msg::warning(
						"HydraulicConductivity: simulating effects of RCS on radial hydraulic conductivity");
			} else {
				msg::warning(
						"HydraulicConductivity: NOT simulating effects of RCS on radial hydraulic conductivity");
				RCSeffect = nullptr;
			}
		}
	} else {
		RCSeffect = nullptr;
	}

}
std::string LateralHydraulicConductivity::getName() const {
	return "hydraulicConductivity";
}
void LateralHydraulicConductivity::calculate(const Time &t, double &c) {
	double s;
	size->get(t, s);
	cond->get(t, c);
	if (inverse) {
		//by length, the longer the root the lower the lateral con
		if (s > 1e-9) {
			c /= s;
		} else {
			c /= 1e-9;
		}
	} else {
		c *= s;
	}

	if (RCSeffect) {
		double e;
		RCSstage->get(t, s);
		RCSeffect->get(s, e);
		c *= (1 - e);
	}
}

DerivativeBase * newInstantiationLateralHydraulicConductivity(
		SimulaDynamic* const pSD) {
	return new LateralHydraulicConductivity(pSD);
}

WaterUptakeDoussanModel::WaterUptakeDoussanModel(SimulaDynamic* pSD) :
		DerivativeBase(pSD), wuList(), KhList(), LrList(), psi_sList(), ni(), neighborMap(), psi_s(), Lr(), b(), psi_x(), C(), psiCollar(
				-100), collarnodeIndex(-1), largeKh(15), nodeCounter(0), useGravity(false) {

	SimulaBase::signalMeAboutNewObjects(pSD);
	pSD->getSibling("plantType")->get(plantType_);
	potentialTranspiration =
			pSD->getSibling("plantPosition")->getChild("shoot")->existingChild(
					"potentialTranspiration", "cm3");
	if (!potentialTranspiration) {
		msg::warning(
				"WaterUptakeDoussanModel: potentialTranspiration not found, assuming predefined collar potential");
	}

	SimulaBase::Positions l;
	SimulaBase::getAllPositions(l);
	int count = 0;
	for (auto it(l.begin()); it != l.end(); ++it) {
		addObject(it->second);
		++count;
	}

	auto p = pSD->existingSibling("waterPotentialAtTheCollar", "hPa");
	if (p) {
		waterPotentialAtTheCollar = dynamic_cast<SimulaTable<double>*>(p);
		if (waterPotentialAtTheCollar && potentialTranspiration)
			waterPotentialAtTheCollar->set(pSD->getStartTime(), 0);
	} else {
		if (!potentialTranspiration)
			msg::error(
					"WaterUptakeDoussan: neither potentialTranspiration nor waterPotentialAtTheCollar given.");
	}
}

std::string WaterUptakeDoussanModel::getName() const {
	return "waterUptakeDoussanModel";
}
void WaterUptakeDoussanModel::addObject(SimulaBase *rootNode) {
	//this is called each time a new root node is inserted.
	//check that this is a root node, not growth point or something else.
	std::string name = rootNode->getName();
	if (name.substr(0, 9) != "dataPoint") {
		msg::warning("WaterUptakeDoussanModel: Ignoring node with name " + name,
				3);
		return;
	}

	//check if the root node belongs to the same plant
	std::string plantType;
	PLANTTYPE(plantType, rootNode);
	if (plantType != plantType_)
		return;

	neighborMap[rootNode] = wuList.size();

	//store the node info in a structure
	SimulaTable<double>* pwu =
			dynamic_cast<SimulaTable<double>*>(rootNode->getChild(
					"rootSegmentWaterUptakeRate"));
	if (!pwu)
		msg::error(
				"WaterUptakeDoussanModel: failed to find table with name rootSegmentWaterUptakeRate for storing data");
	wuList.push_back(pwu);

	//set initial value
	pwu->set(pwu->getStartTime(),0.);

}

void WaterUptakeDoussanModel::findConnections() {

	auto mit(wuList.begin()); //note that the neighbormap is sorted, and so changes in order all the time.
	if (nodeCounter)
		std::advance(mit, nodeCounter);

	for (; mit != wuList.end(); ++mit) {

		//Previous node in the graph
		SimulaBase * rootNode = (*mit)->getParent();
		SimulaBase * rootAnker = rootNode->getParent(2);
		SimulaBase * prevRootNode;
		if (rootAnker->getName() == "hypocotyl") {
			//note this returns nullptr at the collar node;
			prevRootNode = rootNode->getNextSibling(); //getParent()->getChildren()->getNext(rootNode->getName());
		} else {
			prevRootNode = rootNode->getPreviousSibling(); //getParent()->getChildren()->getPrevious(rootNode->getName());
			if (!prevRootNode) {
				//this is first node of the root, hook it up to the parent root
				if (rootAnker->getName() == "primaryRoot") {
					//this is a primary root -> connect it to the hypocotyl.
					prevRootNode = rootAnker->getSibling("hypocotyl")->getChild(
							"dataPoints")->getFirstChild();
				} else {
					//this is a branch -> find the nearest datapoint in the parent root.
					SimulaBase::List l;
					rootAnker->getParent(3)->getChild("dataPoints")->getAllChildren(
							l);
					if (l.empty())
						msg::error("WaterUptakeDoussan: can't connect branch");
					double d = 1e10;
					Coordinate pos, npos;
					rootAnker->get(pos); //note that rootNode has position 0 0 0 relative to the root anker
					for (auto it = l.begin(); it != l.end(); ++it) {
						(*it)->get(npos);
						npos -= pos;
						double ld = npos.squareSum();
						if (ld < d) {
							d = ld;
							prevRootNode = (*it);
						}
					}
				}
			}
		}

		//neigbor map
		int neighborIndex = -1; //negative to indicate collar node.
		if (prevRootNode) {
			auto it = neighborMap.find(prevRootNode);
			if (it != neighborMap.end()) {
				neighborIndex = it->second;
			} else {
				msg::error(
						"WaterUptakeDoussanModel::addObject: program bug detected: previous root node not found in neighbor map.");
			}
		} else {
			//collar node has negative number, but should point to this root node and this root node becomes the new collar node.
			for (std::size_t i = 0; i < ni.size(); ++i) {
				if (ni[i] == -1) {
					ni[i] = (int) ni.size();
					break; //found the collar node.
				}
			}
		}
		ni.push_back(neighborIndex);

		//collect node info
		SimulaBase * pLateralConductivity, *pRadialConductivity,
				*pRootSurfaceWaterPotential;
		//pRootSurface = rootNode->getChild("rootSegmentSurfaceArea", "cm2");
		//pRootLength = rootNode->getChild("rootSegmentLength","cm");
		pLateralConductivity = rootNode->getChild(
				"rootSegmentLateralHydraulicConductivity", "cm3/day/hPa");
		pRadialConductivity = rootNode->getChild(
				"rootSegmentRadialHydraulicConductivity", "cm3/day/hPa");
		pRootSurfaceWaterPotential = rootNode->getChild(
				"hydraulicHeadAtRootSurface", "cm");//cm water pressure, same as hPa

		//store the node info in a structure
		SimulaBase*ppxp = rootNode->existingChild("xylemWaterPotential");
		SimulaTable<double>* pxp = nullptr;
		if (ppxp)
			pxp = dynamic_cast<SimulaTable<double>*>(ppxp);

		xpList.push_back(pxp);
		KhList.push_back(pLateralConductivity);
		LrList.push_back(pRadialConductivity);
		psi_sList.push_back(pRootSurfaceWaterPotential);

		//lengthList.push_back(pRootLength);

		//msg::warning("WaterUptakeDoussanModel: Added root node for plant "+plantType_);
	}
	if (xpList.size() != neighborMap.size())
		msg::error("WaterUptakeDoussanModel:: programming error detected");
	nodeCounter = neighborMap.size();

	//write connection tree.
	/*std::cout<<std::endl<<"connections table";
	 for (unsigned int i=0 ; i<nodeCounter ; ++i){
	 auto j=ni[i];
	 if(j>=0){
	 std::cout<<std::endl<<" connecting "<<wuList[i]->getParent(3)->getName()<<"/"<<wuList[i]->getParent()->getName()<<" to "<<wuList[j]->getParent(3)->getName()<<"/"<<wuList[j]->getParent()->getName();
	 double v(0.);
	 //LrList[i]->get(0.,v);
	 //std::cout<<std::endl<<" lr of "<<LrList[i]->getParent(3)->getName()<<"/"<<LrList[i]->getParent()->getName()<<" is "<<v;
	 }else{
	 std::cout<<std::endl<<" connecting "<<wuList[i]->getParent(3)->getName()<<"/"<<wuList[i]->getParent()->getName()<<" to collarnode";
	 }

	 }*/

}
void WaterUptakeDoussanModel::build(const Time &t) {
	//construct sparse matrix C and right hand side b

	std::size_t n(wuList.size());
	b.resize(n);
	C.resize(n);
	Lr.resize(n);
	psi_s.resize(n);
	gravity.resize(n);
	if (psi_x.size() != n) {
		auto cp(psi_x);
		psi_x.resize(n, 0);
		for (std::size_t i = 0; i < cp.size() && i < n; ++i) {
			psi_x[i] = cp[i];
		}
	} else {
		msg::warning("WaterUptakeDoussanModel::build: using same matrix", 3);
	}

	//C and the diagonal for Lr
	gravity=0.;
	for (std::size_t i = 0; i < n; ++i) {
		double lr, p;
		SimulaBase *obj = LrList[i];
		if (obj && obj->evaluateTime(t)) {
			psi_sList[i]->get(t, p);
			if (p < -25000) {
				//root is probably outside grid
				msg::warning(
						"WaterUptakeDoussanModel::build: getting low h(soil), setting lr to 0. Root outside grid?");
				lr = 0.;
				p = -100;
			} else {
				obj->get(t, lr);
			}

			//correct for gravity
			Coordinate pos,npos;
			obj->getAbsolute(t,pos);
			obj->getSibling("rootDiameter")->followChain(t)->getAbsolute(t,npos);
			if(useGravity) gravity[i]=(pos.y+npos.y)/2;
			p+=gravity[i];
		} else {
			lr = 0.;
			p = -100.;
		}
		C.insert(i, i, lr);	//sets the diagonal , so that in next loop the addValueUnsafely can be called on it.
		Lr[i] = lr;
		psi_s[i] = p;
		b[i] = p * lr;
	}

	//C and boundary condition for collar node on the right hand side
	int count = 0;
	std::valarray<double> y(0., n);
	for (std::size_t i = 0; i < n; ++i) {
		auto j = ni[i];
		Coordinate pos;		//,npos;
		if (j < 0) {
			SimulaBase *obj = KhList[i];
			double v;
			if (obj && obj->evaluateTime(t)) {
				obj->get(t, v);
				if (v < 1.e-6)
					v = 1.e-6;
				if (v > largeKh)
					v = largeKh;
				obj->getAbsolute(t, pos);
				y[i] = pos.y;
			} else {
				v = largeKh;
			}

			//collar node
			collarnodeIndex = i;
			collarKh = v;
			b[i] += v * psiCollar; // b(i) = b(i) + K_c * psi_c
			C.addValueUnsafely(i, i, v);
			if (count > 1)
				msg::error(
						"WaterUptakeDoussanModel: Two collar nodes included, check code");
			++count;
		} else {
			SimulaBase *obj = KhList[i];
			double v;
			if (obj && obj->evaluateTime(t)) {
				obj->get(t, v);
				if (v < 1.e-6)
					v = 1.e-6;
				if (v > largeKh)
					v = largeKh;
			} else {
				v = largeKh;
			}
			C.addValueUnsafely(i, i, v);
			C.insert(i, j, -v); //this is twice, given symmetry
			C.addValueUnsafely(j, j, v);
		}
	}
}

void WaterUptakeDoussanModel::writeValues(const Time &t) {
	//set the uptake and xylem water potentials
	for (std::size_t i = 0; i < Lr.size(); ++i) {
		double jf = Lr[i] * (psi_s[i] - psi_x[i]);
		if (wuList[i]->evaluateTime(t)) {
			wuList[i]->set(t, jf);
			if (xpList[i])
				xpList[i]->set(t, psi_x[i]-gravity[i]);
		} else {
			if (jf > 1e-10)
				msg::error(
						"WaterUptakeDoussanModel: uptake from a root segment outside it's lifetime");
		}
	}

	if (waterPotentialAtTheCollar && potentialTranspiration)
		waterPotentialAtTheCollar->set(t, psiCollar);
}

void WaterUptakeDoussanModel::calculate(const Time &t, double &jsum) {
	//get the target transpiration rate. (note this must happen before we run find connections, as it could trigger a update
	double trans(0.);
	if (potentialTranspiration)
		potentialTranspiration->getRate(t, trans);

	//delayed connecting of floating nodes.
	if (nodeCounter < neighborMap.size()) {
		findConnections();
	}

	double smalltrans = 1.e-4; //minimum precision
	if (smalltrans < 0.0001 * trans) smalltrans = 0.0001 * trans;
	double pcgprecision=smalltrans*1e-8;

	//critical water potential below which xylem cavitation will occur.
	const double psiCrit(-15000); //15000 hPa or 1.5 MPa
	if (potentialTranspiration) {
		if (psiCollar < psiCrit + 1) psiCollar += 100; //try to get out of drought.
		if (psiCollar > -20) psiCollar = -20; //try to get out away from saturated conditions
	} else {
		waterPotentialAtTheCollar->get(t, psiCollar);
	}

	//build the matrix and righthand side
	//std::cout<<std::endl<<"============================================================";
	//std::cout<<std::endl<<"solving model at "<<t<<" with trans="<<trans<<" initial psi collar "<<psiCollar;
	build(t);

	//solve
	Pcg::solve(C, b, psi_x, pcgprecision, 150000, SYM_GAUSS_SEIDEL);//note double precision only give us 14 digits
	jsum = (Lr * (psi_s - psi_x)).sum();
	//std::cout<<std::endl<<t<<" initial collar psi "<<psiCollar<<" trans "<<trans<<" jsum "<<jsum;

	//minimize fabs(trans-jsum) by changing collarpotential
	if (potentialTranspiration) {
		double psiCollarOld = psiCollar;
		double jsumOld = jsum;
		int count = 0;
		while (fabs(jsum - trans) > smalltrans) {
			if (trans > jsum){
				psiCollar -= 50.;
			} else {
				psiCollar += 50.;
			}
			//set limits (assymptote)
			if (psiCollar < psiCrit) psiCollar = psiCrit;
			//update boundary condition at the collar
			b[collarnodeIndex] = collarKh*psiCollar;
			//Solve (result is psi_x).
			Pcg::solve(C, b, psi_x, pcgprecision, 150000, SYM_GAUSS_SEIDEL);//note double precision only give us 14 digits
			//sum of all the fluxes
			jsum = (Lr * (psi_s - psi_x)).sum();
			if ((fabs(jsum - trans) <= smalltrans)) break;
			psiCollar = psiCollarOld + (psiCollar - psiCollarOld)*(trans - jsumOld)/(jsum - jsumOld);
			//set limits (assymptote)
			if (psiCollar < psiCrit) psiCollar = psiCrit;
			//update boundary condition at the collar
			b[collarnodeIndex] = collarKh*psiCollar;
			//Solve (result is psi_x).
			Pcg::solve(C, b, psi_x, pcgprecision, 150000, SYM_GAUSS_SEIDEL);//note double precision only give us 14 digits
			//sum of all the fluxes
			jsum = (Lr * (psi_s - psi_x)).sum();
			if (psiCollar < psiCrit + 1 && jsum < trans) {
				msg::warning("WaterUptakeDoussanModel: Root system can not take up enough water at day " + std::to_string((int) t));
				break;
			}
			if (fabs(jsum - trans) > smalltrans){
				msg::warning("WaterUptakeDoussanModel: Can not find solution incrementally, trying direct interpolation.");
				while (fabs(jsum - trans) > smalltrans) {
					// Calculate water uptake for psi_collar = -100
					b[collarnodeIndex] = -100*collarKh;
					//Solve (result is psi_x).
					Pcg::solve(C, b, psi_x, pcgprecision, 150000, SYM_GAUSS_SEIDEL);//note double precision only give us 14 digits
					//sum of all the fluxes
					double jsumSmall = (Lr * (psi_s - psi_x)).sum();
					// Calculate water uptake for psi_collar = psiCrit
					b[collarnodeIndex] = collarKh*psiCrit;
					//Solve (result is psi_x).
					Pcg::solve(C, b, psi_x, pcgprecision, 150000, SYM_GAUSS_SEIDEL);//note double precision only give us 14 digits
					//sum of all the fluxes
					double jsumCrit = (Lr * (psi_s - psi_x)).sum();
					// Find psi_collar by interpolating. Works because problem is linear
					psiCollar = -100 + (psiCrit + 100)*(trans - jsumSmall)/(jsumCrit - jsumSmall);
					//set limits (assymptote)
					if (psiCollar < psiCrit) psiCollar = psiCrit;
					b[collarnodeIndex] = collarKh*psiCollar;
					//Solve (result is psi_x).
					Pcg::solve(C, b, psi_x, pcgprecision, 150000, SYM_GAUSS_SEIDEL);//note double precision only give us 14 digits
					//sum of all the fluxes
					jsum = (Lr * (psi_s - psi_x)).sum();
					pcgprecision*=0.1; // In case solution is not found immediately, increase precision of pcg solver
					count += 1;
					if (psiCollar < psiCrit + 1 && jsum < trans) {
						msg::warning("WaterUptakeDoussanModel: Root system can not take up enough water at day " + std::to_string((int) t));
						break;
					}
					if (count > 5) {
						msg::warning("WaterUptakeDoussanModel: Can not find accurate solution for Doussan model at time = " + std::to_string(t)+ ", continuing with small imprecision.");
						break;
					}
				}
			}
		}
	}
	//set the uptake for all the root nodes.
	writeValues(t);
}

DerivativeBase * newInstantiationWaterUptakeDoussanModel(
		SimulaDynamic* const pSD) {
	return new WaterUptakeDoussanModel(pSD);
}


//Schnepf implements doussan+gravity
class WaterUptakeSchnepf:public WaterUptakeDoussanModel{
public:
	WaterUptakeSchnepf(SimulaDynamic* const pSD):WaterUptakeDoussanModel(pSD){
		useGravity=true;
	}
	virtual std::string getName()const{
		return "waterUptakeAlmDoussanSchnepfModel";
	};

};

DerivativeBase * newInstantiationWaterUptakeSchnepf(
		SimulaDynamic* const pSD) {
	return new WaterUptakeSchnepf(pSD);
}


HydraulicConductivityRootSystem::HydraulicConductivityRootSystem(
		SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	Unit u = pSD->getUnit();
	if (u == "cm3/day/g/hPa") {
		size = pSD->getSibling("rootDryWeight", "g");
	} else if (u == "cm3/day/cm2/hPa") {
		size = pSD->getSibling("rootSurfaceArea", "cm2");
	} else if (u == "cm3/day/cm3/hPa") {
		size = pSD->getSibling("rootVolume", "cm3");
	} else {
		msg::error(
				"HydraulicConductivityRootSystem: unknown unit " + u.name
						+ " for " + pSD->getName()
						+ " use cm3/day/g/hPa, cm3/day/cm3/hPa or cm3/day/cm2/hPa");
	}
	collarPotential = pSD->getSibling("waterPotentialAtTheCollar", "hPa");
	flowrate = pSD->getSibling("rootWaterUptake", "cm3");

}
std::string HydraulicConductivityRootSystem::getName() const {
	return "hydraulicConductivityRootSystem";
}
void HydraulicConductivityRootSystem::calculate(const Time &t, double &K) {
	double s, h, f;
	size->get(t, s);
	if (s > 0) {
		collarPotential->get(t, h);
		flowrate->getRate(t, f);
		K = f / s / (-h); //ml/day/g/hPa
	} else {
		K = 0;
	}
}
DerivativeBase * newInstantiationHydraulicConductivityRootSystem(
		SimulaDynamic* const pSD) {
	return new HydraulicConductivityRootSystem(pSD);
}

//Register the module
static class AutoRegisterDoussanInstantiationFunctions {
public:
	AutoRegisterDoussanInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["waterUptakeDoussanModel"] =
				newInstantiationWaterUptakeDoussanModel;
		BaseClassesMap::getDerivativeBaseClasses()["waterUptakeAlmDoussanSchnepfModel"] =
				newInstantiationWaterUptakeSchnepf;
		BaseClassesMap::getDerivativeBaseClasses()["hydraulicConductivity"] =
				newInstantiationLateralHydraulicConductivity;
		BaseClassesMap::getDerivativeBaseClasses()["hydraulicConductivityRootSystem"] =
				newInstantiationHydraulicConductivityRootSystem;
	}
} l6k5tfottos789g97f;

