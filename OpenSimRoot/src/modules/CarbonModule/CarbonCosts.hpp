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

#ifndef EXUDATES_HPP_
#define EXUDATES_HPP_
/* it's hard to know what exudates do to nutrient uptake, but we can estimate
 * the costs of exudates and have them as part of the carbon model.
 * Cost of exudates should be related to the size of the root system.
 * JPL want's it root class specific for future development.
 *
 */
/**
 * The carbon cost of exudates is calculated simply on a per root length basis.
 *
 * Notes:
 * 1) The intention is to simply include a carbon cost of exudates, without
 * considering the function of exudates.
 * 2) Label as rootSegmentCarbonCostOfExudates
 *
 * Location in Database:
 * Should be part of a root node (datapoint)
 *
 *
 */
#include "../../engine/BaseClasses.hpp"
#include <vector>
#include "../GenericModules/Totals.hpp"
/**
 * Calculation of the carbon cost of exudates by a rootsegment.
 *
 * Notes:
 *
 * 1) The size of the root segment is multiplied
 * with a relative cost of exudates which should be present in the
 * root parameter section. The relative cost parameter
 * can be relative to length, dry weight or volume of the root
 * segment. The modules switches based on the unit of the relative cost parameter
 * which should be g/cm/day or g/g/day or g/cm3/day.
 *
 * 2) If at the top of the plant a multiplication factor is present,
 * the multiplication factor will be used. The multiplication factor
 * typically is a function of the plants nutrient status.
 * See carbonCostOfExudationMultiplicationFactor.
 *
 * Location in Database:
 * This should be a child of a root node (datapoint).
 *
 *
 */
class CarbonCostOfExudates:public TotalBase{
public:
	CarbonCostOfExudates(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *param, *size, *multiplier;
};

class CarbonCostOfBiologicalNitrogenFixation:public DerivativeBase{
public:
	CarbonCostOfBiologicalNitrogenFixation(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *factor, *fixation;
};

class CarbonCostOfNutrientUptake:public DerivativeBase{
public:
	CarbonCostOfNutrientUptake(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *factor, *uptake, *max;
};

class SumCarbonCosts:public DerivativeBase{
public:
	SumCarbonCosts(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	std::vector<SimulaBase*> list;
	bool rates;
};

#endif /* EXUDATES_HPP_ */
