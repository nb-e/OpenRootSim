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
#include "RootLengthProfile.hpp"
#include "../../cli/Messages.hpp"
#include "../PlantType.hpp"
//#include "../../MathLibrary/MathLibrary.hpp"
#include "../../engine/SimulaTable.hpp"
#include <valarray>
#include <utility>

std::string RootLengthProfile::getName() const {
	return "rootLengthProfile";
}
RootLengthProfile::RootLengthProfile(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//determine the thickness of the layer
	pSD->getChild("y1")->get(y1);
	pSD->getChild("y2")->get(y2);
}
void RootLengthProfile::calculate(const Time &t, double &length) {
	///@TODO this module slows everything down a lot if it driving plant growth, but if it is called after growth is completed, it is rather quick.
	//get y slice
	SimulaBase::Positions listOfRootNodes;
	pSD->getYSlice(t, y1, y2, listOfRootNodes);
	//sum up length of all segments
	length = 0;
	double tl;
	for (auto & it : listOfRootNodes) {
		SimulaBase *l(it.second->getChild("rootSegmentLength"));
		l->get(t, tl);
		length += tl;
	}
}

DerivativeBase * newInstantiationRootLengthProfile(SimulaDynamic* const pSD) {
	return new RootLengthProfile(pSD);
}

std::string RootLengthDensity::getName() const {
	return "rootLengthDensity";
}

RootLengthDensity::RootLengthDensity(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	pSD->checkUnit("cm/cm3");
	//determine the thickness of the layer
	pSD->getChild("location")->get(loc);
	pSD->getChild("radius")->get(r);
	vol = PI * r * r * r * 4 / 3;
}
void RootLengthDensity::calculate(const Time &t, double &length) {
	///@TODO this module slows everything down a lot if it driving plant growth, but if it is called after growth is completed, it is rather quick.
	//get y slice
	SimulaBase::Positions listOfRootNodes;
	pSD->getPositionsWithinRadius(t, loc, r, listOfRootNodes);
	//sum up length of all segments
	length = 0;
	double tl;
	for (auto & it : listOfRootNodes) {
		it.second->getChild("rootSegmentLength")->get(t, tl);
		length += tl;
	}
	length /= vol;
}

DerivativeBase * newInstantiationRootLengthDensity(SimulaDynamic* const pSD) {
	return new RootLengthDensity(pSD);
}

std::string SuperCoring::getName() const {
	return "virtualCoring";
}

SuperCoring::SuperCoring(SimulaDynamic* pSD) :
		DerivativeBase(pSD), label(pSD->getName()), center(0, 0, 0), spacing(
				10), radius(2), sqradius(4), depth(-60), maxslices(1), mindia(-1.), maxdia(-1.) {
	//determine cores
	//pSD->getChild("label")->get(label);
	pSD->getChild("radius")->get(radius);
	sqradius = radius * radius;
	pSD->getChild("center")->get(center);
	pSD->getChild("verticalSpacing")->get(spacing);
	pSD->getChild("coringDepth")->get(depth);
	maxslices = abs(depth / spacing);
	depth = -maxslices * spacing;//correct if depth is not multiple of max slices

	//diameter
	SimulaBase *p=pSD->existingChild("minThresh");
	if(p) p->get(mindia);
	p=pSD->existingChild("maxThresh");
	if(p) p->get(maxdia);
	p=pSD->existingChild("parameterForThresholding");
	if(p) {
		p->get(threshparam);
	}else{
		threshparam="rootDiameter";
	}

	//param
	p=pSD->existingChild("parameter");
	if(p) {
		p->get(param);
	}else{
		param="rootSegmentLength";
	}


	os.open("coringData_" + label + ".tab");
	center.y = depth;
	if (!os.is_open())
		msg::error("generateTable: Failed to open coringData.tab");
	os << std::endl << "label" << "\t" << "time(d)" << "\t" << "x" << "\t"
			<< "depth" << "\t" << "z" << "\t" << "radius" << "\t" << "total";
	for (int i(0); i < maxslices; ++i) {
		os << "\t" << i * spacing << "-" << (i + 1) * spacing;
	}

}

SuperCoring::~SuperCoring() {
	os.close();
}

