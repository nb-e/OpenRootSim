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

#include "GrowthDirection.hpp"
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaConstant.hpp"
#include <math.h>
#include "../../math/VectorMath.hpp"
#include "../PlantType.hpp"

//===========Growth Rate functions for Roots ===============
RootGrowthDirection::RootGrowthDirection(SimulaDynamic* pSD) :
		DerivativeBase(pSD), topBoundary(false), bottomBoundary(false), bounceOfSide(
				false), cannotgrowup(false), refTimeI(0), refTimeG(0), steps_(0) {
	parent = dynamic_cast<SimulaPoint*>(pSD);
	if (parent == nullptr)
		msg::error(
				"RootGrowthDirection: failed dynamic_cast: expected the parent to be of type SimulaPoint");

	//plant type
	std::string plantType;
	PLANTTYPE(plantType, pSD);

	//get the root type
	std::string rootType;
	pSD->getSibling("rootType")->get(rootType);

	//gravitropism and soil impedence
	SimulaBase* p(GETROOTPARAMETERS(plantType,rootType));
	impedenceSimulator = pSD->existingChild("soilImpedence");
	if (!impedenceSimulator){
		impedenceSimulator = p->getChild("soilImpedence.v2");
		refTimeI=pSD->getStartTime();
	}
	gravitropismSimulator = pSD->existingChild("gravitropism");
	if (!gravitropismSimulator){
		gravitropismSimulator = p->getChild("gravitropism.v2");
		refTimeG=pSD->getStartTime();
	}
	tropismSimulator = pSD->existingChild("tropisms");
	if (tropismSimulator) msg::warning("RootGrowthDirection: including tropisms");

	growthSimulator = pSD->getChild("rootLongitudinalGrowth", "cm");

	//initial direction
	if(parent->getTable()->size()<2){
		setInitialDirection(plantType);
	}else{
		//just in case the initial direction was read from the input file.
		idirection=parent->getTable()->begin()->second.rate;
	}

	//are we restricting roots to a box?
	SimulaBase* b(p->existingChild("topBoundary"));
	if (!b)
		b = pSD->existingChild("topBoundary");
	if (b)
		b->get(topBoundary);
	if (!b)
		b = pSD->existingChild("bottomBoundary");
	b = p->existingChild("bottomBoundary");
	if (b)
		b->get(bottomBoundary);
	b = p->existingChild("bounceOfTheSide");
	if (b)
		b->get(bounceOfSide);
	b = p->existingChild("cannotgrowup");
	if (b)
		b->get(cannotgrowup);

	if (minCorner == maxCorner) {
		SimulaBase* d(ORIGIN->existingChild("environment"));
		if(d)d=d->existingChild("dimensions");
		if (d) {
			d->getChild("minCorner")->get(minCorner);
			d->getChild("maxCorner")->get(maxCorner);
			if (minCorner.x >= maxCorner.x)
				msg::error(
						"RootGrowthDirection: Dimension of the box incorrect: min x corner not less than max x corner");
			if (minCorner.y >= maxCorner.y)
				msg::error(
						"RootGrowthDirection: Dimension of the box incorrect: min y corner not less than max y corner");
			if (minCorner.z >= maxCorner.z)
				msg::error(
						"RootGrowthDirection: Dimension of the box incorrect: min z corner not less than max z corner");
			//rotation parameters
			b = d->existingChild("rotateBox");
			if(b) b->get(rotateBox);
			if(rotateBox){
				b->getChild("rotationAngle")->get(angle);
				b->getChild("rotationAxis")->get(rotationAxis);
				b->getChild("rotationCenter")->get(rotationCenter);
			}
			//pot shape
			b = d->existingChild("roundPot");
			if (b){
				b->get(roundPot);
				if(roundPot) msg::warning("RootGrowthDirection: Simulating round pot");
			}
		} else {
			msg::warning(
					"GrowthDirection:: Dimensions of system not given, assuming huge space",2);
			minCorner.x = -1e9;
			minCorner.y = -1e9;
			minCorner.z = -1e9;
			maxCorner.x = 1e9;
			maxCorner.y = 0;
			maxCorner.z = 1e9;
		}

	}
	if(timeScalingFactor<0.){
		//first instance set the time scaling factor
		SimulaBase *pointer = ORIGIN->existingPath("/simulationControls/integrationParameters/timeScalingFactorForTropisms");
		if (pointer) {
			pointer->get(timeScalingFactor);
			msg::warning("RootGrowthDirection: setting timeScalingFactor to " + std::to_string(timeScalingFactor),3);
		}else {
			msg::warning("RootGrowthDirection: timeScalingFactor not found. Defaulting to 5.",3);
			timeScalingFactor=5.;//backward compatible value, do not change.
		}
	}

}

