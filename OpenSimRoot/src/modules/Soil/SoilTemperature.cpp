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

#if _WIN32 || _WIN64
#define _USE_MATH_DEFINES
#endif
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#include "SoilTemperature.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/Time.hpp"
#include <math.h>

/************************ThermalConductivity*********************************/

ThermalConductivity::ThermalConductivity(SimulaDynamic* const pSV) : // Prototyp
		DerivativeBase(pSV), // Initialisierung von DerivativeBase
		theta_(nullptr), Q_(nullptr), theta_quartz_(nullptr), theta_other_minerals_(nullptr), theta_clay_(nullptr), beta_t_(nullptr), thermalConductivity_(
				nullptr) {
	theta_quartz_ = pSD->existingPath("/environment/soil/theta_quartz");
	if (theta_quartz_) {
		msg::warning("ThermalConductivity:: Simulating based on Campbell [1985]", 2);
		theta_other_minerals_ = pSD->getPath("/environment/soil/theta_other_minerals");
		theta_clay_ = pSD->getPath("/environment/soil/theta_clay");
		beta_t_ = pSD->getPath("/environment/soil/thermal_dispersivity");
		theta_ = pSD->getPath("/soil/theta"); //todo needs to be implemented
		Q_ = pSD->getPath("/soil/Q"); //todo needs to be implemented
	} else {
		thermalConductivity_ = pSD->existingPath("/environment/soil/ThermalConductivity");
		if (thermalConductivity_) {
			msg::warning("ThermalConductivity:: Using values from input file", 2);
		} else {
			msg::warning("ThermalConductivity:: Using default value of 0.6", 2);
		}
	}
}

std::string ThermalConductivity::getName() const {
	return "thermalConductivity";
}

DerivativeBase * newInstantiationThermalConductivity(SimulaDynamic* const pSD) {
	return new ThermalConductivity(pSD);
}

/****************************************************************************/
void ThermalConductivity::calculate(const Time &t, const Coordinate &pos, double& lambda) {
	/****************************************************************************/
	double theta_quartz, theta_other_minerals, theta_clay, beta_t;
	if (!theta_quartz_) {
		if (thermalConductivity_) {
			thermalConductivity_->get(t, lambda);
		} else {
			lambda = 0.6;
		}
	} else {

		theta_quartz_->get(t, theta_quartz);
		theta_other_minerals_->get(t, theta_other_minerals);
		theta_clay_->get(t, theta_clay);
		beta_t_->get(t, beta_t);
		double theta;
		theta_->get(t, pos, theta);
		double q;
		Q_->get(t, pos, q);

		const double theta_solid = theta_quartz + theta_other_minerals; // p.34 of G.S. Campbell, Soil physics with BASIC 1985

		// a function suggested by Campbell [1985], emperical parameters
		const double A = (0.57 + 1.73 * theta_quartz + 0.93 * theta_other_minerals) / (1. - 0.74 * theta_quartz - 0.49 * theta_other_minerals)
				- 2.8 * theta_solid * (1. - theta_solid);
		const double B = 2.8 * theta_solid;
		const double C = 1. + 2.6 * pow(theta_clay, -0.5);
		const double D = 0.03 + 0.7 * theta_solid * theta_solid;
		const double E = 4.0;

		const double lambda_0 = A + B * theta - (A - D) * exp(-pow(C * theta, E));

		// The apparent thermal conductivity λ(θ) combines the thermal conductivity λ0(θ) of the
		// porous medium (solid plus water) in the absence of flow and the macrodispersivity,
		// which is linear function of the velocity [de Marsily, 1986]:
		//    λ(θ) = λ0(θ) + β(t) Cw |q|
		// where β_t is the thermal dispersivity [L].
		// The volumetric heat capacity of the liquid phase is
		// included in the definition of the thermal conductivity in order to have the dimensions of the
		// thermal dispersivity in length units.

		// Cw are the volumetric heat capacities (water phase)
		const double Cw = 4.18 * 1.e6;

		lambda = lambda_0 + beta_t * Cw * fabs(q);
	}
	return;
} // end calc. ThermalConductivity

/************************VolumetricHeatCapacity*********************************/