void SuperCoring::calculate(const Time &t, double &length) {
	///@TODO this module slows everything down a lot if it driving plant growth, but if it is called after growth is completed, it is rather quick.
	//get y slice
	SimulaBase::Positions listOfRootNodes;
	std::valarray<double> data(0., maxslices);
	double tl;
	Coordinate p1, p2;
	double y1(0), y2(-spacing), dx, dz, d1, d2, frac, yfrac, yvfrac;
	double ml = 2;	//max segment length
	SimulaBase *l, *np;
	for (int slice(0); slice < maxslices; ++slice) {
		pSD->getYSlice(t, y2 - ml, y1 + ml, listOfRootNodes);
		//sum up length of all segments
		for (auto & it : listOfRootNodes) {
			it.second->getAbsolute(t, p1);
			dx = (p1.x - center.x);
			dz = (p1.z - center.z);
			d1 = dz * dz + dx * dx;
			frac = 0;
			tl = 0;
			yfrac = 0;
			yvfrac = 0;
			if (d1 < sqradius + ml) {
				//segment might be in core
				l = it.second->getChild(param);
				np = l->followChain(t);
				np->getAbsolute(t, p2);
				dx = (p2.x - center.x);
				dz = (p2.z - center.z);
				d2 = dz * dz + dx * dx;
				//calculate radially
				d1 = sqrt(d1);
				d2 = sqrt(d2);
				if (d1 > d2)
					std::swap(d1, d2);
				if (d2 > d1) {
					frac = (radius - d1) / (d2 - d1);
					if (frac < 0)
						frac = 0;
					if (frac > 1)
						frac = 1;
				} else if (d1 > radius) {
					//both points are to far away from the mid of the core
					frac = 0;
				} else {
					//both points are close enough to the mid axis of the core
					frac = 1;
				}

				//calculate vertically (assuming root segments are shorter then core segment)
				if (p1.y < p2.y)
					std::swap(p1.y, p2.y);
				if (p1.y > p2.y) {
					if (p1.y > y1) {
						//most shallow point p1 higher then top y1
						yfrac = (y1 - p2.y) / (p1.y - p2.y);
						if (yfrac < 0)
							yfrac = 0;
						if (yfrac > 1)
							yfrac = 1;
					} else if (p2.y < y2) {
						//deepest point p2 below bottom y2
						yfrac = (p1.y - y2) / (p1.y - p2.y);
						if (yfrac < 0)
							yfrac = 0;
						if (yfrac > 1)
							yfrac = 1;
					} else {
						//root is inside complete
						yfrac = 1;
					}
				} else if (p1.y > y1 || p1.y < y2) {
					//horizontal root outside
					yfrac = 0;
				} else {
					//horizontal root inside
					yfrac = 1;
				}

				//yfrac top and bottom core
				if (p1.y > p2.y) {
					if (p1.y > 0) {
						//shallowest point p1 higher then top y1
						yvfrac = (0 - p2.y) / (p1.y - p2.y);
						if (yvfrac < 0)
							yvfrac = 0;
						if (yvfrac > 1)
							yvfrac = 1;
					} else if (p2.y < depth) {
						//deepest point p2 below bottom y2
						yvfrac = (p1.y - depth) / (p1.y - p2.y);
						if (yvfrac < 0)
							yvfrac = 0;
						if (yvfrac > 1)
							yvfrac = 1;
					} else {
						//root is inside complete
						yvfrac = 1;
					}
				} else if (p1.y > 0 || p1.y < depth) {
					//horizontal root outside
					yvfrac = 0;
				} else {
					//horizontal root inside
					yvfrac = 1;
				}

				//filter out roots that do not meet the diameter requirements
				if(mindia>0 || maxdia>0){
					auto pd0 = it.second->getChild(threshparam);
					auto pd1 = pd0->followChain(t);
					double d0,d1;
					pd0->get(t,d0);
					pd1->get(t,d1);
					if(mindia>0 && (d0<mindia || d1<mindia) ) frac=0;
					if(maxdia>0 && (d0>maxdia || d1>maxdia) ) frac=0;
					msg::warning("threshing"+threshparam);
				}

				//take minimum fraction for inside (what ever side is crossed first)
				l->get(t, tl);
				tl *= fmin(frac, yfrac);
				data[slice] += tl;
			}
			l = it.second->existingChild(param+"InCore_" + label);
			if (l) {
				SimulaTable<double>* d = dynamic_cast<SimulaTable<double>*>(l);
				if (d) {
					d->set(t, fmin(frac, yvfrac));//todo ignores vertical boundaries of the core (not slice)
				}
			}
			l = it.second->existingChild("rootSegmentLengthInCore");
			if (l) {
				SimulaTable<double>* d = dynamic_cast<SimulaTable<double>*>(l);
				if (d) {
					d->set(t, fmin(frac, yvfrac));//todo ignores vertical boundaries of the core (not slice)
				}
			}
		}
		y1 = y2;
		y2 -= spacing;
	}
	length = data.sum();

	//write data to file
	os << std::endl << label << "\t" << t << "\t" << center << "\t" << radius
			<< "\t" << length;

	for (auto &it : data) {
		os << "\t" << it;
	}
}