Coordinate RootGrowthDirection::minCorner;
Coordinate RootGrowthDirection::maxCorner;
Coordinate RootGrowthDirection::rotationCenter;
Coordinate RootGrowthDirection::rotationAxis;
double RootGrowthDirection::angle;
bool RootGrowthDirection::rotateBox(false);
bool RootGrowthDirection::roundPot(false);
double RootGrowthDirection::timeScalingFactor(-1.);

void RootGrowthDirection::getRadialBranchingAngle(double & alpha, Unit & u,
		const std::string &plantType) {
	//set the units
	u = UnitRegistry::getUnit("degrees");

	//check that the radial Branching angle is not already given or set
	SimulaBase* p=pSD->existingChild("radialBranchingAngle",u);
	if(p){
		p->get(alpha);
		return;
	}

	//child root pointer
	SimulaBase* child(pSD->getParent());
	//get type of the child root
	std::string typeChild;
	child->getChild("rootType")->get(typeChild);
	//if this is the primary root return 0
	if (typeChild == "primaryRoot") {
		//store the direction
		alpha = 0;
		return;
	}

	//root type parameters of the root
	SimulaBase* rootTypeParametersChild(GETROOTPARAMETERS(plantType,typeChild));

	//parent root pointer
	SimulaBase* parent(child->getParent(3));
	//get the root type of the parent root
	std::string typeParent;
	parent->getChild("rootType")->get(typeParent);
	//root type parameters of the parent root
	SimulaBase* rootTypeParametersParent(
			GETROOTPARAMETERS(plantType,typeParent));

	//Number of xylem poles in the parent root
	int nrXylem;
	rootTypeParametersParent->getChild("numberOfXylemPoles")->get(
			nrXylem);
	if (nrXylem < 1)
		msg::error(
				"GrowthDirection: number of xylem poles smaller than 1 for "
						+ typeParent + " of plant " + plantType);

	//number of branches per whorl
	int numberOfBranchesPerWhorl(1);
	p=rootTypeParametersParent->getChild("branchList")->getChild(typeChild)->existingChild("numberOfBranches/whorl");
	if (p)
		p->get(numberOfBranchesPerWhorl);

	// Check if allometric scaling is active & account for effect on root count:
	// Branching angle calculation needs to use approximately the same number
	// of roots initiated by RootBranches::generateWhorl, otherwise when
	// scaling factor was small we would generate a partial whorl of roots
	// spaced at full-whorl radial angles ==> child roots would cluster on one
	// side of the parent axis
	// TODO:
	//	This redoes a calculation already performed by rootBranches::generateWhorl.
	//	A better fix would be to refactor so calculation is only done once,
	//  and result is either passed around or better yet only *needed* in one place.
	if (p && numberOfBranchesPerWhorl > 1) {
		bool do_allom(false);
		p = p->existingSibling("allometricScaling");
		if (p) {
			p->get(do_allom);
		}
		if (do_allom) {
			p = pSD;
			PLANTTOP(p);
			p = p->getChild("plantPosition")->getChild("shoot")->existingChild("leafAreaReductionCoefficient");
			if (p) {
				double allom_factor(1.0);
				p->get(pSD->getStartTime(), allom_factor);
				numberOfBranchesPerWhorl = (int) round(allom_factor * (double) numberOfBranchesPerWhorl);
			}
		}
	}

	// regular / irregular topology
	int regularTopology(0);
	SimulaBase * rt(
			rootTypeParametersChild->existingChild(
					"regularTopology"));
	if (rt)
		rt->get(regularTopology);

	//topology offset or fixed position
	int offset(0);
	if (regularTopology) {
		p = rootTypeParametersChild->existingChild("topologyOffset");
		if (p)
			p->get(offset);
	}

	//branch number in the current whorl
	if (numberOfBranchesPerWhorl > 1) {
		//which branch this is in the current whorl / position. Extract it from the name
		std::string name = child->getName();
		//try to convert the name
		int i = convertFromString<int>(name.substr(name.find_last_of('-') + 1))
				- 1;
		offset = 0;
		regularTopology += (int) round(
				i * (double) nrXylem / (double) numberOfBranchesPerWhorl);
	} else {
		//set regulorTopology to random number if it is not set by now
		if (!regularTopology && !offset)
			regularTopology = randomIntDavis(nrXylem);
	}

	//the radial branching angle alpha depends on the position of the Xylem pole from which the root branches
	alpha = regularTopology * 360 / nrXylem;
	if (offset) {
		//topology is an offset since last branch and needs to be converted to current absolute position
		SimulaBase* l(child->getPreviousSibling());
		if (l) { //there is a previous branch
				 //get angle of the previous branch
			double last(0);
			//previous branch's growthpoint
			l = l->getChild("growthpoint");
			//make sure the branch is initiated
			Coordinate c;
			l->get(pSD->getStartTime(), c);
			//get the radialBranchingAngle
			l->getChild("radialBranchingAngle")->get(last);
			//add
			alpha += last;
		}
	}
	while (alpha > 360)
		alpha -= 360;

	//store alpha as a simulaconstant child object
	new SimulaConstant<double>("radialBranchingAngle", pSD, alpha, u,
			pSD->getStartTime());
}

