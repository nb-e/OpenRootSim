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

#ifndef SOILTEMPERATURE_HPP__
#define SOILTEMPERATURE_HPP__
#include "../../engine/BaseClasses.hpp"
#include "../../engine/Origin.hpp"

/************************************************************************//**
 * \class SimpleSoilTemperature
 * Input: soil depth z, Top-soil Temperature, time
 *
 * Output: soilTemperature(z,t), Annual computation, so its an average soil temperature profile for a day
 *
 * Information:
 * 	If
 * 	theta_solid_, theta_organic_matter_, theta_quartz_, theta_other_minerals_, theta_clay_, beta_t_
 * 	are not given, then water is decoupled and we
 * 	assume that the volumetric heat capacity and the thermal conductivity coefficient
 *  are both constant with depth and water movement in the soil is ignored.
 *	We assume that the soil surface temperature varies sinusoidally around an average value "avSurfTemp"
 *	with a daily (or annual) amplitude A. ---> For formulas see code in .cpp-file or Literature.
 *
 *	Lit.: Park S. Nobel, Physicochemical and Environmental Plant Physiology. 2005.
 *		  Chapter 7, Section Soil. Page 355 ff.
 *
****************************************************************************/
class SimpleSoilTemperature: public DerivativeBase {
public:
	SimpleSoilTemperature(SimulaDynamic* const pSV);
	std::string getName() const;

protected:
	void calculate(const Time &t, const Coordinate &pos, double& soilTemp);
	SimulaBase *maxSurfaceTemperature_, *minSurfaceTemperature_, *yearlyMaxSurfaceTemperature_, *yearlyMinSurfaceTemperature_,
	*hottestDayOfYear_,*hottest_time_of_the_day_,
	*thermalConductivity_, *volumetricHeatCapacity_;
};

/************************************************************************//**
 * \class VolumetricHeatCapacity
 * Input: Theta
 * Output: Cp
****************************************************************************/
class VolumetricHeatCapacity: public DerivativeBase {
public:
	VolumetricHeatCapacity(SimulaDynamic* const pSV);

	std::string getName() const;

protected:
	void calculate(const Time &t,const Coordinate &pos, double& Cp);
	SimulaBase * theta_;
	SimulaBase *theta_organic_matter_,*theta_quartz_, *theta_other_minerals_, *volumetricHeatCapacity_;
};

/************************************************************************//**
 * \class Apparent ThermalConductivity
 * Input: Theta
 * Output: lambda
****************************************************************************/
class ThermalConductivity: public DerivativeBase {
public:
	ThermalConductivity(SimulaDynamic* const pSV);

	std::string getName() const;

protected:
	void calculate(const Time &t,const Coordinate &pos, double& lambda);
	SimulaBase * theta_, *Q_;
	SimulaBase *theta_quartz_, *theta_other_minerals_, *theta_clay_, *beta_t_ ,*thermalConductivity_;
};

#endif
