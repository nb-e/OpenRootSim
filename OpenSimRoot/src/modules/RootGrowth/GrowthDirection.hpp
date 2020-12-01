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
#ifndef DerivativeFUNCTIONLIBRARY_HPP_
#define DerivativeFUNCTIONLIBRARY_HPP_


#include "../../engine/BaseClasses.hpp"

class RootGrowthDirection:public DerivativeBase{
public:
	RootGrowthDirection(SimulaDynamic* pSD);
	bool postIntegrationCorrection(SimulaPoint::Table &table);
	std::string getName()const;
protected:
	void calculate(const Time &t,Coordinate &var);
	inline void setInitialDirection(const std::string &plantType);
	inline void getRadialBranchingAngle(double & alpha, Unit &u, const std::string &plantType);
	inline void getLateralBranchingAngle(double & alpha, Unit &u, const std::string &plantType);
	SimulaPoint *parent;
	SimulaBase *impedenceSimulator, *gravitropismSimulator, *growthSimulator, *tropismSimulator;
	Coordinate idirection;
	bool topBoundary, bottomBoundary, bounceOfSide, cannotgrowup;
	static Coordinate minCorner, maxCorner, rotationCenter, rotationAxis;
	static double angle;
	static bool rotateBox, roundPot;
	Time refTimeI, refTimeG;
	int steps_;
	static double timeScalingFactor;
};
class RandomImpedence:public DerivativeBase{
public:
	RandomImpedence(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,Coordinate &direction);
	SimulaBase *rootTypeSpecificWeightFactorSimulator;
	Time defaultTimeUnit;
};
class RandomGravitropism:public DerivativeBase{
public:
	RandomGravitropism(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,Coordinate &direction);
	SimulaBase *rootTypeSpecificWeighFactorSimulator;
    Time defaultTimeUnit;
};
class Tropisms:public DerivativeBase{
public:
	Tropisms(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,Coordinate &direction);
};
#endif /*DerivativeFUNCTIONLIBRARY_HPP_*/