void RootGrowthDirection::getLateralBranchingAngle(double &beta, Unit & u,
		const std::string &plantType) {
	//get the root type
	std::string rootType;
	pSD->getSibling("rootType")->get(rootType);

	//pointer to roottypeparameters
	SimulaBase* p(GETROOTPARAMETERS(plantType,rootType));

	//the lateral branching angle beta is a roottype specific constant
	p = p->getChild("branchingAngle");
	p->get(beta);
	u = p->getUnit();
}


void RootGrowthDirection::setInitialDirection(const std::string &plantType) {
	//pointer to parent root (top of this rootgrowthpoint root tree)
	SimulaBase* currentRoot(pSD->getParent());
	SimulaBase* parentRoot(currentRoot->getParent(3));
	//Basepoint of the branch relative to basepoint of the parent root
	Coordinate base;
	currentRoot->get(base);

	//Direction of segment of parent root from the basepoint onwards
	if (parentRoot) {
		parentRoot = parentRoot->existingChild("growthpoint");
	}

	if (!parentRoot) {
		idirection = Coordinate(0, -1, 0);
		idirection.x += 0.20 * ((double(rand()) / double(RAND_MAX)) - 0.5);
		idirection.z += 0.20 * ((double(rand()) / double(RAND_MAX)) - 0.5);
		normalizeVector(idirection);
		if (currentRoot->getName() == "hypocotyl")
			idirection *= -1;
		return;
	}

	SimulaPoint* parentsGrowthpoint=dynamic_cast<SimulaPoint*>	(parentRoot);
	Coordinate directionParent,projectionPoint;
	parentsGrowthpoint->getTangent(base, directionParent,projectionPoint);

	if (directionParent.x == 0 && directionParent.y == 0 && directionParent.z == 0) {
		msg::warning(
				"RootGrowthDirection: Failed to determine the growth direction of the parent root " + parentsGrowthpoint->getName() + " assuming vertical");
		parentsGrowthpoint->getTangent(base, directionParent, projectionPoint);
		directionParent.y = 1;
	}
	//get vector in the vertical plane
	/* TODO note: this version gives the perpendicular perpendicular to the vertical plane while
	 * the original SimRoot model gives the perpendicular perpendicular to the
	 * directionParent-Xaxis plane which I think is a mistake? (it also switches to
	 * z axes when root is parallel to x axes.)
	 *
	 * ///@todo: precision error maybe avoided if corner between cp and directionParent is sufficient large
	 */
	//default (radialangle = 0 ) is in the vertical plane down
	Coordinate vp(0, 1, 0);
	if (directionParent.x == 0 && directionParent.z == 0) {
		vp.x = 1;
		vp.y = 0;
	}

	//get the perpendicular
	Coordinate perp;
	//get radial branching angle
	double angle;
	Unit u;
	projectionPoint -= base;
	if (projectionPoint.length() > 0.01) {
		msg::warning("RootGrowthDirection:setInitialDirection: not rotating, as the root is not attached");
		angle = u.factor * 0.20 * ((double(rand()) / double(RAND_MAX)) - 0.5);
		perp = perpendicular(projectionPoint, directionParent);
	} else {
		getRadialBranchingAngle(angle, u, plantType);
		perp = (perpendicular(vp, directionParent));
		perp = rotateAroundVector(perp, directionParent, angle, u);
	}

	//turn the perpendicular around the parent using the radial brancing angle.
	//rotate 'direction' alpha degrees around 'directionParent'.
	/*TODO: Note, the plane directionParent - perpendicular is in 90 degrees to
	 * the plane directionParent-direction. Since the definition of alpha=0
	 * is not clear, I don't know whether this should be corrected. The old
	 * SimRoot does not correct this.
	 */

	//get lateral branching angle
	getLateralBranchingAngle(angle, u, plantType);

	//turn direction beta out from directionParent using the perpendicular
	idirection = directionParent;
	idirection = rotateAroundVector(idirection, perp, angle, u);

	//normalize direction
	normalizeVector(idirection);
}