DerivativeBase * newInstantiationSuperCoring(SimulaDynamic* const pSD) {
	return new SuperCoring(pSD);
}

//++++++++++++++++++
std::string D95::getName() const {
	return "D95";
}

D95::D95(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//determine cut off
	SimulaBase* p(pSD->existingChild("threshold"));
	if (p) {
		p->get(threshold);
	} else {
		threshold = 0.95;
	}
	//total root length
	rootLength = pSD->existingSibling("rootLongitudinalGrowth", pSD->getUnit());
	if (!rootLength)
		rootLength = pSD->getSibling("rootLength", pSD->getUnit());
	//plant
	plant = pSD->getParent()->getName();
	SimulaBase::signalMeAboutNewObjects(pSD);
}

void D95::addObject(SimulaBase *rootNode) {
	//this is called each time a new root node is inserted.
	//check that this is a root node, not growth point or something else.
	std::string name = rootNode->getName();
	if (name.substr(0, 9) != "dataPoint") {
		return;
	}
	//check if the root node belongs to the same plant
	std::string plantType;
	PLANTNAME(plantType, rootNode);
	if (plantType != plant) return;
	Coordinate c;
	rootNode->getAbsolute(rootNode->getStartTime(),c);
	if (positions.insert(std::pair<Coordinate, SimulaBase*>(c, rootNode->getChild("rootSegmentLength"))) == positions.end()) msg::error("D95: Failed to insert coordinate.");
}

void D95::calculate(const Time &t, double &depth) {
	SimulaBase::updateAll(t);
	SimulaBase::Positions rootNodes;
	for(auto & it:positions){
		if(it.second->evaluateTime(t)) rootNodes.insert(it);
	}
	if (rootNodes.size() > 0){
		double tl, l, length(0);
		rootLength->get(t, tl);
		tl *= (1 - threshold);
		SimulaBase::Positions::iterator it(rootNodes.begin());
		while (length < tl){
			if (it == rootNodes.end()){
				msg::error("D95: programming error detected. Check file RootLengthProfile.cpp and if rootLength and rootLongitudinalGrowth give same result");
			}
			it->second->get(t, l);
			length += l;
			++it;
		}
		depth = it->first.y;
	} else {
		depth = 0;
	}
}

DerivativeBase * newInstantiationD95(SimulaDynamic* const pSD) {
	return new D95(pSD);
}

//++++++++++++++++++
std::string RootsBelowD95Solute::getName() const {
	return "rootsBelowD95Solute";
}

RootsBelowD95Solute::RootsBelowD95Solute(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//determine nutrient
	std::string nutrient(pSD->getParent()->getName());
	//d95 value
	d95 = ORIGIN->getChild("soil")->getChild(nutrient)->getChild("D90");
}
void RootsBelowD95Solute::calculate(const Time &t, double &length) {
	double depth;
	d95->get(t, depth);
	//get y slice
	SimulaBase::Positions listOfRootNodes;
	pSD->getYSlice(t, depth, -1e5, listOfRootNodes);
	//sum up length of all segments
	length = 0;
	double tl;
	for (auto & it : listOfRootNodes) {
		///@todo limit to current plant species, see d95 code above
		SimulaBase *l(it.second->getChild("rootSegmentLength"));
		l->get(t, tl);
		length += tl;
	}

}

DerivativeBase * newInstantiationRootsBelowD95Solute(SimulaDynamic* const pSD) {
	return new RootsBelowD95Solute(pSD);
}

class AutoRegisterRootLengthProfileInstantiationFunctions {
public:
	AutoRegisterRootLengthProfileInstantiationFunctions() {
		// register the maker with the factory
		BaseClassesMap::getDerivativeBaseClasses()["rootLengthProfile"] =
				newInstantiationRootLengthProfile;
		BaseClassesMap::getDerivativeBaseClasses()["D95"] = newInstantiationD95;
		BaseClassesMap::getDerivativeBaseClasses()["rootsBelowD95Solute"] =
				newInstantiationRootsBelowD95Solute;
		BaseClassesMap::getDerivativeBaseClasses()["rootLengthDensity"] =
				newInstantiationRootLengthDensity;
		BaseClassesMap::getDerivativeBaseClasses()["virtualCoring"] =
				newInstantiationSuperCoring;
	}
	;
};

static AutoRegisterRootLengthProfileInstantiationFunctions p8749587204985720349587;