VolumetricHeatCapacity::VolumetricHeatCapacity(SimulaDynamic* const pSV) : // Prototyp
		DerivativeBase(pSV), // Initialisierung von DerivativeBase
		theta_(nullptr), theta_organic_matter_(nullptr), theta_quartz_(nullptr), theta_other_minerals_(nullptr), volumetricHeatCapacity_(nullptr) {
	theta_quartz_ = pSD->existingPath("/environment/soil/theta_quartz");
	if (theta_quartz_) {
		msg::warning("VolumetricHeatCapacity:: Using Campbell [1985]",2);
		theta_ = pSD->getPath("/soil/theta");
		theta_organic_matter_ = pSD->getPath("/environment/soil/theta_organic_matter");
		theta_other_minerals_ = pSD->getPath("/environment/soil/theta_other_minerals");
	} else {
		volumetricHeatCapacity_ = pSD->existingPath("/environment/soil/VolumetricHeatCapacity");
		if (volumetricHeatCapacity_) {
			msg::warning("VolumetricHeatCapacity:: Using VolumetricHeatCapacity from parameter section",2);
		} else {
			msg::warning("VolumetricHeatCapacity:: Using default VolumetricHeatCapacity of 1e6*2.0",2);
		}

	}
}

std::string VolumetricHeatCapacity::getName() const {
	return "VolumetricHeatCapacity";
}

DerivativeBase * newInstantiationVolumetricHeatCapacity(SimulaDynamic* const pSD) {
	return new VolumetricHeatCapacity(pSD);
}

/****************************************************************************/
void VolumetricHeatCapacity::calculate(const Time &t, const Coordinate &pos, double& Cp) {
	/****************************************************************************/
	double theta_organic_matter, theta_quartz, theta_other_minerals;
	if (!theta_quartz_) {
		if (volumetricHeatCapacity_) {
			volumetricHeatCapacity_->get(t, Cp);
		} else {
			Cp = 2.e6;
		}
		return;
	} else {
		theta_quartz_->get(t, theta_quartz);
		theta_other_minerals_->get(t, theta_other_minerals);
		theta_organic_matter_->get(t, theta_organic_matter);
		double theta;
		theta_->get(t, pos, theta);

		const double theta_solid = theta_quartz + theta_other_minerals; // p.34 of G.S. Campbell, Soil physics with BASIC 1985

		// Cw are the volumetric heat capacities

		// According to de Vries [1963] the volumetric heat capacity is given by

		// Cp = Cn * theta_solid + Co * theta_organic_matter + Cw * theta + Ca av;
		Cp = (1.92 * theta_solid + 2.51 * theta_organic_matter + 4.18 * theta) * 1.e6;  //  ( J m^-3 °C^-1 )
	}
	return;
} // end calc. volumetric heat capacity

/***************************SimpleSoilTemperature*************************************/
SimpleSoilTemperature::SimpleSoilTemperature(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("degreesC");
	maxSurfaceTemperature_ = pSD->getPath("/environment/soil/maxSurfaceTemperature", "degreesC");
	minSurfaceTemperature_ = pSD->getPath("/environment/soil/minSurfaceTemperature", "degreesC");
	yearlyMaxSurfaceTemperature_ = pSD->getPath("/environment/soil/yearlyMaxSurfaceTemperature", "degreesC");
	yearlyMinSurfaceTemperature_ = pSD->getPath("/environment/soil/yearlyMinSurfaceTemperature", "degreesC");

	hottestDayOfYear_ = pSD->existingPath("/environment/soil/hottestDayOfYear", "day");
	if(!hottestDayOfYear_){
		msg::warning("SimpleSoilTemperature:: '/environment/soil/hottestDayOfYear' not found using 213.",2);
	}
	hottest_time_of_the_day_ = pSD->existingPath("/environment/soil/hottest_time_of_the_day");
	if (!hottest_time_of_the_day_){
		msg::warning("SimpleSoilTemperature:: '/environment/soil/hottest_time_of_the_day' not found, using 14:00 o clock.",2);
	}
	thermalConductivity_ = pSD->getSibling("ThermalConductivity");
	volumetricHeatCapacity_ = pSD->getSibling("VolumetricHeatCapacity");

}

std::string SimpleSoilTemperature::getName() const {
	return "SimpleSoilTemperature";
}

DerivativeBase * newInstantiationSimpleSoilTemperature(SimulaDynamic* const pSD) {
	return new SimpleSoilTemperature(pSD);
}