void RootGrowthDirection::calculate(const Time &t, Coordinate &direction) {
	//initial direction
	direction = idirection;
	//speed
	double gr;
	growthSimulator->getRate(t, gr);
	if (gr == 0) {
		direction *= 0;
		return;
	}
	//direction
	///@todo how do we correct for growth speed. Logical solution add to each vector that changes the direction a weight vector for growth
	///@todo, getting the last direction could be more simple, if we could garantee that this function is only and sequentially called by the growthPoint Simulator
	//change direction since last time
	///@todo this is just an ugly hack that makes is slower and slower to find the direction when a root is not growing. It's best that this function is not called at all for a non growing root.
	SimulaPoint::Table const * const table(parent->getTable());
	Coordinate lastDirection;
	bool found(false);
	Time st, et;
	pSD->getTimeStepInfo(st,et);
	for (auto it(table->rbegin()); it != table->rend(); ++it) {
		lastDirection = it->second.rate;
		if (lastDirection.length() > 1E-8 && it->first<et-TIMEERROR) {
			found = true;
			break;
		}
	}

	//timestep correction factor
	double iet;
	double fet= modf(et*timeScalingFactor , &iet);
	int nor=iet-steps_;
	if (fet>0.5) ++nor;
	steps_+=nor;
	//std::cout<<std::endl<<std::setprecision(4)<<st<<" "<<et<<" "<<nor<<" "<<fst<<" "<<fet<<" "<<iet-ist;

	//We run into trouble here when root didn't grow, get the initial branching direction instead
	if (found) {
		direction = lastDirection;
		int count=0;
		while(count<nor){
			++count;
			//normalize the previous growth vector
			normalizeVector(direction);
			//soil impedence
			Coordinate impedence;
			impedenceSimulator->get(t - refTimeI, direction, impedence);
			direction += impedence;
			normalizeVector(direction);
			//gravitropism
			Coordinate tropism(0, 0, 0);
			gravitropismSimulator->get(t - refTimeG, tropism);
			direction.y += tropism.y;
			//other tropisms
			if(tropismSimulator){
				if (direction.x != 0 && direction.y != 0 && direction.z != 0)	normalizeVector(direction);
				tropismSimulator->get(t, direction, tropism);
				direction += tropism;
			}
		}
	}
	//make sure roots are not growing upt
	if (cannotgrowup && direction.y > 0) {
		direction.y = 0;
		if (direction.x == 0 && direction.z == 0) {
			direction.x = (((double) rand() / (double) RAND_MAX) - 0.5);
			direction.z = (((double) rand() / (double) RAND_MAX) - 0.5);
		}
	}
	//normalize
	normalizeVector(direction);
	//multiply with growth rate
	direction *= gr;
}

