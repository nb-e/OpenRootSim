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
#ifndef StressAndPlasticity_HPP_
#define StressAndPlasticity_HPP_

#include "../../engine/BaseClasses.hpp"

class NutrientStressFactor: public DerivativeBase {
public:
	NutrientStressFactor(SimulaDynamic* pSD);
	std::string getName() const;
protected:
	void calculate(const Time &t, double &var);
	SimulaBase *uptake, *target, *minimum;
};
class NutrientStressFactorV2: public DerivativeBase {
public:
	NutrientStressFactorV2(SimulaDynamic* pSD);
	std::string getName() const;
	bool postIntegrationCorrection(SimulaVariable::Table & data);
protected:
	void calculate(const Time &t, double &var);
	SimulaBase *uptake, *fixation, *target, *minimum;
	bool corrected;
};

class StressFactor: public DerivativeBase {
public:
	StressFactor(SimulaDynamic* pSD);
	std::string getName() const;
protected:
	void calculate(const Time &t, double &var);
	SimulaBase::List nutrients;
	SimulaBase::List impactFactors;
};

class LocalNutrientResponse: public DerivativeBase {
public:
	LocalNutrientResponse(SimulaDynamic* pSD);
	std::string getName() const;
protected:
	void calculate(const Time &t, const Coordinate &pos, double &var);
	void calculate(const Time &t, double &var);
	SimulaBase::List nutrients;
	SimulaBase::List impactFactors;
	SimulaBase::List nutrientsMod;
	SimulaBase::List impactFactorsMod;

	double (*aggregationFunction)(std::vector<double> &d);
	SimulaBase *position;
};

class BFMmemory: public DerivativeBase {
public:
	BFMmemory(SimulaDynamic* pSD);
	std::string getName() const;
protected:
	void calculate(const Time &t, double &var);
	double mem;
};

#endif

