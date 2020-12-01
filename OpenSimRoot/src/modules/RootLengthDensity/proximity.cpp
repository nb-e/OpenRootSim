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
#if _WIN32 || _WIN64
#define _USE_MATH_DEFINES
#endif
#include "proximity.hpp"
#include "../PlantType.hpp"
#include "../../cli/Messages.hpp"
#include <math.h>
#include "../../math/VectorMath.hpp"
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

std::string Proximity::getName() const {
	return "proximity";
}

Proximity::Proximity(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//set coordinate
	pSD->getAbsolute(pSD->getStartTime(), center);

	//set
	SimulaBase *r(pSD->existingChild("searchRadius"));
	if (r) {
		r->get(radius);
	} else {
		radius = 1;
	}
	//plantTYpe
	if (pSD->getName() == "nearestNeighborRootOfOtherSpecies") {
		PLANTTYPE(plantType, pSD)
	} else {
		PLANTNAME(plantType, pSD)
	}
}
void Proximity::calculate(const Time &t, double &var) {
	SimulaBase::Positions list, list2;
	pSD->getPositionsWithinRadius(t, center, radius, list);
	//eliminate nodes from same root
	SimulaBase *root1(pSD->getParent(3)), *root2;
	for (auto & it : list) {
		root2 = it.second->getParent(2);
		//if(root1==root2) list.erase(it--);//note this must be post decrement of iterator, it will decrement the iterator and than return old copy to erase.
		if (root1 != root2)
			list2.insert(it);
	}
	//Branch modes based on name
	if (pSD->getName() == "neighborCount") {
		//return count of number of neighboring root nodes within radius
		var = (double) list2.size();
	} else if (pSD->getName() == "nearestNeighbor") {
		//return nearest neighbor
		double distance(radius);
		for (auto &it : list2) {
			distance = fmin(distance, vectorlength(center, it.first));
		}
		var = distance;
	} else if (pSD->getName() == "spatialRootDensity") {
		//length within the sphere
		double length(0), tl;
		SimulaBase *p1;
		Coordinate c1, c2;
		for (auto & it : list2) {
			//two pointers
			p1 = it.second->getChild("rootSegmentLength", t);
			//length of the segment
			p1->get(t, tl);
			//see if p2 is inside the sphere
			p1->getAbsolute(t, c2);
			if (vectorlength(c2, center) < radius) {
				//segment is totally inside the cylinder
				length += tl;
			} else {
				//segment intersects with cylinder boundary: calculate what lengths is inside the cylinder
				double u = intersectLineWithSphere(it.first, c2, center,
						radius);
				//TODO redundant check, remove if code is working well
				if (u > 1)
					msg::error("proximity: conceptual error, check code");
				//add u to the length
				length += tl * u;
			}
		}
		//volume of the sphere
		double volume = radius * radius * radius * (4. / 3.) * M_PI;
		//divide length/volume
		var = length / volume;
	} else if (pSD->getName() == "nearestNeighborRootOfOtherSpecies") {
		//return nearest neighbor
		double distance(radius);
		std::string plantTypeRoot;
		for (auto & it : list2) {
			PLANTTYPE(plantTypeRoot, it.second)
			if (plantTypeRoot != plantType)
				distance = fmin(distance, vectorlength(center, it.first));
		}
		var = distance;
		//note that in worst case var could be sqrt(0.5^2+0.5^2)= 0.7 cm larger.
	} else if (pSD->getName() == "nearestNeighborRootOfOtherPlant") {
		//return nearest neighbor
		double distance(radius);
		std::string plantTypeRoot;
		for (auto & it : list2) {
			PLANTNAME(plantTypeRoot, it.second)
			if (plantTypeRoot != plantType)
				distance = fmin(distance, vectorlength(center, it.first));
		}
		var = distance;
		//note that in worst case var could be sqrt(0.5^2+0.5^2)= 0.7 cm larger.
	} else if (pSD->getName() == "nearestNeighborRootOfSelf") {
		//return nearest neighbor
		double distance(radius);
		std::string plantTypeRoot;
		for (auto & it : list2) {
			PLANTNAME(plantTypeRoot, it.second)
			if (plantTypeRoot == plantType)
				distance = fmin(distance, vectorlength(center, it.first));
		}
		var = distance;
		//note that in worst case var could be sqrt(0.5^2+0.5^2)= 0.7 cm larger.
	} else {
		msg::error(
				"Proximity: unknown mode '" + pSD->getPath()
						+ "' . Currently programmed modes are: \"neighborCount\", \"nearestNeighbor\", \"spatialRootDensity\"");
	}

}
DerivativeBase * newInstantiationProximity(SimulaDynamic* const pSD) {
	return new Proximity(pSD);
}

//Register the module
static class AutoRegisterProximityInstantiationFunctions {
public:
	AutoRegisterProximityInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["proximity"] =
				newInstantiationProximity;
	}
} p479874958375092845;