bool RootGrowthDirection::postIntegrationCorrection(SimulaPoint::Table &table) {
	bool r(false);
	//iterators to last two rows in table
	auto lit(table.rbegin()), pit(lit);
	++pit;
	//absolute base position
	Coordinate b;
	pSD->getBase(lit->first, b);
	if (topBoundary) {
		//n is the absolute location after integration
		Coordinate n(b);
		n += lit->second.state;
		if (n.y > maxCorner.y) {
			MovingCoordinate m(pit->second);
			if (m.rate.y > 0) { //growing up
				//allow roots to grow above the soil when they are already there
				if ((b.y + m.state.y) < 0) {
					//store original growthrate - it should not change by this
					double l1(m.rate.length());
					//try to detect original x,z direction (just in case rate.x and rate.z are zero)
					for (auto it(pit); (m.rate.x == 0 && m.rate.z == 0); ++it) {
						if (it == table.rend()) {
							m.rate.x = 1;
							break;
						}
						m.rate = it->second.rate;
					}
					//make sure the root does not grow above the soil
					if (n.y > maxCorner.y) {
						m.rate.y = 0;
					} else {
						m.rate.y = pit->second.rate.y / 2;
					}
					//growthrate should not be effected by this
					normalizeVector(m.rate);
					m.rate *= l1;

					//store new rate and return true to redo the integration
					pit->second.rate = m.rate;
					lit->second.rate = pit->second.rate;
					r = true;
				} else {
					if ((b.y + m.state.y) > 0.1) {
						std::string type;
						pSD->getSibling("rootType")->get(
								type);
						msg::warning(
								"RootGrowth: " + type
										+ " Growing above the ground");
					}
				}
			}
		}
	}
	if (bottomBoundary) {
		//n is the absolute location after integration
		Coordinate n(b);
		n += lit->second.state;
		//last position
		Coordinate m(pit->second.rate);
		double l1(vectorlength(m));

		//rotation of box/pot
		if(rotateBox){
			n-=rotationCenter;
			double l2=n.length();
			n=rotateAroundVector(n, rotationAxis, angle, "degrees")*l2;
			n+=rotationCenter;
			m=rotateAroundVector(m, rotationAxis, angle, "degrees")*l1;
		}

		if (n.y < minCorner.y) {
			//make sure the root does not grow bellow bottom boundary
			//todo disabled this as it causes strange vertical up growth behaviour.
			/*if(n.y< minCorner.y-0.6){
				m.y=1;
			}else{*/
			    m.y = 0;
			//}
		}
		if (roundPot) {
			Coordinate center((maxCorner + minCorner) / 2);
			center.y=0;
			Coordinate edge(maxCorner-center);
			edge.y=0;
			if(edge.x>edge.z){//make sure circle is inside box not around the outside.
				edge.x=0;
			}else{
				edge.z=0;
			}
			double radius(edge.length());
			Coordinate nt(n-center);
			nt.y=0;


			if (vectorlength(nt,center)>radius) {
				Time dt=lit->first-pit->first;
				MovingCoordinate mt(pit->second);
				mt+=b;
				mt-=center;
				mt.state.y=0; mt.rate.y=0;
				double oz=0.5*mt.rate.length()*dt;
				double sz=mt.length();
				double a=2*asin(oz/sz);
				Coordinate ntp=rotateAroundVector(mt.state, Coordinate (0,1,0), a , "radians"  )*radius;
				Coordinate ntn=rotateAroundVector(mt.state, Coordinate (0,1,0), -a , "radians"  )*radius;

				if(vectorlength(ntp,nt)>vectorlength(ntn,nt)){//rotate at smallest angle.
					nt=ntn;
				}else{
					nt=ntp;
				}
				if(nt.length()>radius){
					//roots escaped, bring them back in
					nt.setLength(radius);
				}
				m.x=nt.x-mt.state.x;
				m.z=nt.z-mt.state.z;

			}

		} else if (bounceOfSide) {
			//rectangular box, bouncing of side
			if (n.x < minCorner.x) {
				//store original growthrate - it should not change by this
				if (m.x > 0) {
					//sometimes happens with rk4 when randomness swaps the sign between r1,r2,r3 and r4
					r = true; //force forward euler
				} else {
					//make sure the root does not grow out of the box
					m.x *= -1;
				}
			}
			if (n.x > maxCorner.x) {
				if (m.x < 0) {
					msg::warning(
							"RootGrowthDirection::postIntegrationCorrection: maxx boundary broken");
					//r =false;
					m.x = 0;
				} else {
					m.x *= -1;
				}
			}
			if (n.z < minCorner.z) {
				if (m.z > 0) {
					//sometimes happens with rk4 when randomness swaps the sign between r1,r2,r3 and r4
					r = true; //force forward euler
				} else {
					m.z *= -1;
				}
			}
			if (n.z > maxCorner.z) {
				if (m.z < 0) {
					//sometimes happens with rk4 when randomness swaps the sign between r1,r2,r3 and r4
					r = true; //force forward euler
				} else {
					m.z *= -1;
				}
			}
		} else {
			//rectangular box follow side
			if (n.x < minCorner.x)
				m.x = 0;
			if (n.x > maxCorner.x)
				m.x = 0;
			if (n.z < minCorner.z)
				m.z = 0;
			if (n.z > maxCorner.z)
				m.z = 0;

		} //end if  roundpot / bounce of side / follow side

		//corner protection
		if (m.x == 0 && m.y == 0 && m.z == 0) {
			//we are trapped in a corner
			if (n.y > minCorner.y + 2) {
				m.y = -1; //simply grow down
			} else {
				//rotate clockwise
				if (n.z > minCorner.z + (maxCorner.z - minCorner.z) / 2) {
					//we are in the maxz side
					if (n.x > minCorner.x + (maxCorner.x - minCorner.x) / 2) {
						//we are in the maxx side
						m.x = -1;
					} else {
						//we are minx side
						m.z = -1;
					}
				} else {
					//we are in the minz side
					if (n.x > minCorner.x + (maxCorner.x - minCorner.x) / 2) {
						//we are in the maxx side
						m.z = 1;
					} else {
						//we are minx side
						m.x = 1;
					}
				}
			}
		}

		//rotate back
		if(rotateBox){
			m=rotateAroundVector(m, rotationAxis, -1*angle, "degrees")*l1;
		}else{
			//growthrate should not be effected by this
			normalizeVector(m);
			m *= l1;
		}

		//store new rate and return true to redo the integration
		if (vectorlength(pit->second.rate, m) > 0.00001) {
			pit->second.rate = m;
			lit->second.rate = m;
			r = true;
		}
	}

	//Collision detection
//	bool collisionDetection(false);
	//if collision detection and r = false
/*	if(collisionDetection & !r){
		//get the current segment
		Coordinate n(b), m(b), mid;
		n += lit->second; //growthpoint
		m += pit->second; //last position of the growth point
		mid= n+ (m-n)/2;
		Time t(pit->first);

		//get list of neighboring root nodes, 0.8 cm away
		//note this list does not include growthpoints, but does include the node behind the growthpoint and so all segments are considered
		//TODO radius is hard coded, but really depends on the length of the segments around
//		Database::Positions list;
//		pSD->getChildren()->getPositionsWithinRadius(t,mid,0.8,list);

		//get list of segments, with diameters
//		Coordinate pos1, pos2, pos3;
//		SimulaBase* p;
//		for(Database::Positions::iterator it(list.begin()); it!=list.end() ; ++ it ){
//			p=it->second->getParent()->getPrevious(t,it->second->getName());
//			if(p) p->getAbsolute(t,pos1);
//			pos2 = it->first;
			//test intersection

			//get next point
//			p=it->second->getParent()->getNext(t,it->second->getName());
//			if(!p) p=it->second->getParent(2)->get("growthpoint");
//			p->getAbsolute(t,pos3);
			//test intersection again

//		}
		//check if there is any overlap
		//rotate growth point such that overlap does not occur.
	}*/
	return r;
}