/***************************************************************************/
void SimpleSoilTemperature::calculate(const Time &t, const Coordinate &pos, double& soilTemp) {
	/****************************************************************************/
	double tmax(213.0);
	if (hottestDayOfYear_) {
		hottestDayOfYear_->get(tmax);
	}

	std::size_t actualDay = TimeConversion::dateToNumber(t);

	// Ksoil	thermal conductivity coefficient (depends on the water content)
	// CpSoil	volumetric heat capacity

	// getting z
	double z = -0.01 * pos.y; // y is negative down in SimRoot and in cm not meter
	// z is considered positive downwards
	if (z < 0.0) {
		msg::warning("SimpleSoilTemperature::calculate: We are virtually above ground, returning surface temperature.");
		z = 0;
	}

	// thermal conductivity Ksoil can vary from 0.2 for a dry soil to 2 J/s/m/degreesC for wet soil
	double Ksoil;
	thermalConductivity_->get(t, pos, Ksoil);
	if (Ksoil < 0.2 || Ksoil > 2.0) {
		msg::error(
				"SimpleSoilTemperature: 0.2 <= Ksoil <= 2.0 ? Ksoil = " + std::to_string(Ksoil) + "  J/s/m/degreesC. Check thermalConductivity calculation!");
	}

	// volumetric heat capacity CpSoil
	double CpSoil; // J/m³/degreesC
	volumetricHeatCapacity_->get(t, pos, CpSoil);

	//thermal diffusivity
	double DH = Ksoil / CpSoil; // m2/s

	double yearlyMaxSurfaceTemperature;
	double yearlyMinSurfaceTemperature;
	double yearlyAvSurfT;
	yearlyMaxSurfaceTemperature_->get(yearlyMaxSurfaceTemperature);
	yearlyMinSurfaceTemperature_->get(yearlyMinSurfaceTemperature);
	yearlyAvSurfT = 0.5 * (yearlyMaxSurfaceTemperature + yearlyMinSurfaceTemperature);

	double yearlyAmplitudeSurfT; // amplitude of daily variation in soil surface temperature about its mean for bare soil is often 15°C
	yearlyAmplitudeSurfT = (yearlyMaxSurfaceTemperature - yearlyMinSurfaceTemperature) * 0.5;

	// p : period (86400 for a daily variation, 365 annual)
	// p is the period (24 hours, or 8.64e4 s, for a daily variation and 365 times longer for an annual variation)
	double p = 365.; //days per year
	// d : damping depth
	double d = sqrt(p * 86400. * DH / M_PI); //m

	// every point should get a temperature depending on its depth z coordinate only
	soilTemp = yearlyAvSurfT + yearlyAmplitudeSurfT * exp(-z / d) * cos(2.0 * M_PI / p * (actualDay - tmax) - z / d);
	// double soilTempSurf = yearlyAvSurfT + yearlyAmplitudeSurfT *cos(2.0*PI/p*(actualDay-tmax) );

	//if(z==0)
	//msg::warning(" at day "+std::to_string(t)+" yearlyAvSurfT="+std::to_string(yearlyAvSurfT)+" z="+std::to_string(z));

	double dailyAmplitudeSurfT, dailyMaxSurfaceTemperature, dailyMinSurfaceTemperature;	// amplitude of daily variation in soil surface temperature about its mean for bare soil is often 15°C
	maxSurfaceTemperature_->get(t, dailyMaxSurfaceTemperature);
	minSurfaceTemperature_->get(t, dailyMinSurfaceTemperature);
	dailyAmplitudeSurfT = (dailyMaxSurfaceTemperature - dailyMinSurfaceTemperature) * 0.5;
	// double tdev=((dailyMaxSurfaceTemperature + dailyMinSurfaceTemperature)/2.)-soilTempSurf; // deviation idea

	p = 8.64e4; //seconds per day
	double hottest_time_of_the_day(14.0); //2 oclock hottest time of the day
	if (hottest_time_of_the_day_) {
		hottest_time_of_the_day_->get(t, hottest_time_of_the_day);
	}
	tmax = p * hottest_time_of_the_day / 24.; // divided by 24 only because SimRoot t is decimal 0-1 over a day
	// d : damping depth
	d = sqrt(p * DH / M_PI);
	soilTemp += exp(-z / d) * (dailyAmplitudeSurfT * cos(2.0 * M_PI * ((t - trunc(t)) - tmax / p) - z / d)/*+tdev*/);

}

//==================registration of the classes=================
class AutoRegisterSimpleSoilTemperatureClassInstantiationFunctions {
public:
	AutoRegisterSimpleSoilTemperatureClassInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["SimpleSoilTemperature"] = newInstantiationSimpleSoilTemperature;
		BaseClassesMap::getDerivativeBaseClasses()["VolumetricHeatCapacity"] = newInstantiationVolumetricHeatCapacity;
		BaseClassesMap::getDerivativeBaseClasses()["ThermalConductivity"] = newInstantiationThermalConductivity;
	}
};

// our one instance of the proxy
static AutoRegisterSimpleSoilTemperatureClassInstantiationFunctions p4510061256745435;

