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
#ifndef SEEDRESERVES_HPP_
#define SEEDRESERVES_HPP_

#include "../../engine/BaseClasses.hpp"

/**
 * Simulates the depletion of seed reserves. The depletion is
 * simulated using the reserveAllocationRate from the resource
 * section in the parameter section.
 *
 * Notes:
 * Function guarantees that reserves do not become negative.
 *
 * Database location:
 * Seed reserves are expected to be a child of a plant.
 *
 */
class Reserves:public DerivativeBase {
public:
	Reserves(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *rrSimulator;
	Time plantingTime;
};

/**
 * Simulates the depletion of seed reserves. The depletion is
 * simulated by meeting the potential growth rate of the plant.
 *
 * Depends on the simulation of photosynthesis, respiration and
 * potential carbon sink for growth.
 *
 * Notes:
 * 1) Class guarantees that reserves do not become negative.
 * 2) Class gives a warning when photosynthesis provides enough
 * C for potential growth while the seed reserves are not depleted.
 *
 * Database location:
 * Seed reserves are expected to be a child of a plant.
 *
 */
class ReservesSinkBased:public DerivativeBase {
public:
	ReservesSinkBased(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *sink, *photosynthesis, *respiration, *exudates, *CinDryWeight;
	Time mt;
	double mr;
};

/**
 * Represents carbon storage. Will be a carbon sink if available carbon
 * is more than is necessary for potential growth
 * and a carbon source if available carbon is less than potential growth.
 * Should never be negative.
 *
 * Database location: member of plant
 *
 * Depends on:
 * 	"plantPotentialCarbonSinkForGrowth", "plantCarbonIncome"
 * and optionally on "plantRespiration","rootCarbonCostOfExudates"
 *
 *
 */
class CarbonReserves:public DerivativeBase {
public:
	CarbonReserves(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *potentialGrowth, *available, *respiration, *exudates;
};



#endif /*SEEDRESERVES_HPP_*/