std::string RootGrowthDirection::getName() const {
	return "rootGrowthDirection";
}


RandomImpedence::RandomImpedence(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//plant type
	std::string plantType;
	PLANTTYPE(plantType, pSD);
	//get the root type
	std::string type;
	pSD->getParent(2)->getChild("rootType")->get(type);
	//weightfactor simulator
	rootTypeSpecificWeightFactorSimulator =
			ORIGIN->getChild("rootTypeParameters")->getChild(
					plantType)->getChild(type)->getChild(
					"soilImpedence");
	//Unit to correct for (default is hours/day) ///@todo could be derived from the unit of the weight factor
	defaultTimeUnit = 24;
}

void RandomImpedence::calculate(Time const &t, Coordinate &direction) {
	//weight factor for impedence
	double weight;
	rootTypeSpecificWeightFactorSimulator->get(t, weight);
	//correction factor for time step is 1/timestep.
	Time timeStep = pSD->getParent()->lastTimeStep();
	//random factor for gravitropism (originally timestep was 1 hour which is 1/24 day)
	for (int i(0); i < (timeStep * defaultTimeUnit); i++) {
		direction.x += weight
				* (((double) rand() / (double) RAND_MAX) - 0.5);
		direction.y += weight
				* (((double) rand() / (double) RAND_MAX) - 0.5);
		direction.z += weight
				* (((double) rand() / (double) RAND_MAX) - 0.5);
	}
	//add random factor for 'left over time'
	direction.x +=
			weight * (((double) rand() / (double) RAND_MAX) - 0.5)
					* ((timeStep * defaultTimeUnit)
							- floor(timeStep * defaultTimeUnit));
	direction.y +=
			weight * (((double) rand() / (double) RAND_MAX) - 0.5)
					* ((timeStep * defaultTimeUnit)
							- floor(timeStep * defaultTimeUnit));
	direction.z +=
			weight * (((double) rand() / (double) RAND_MAX) - 0.5)
					* ((timeStep * defaultTimeUnit)
							- floor(timeStep * defaultTimeUnit));
}

std::string RandomImpedence::getName() const {
	return "randomImpedence";
}


RandomGravitropism::RandomGravitropism(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//get the root type
	std::string type;
	pSD->getParent(2)->getChild("rootType")->get(type);
	//plant type
	std::string plantType;
	PLANTTYPE(plantType, pSD);
	rootTypeSpecificWeighFactorSimulator =
			ORIGIN->getChild("rootTypeParameters")->getChild(
					plantType)->getChild(type)->getChild(
					"gravitropism");
	//Unit to correct for (default is hours/day) ///@todo could be derived from the unit of the weight factor
	defaultTimeUnit = 24;
}

void RandomGravitropism::calculate(const Time &t, Coordinate &direction) {
	//weight factor for gravitropism
	double gravitropism;
	rootTypeSpecificWeighFactorSimulator->get(t, gravitropism);
	//correction factor for time step is 1/timestep.
	Time timeStep = pSD->getParent()->lastTimeStep();
	//random factor for gravitropism (originally timestep was 1 hour)
	double random(0);
	for (int i(0); i < (timeStep * 24); i++) {
		random += gravitropism * ((double) rand() / (double) RAND_MAX);
	}
	//left over time
	random +=
			gravitropism * (((double) rand() / (double) RAND_MAX))
					* ((timeStep * defaultTimeUnit)
							- floor(timeStep * defaultTimeUnit));
	//gravitropism
	direction.y -= random;
}

std::string RandomGravitropism::getName() const {
	return "randomGravitropism";
}



//general tropisms like hydrotropisms, nutrienttropisms etc.
Tropisms::Tropisms(SimulaDynamic* pSD) :
		DerivativeBase(pSD) {
	//get parameter section for this root type
	//get list of environmental factor this root type responses to
	//get list of pointers for those factors
	//get list of pointers for the impact of those factors
	//aggregation function (probably just add up?) note that the impact only influences how strongly the gradients changes the direction relative to something else.
	msg::warning("You are using the Tropisms class, which actually is purely experimental and may not do what you expect it to do");
}

void Tropisms::calculate(const Time &t, Coordinate &direction) {
	//get current location
	//for every environmental factor in the list
	//get  gradient - note that these gradients should be based on what the plant may experience. Probably only differences at both sides of the root, or along the root axis
	//determine direction of vector
	//determine magnitude of vector
	//return
	direction.x=  0;
	direction.y= -1;
	direction.z=  0;
}

std::string Tropisms::getName() const {
	return "tropisms";
}



//==================registration of the classes=================
DerivativeBase * newInstantiationRootGrowthDirection(SimulaDynamic* const pSD) {
	return new RootGrowthDirection(pSD);
}

DerivativeBase * newInstantiationRandomImpedence(SimulaDynamic* const pSD) {
	return new RandomImpedence(pSD);
}

DerivativeBase * newInstantiationRandomGravitropism(SimulaDynamic* const pSD) {
	return new RandomGravitropism(pSD);
}

DerivativeBase * newInstantiationTropisms(SimulaDynamic* const pSD) {
	return new Tropisms(pSD);
}


//registration of classes
class AutoRegisterDerivativeBaseInstantiationFunctions {
public:
	AutoRegisterDerivativeBaseInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["rootGrowthDirection"] =
				newInstantiationRootGrowthDirection;
		BaseClassesMap::getDerivativeBaseClasses()["randomImpedence"] =
				newInstantiationRandomImpedence;
		BaseClassesMap::getDerivativeBaseClasses()["randomGravitropism"] =
				newInstantiationRandomGravitropism;
		BaseClassesMap::getDerivativeBaseClasses()["tropisms"] =
				newInstantiationTropisms;
	}
};

static AutoRegisterDerivativeBaseInstantiationFunctions p;
