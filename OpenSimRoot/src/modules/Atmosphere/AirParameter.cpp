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
#include "AirParameter.hpp"

#include <math.h>


/***************************AirPressure*************************************/
AirPressure::AirPressure(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
    pSD->checkUnit("Pa");
	dailyTemperature_ 			= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");
	measuredAirPressure_		= pSD->existingPath("/environment/atmosphere/measuredAirPressure");
	altitude_					= pSD->existingPath("/environment/atmosphere/altitude");
}
std::string AirPressure::getName() const {
	return "airPressure";
}

DerivativeBase * newInstantiationAirPressure(
		SimulaDynamic* const pSD) {
	return new AirPressure(pSD);
}

/***************************************************************************/
void AirPressure::calculate(const Time &t, double& P) {
/****************************************************************************/
	double temperature;
	dailyTemperature_->get(t,temperature);
	double Tk = temperature + 273.15; ///< get Kelvin from Celsius
	///default
	P = 101325; // 101325 Pa (1013.25 mb.= hPa) sea level standard pressure
		if(measuredAirPressure_){
			measuredAirPressure_->get(t,P);
		}
		else if (altitude_) {
			double z;
			altitude_->get(t, z);
			if(z != 2){
			const double a1= 0.0065; // constant lapse rate moist air = 0.0065 [K/m]
			const double exponent = 5.256; /// = g/(a1*Rd) // g = 9.807; // gravitational acceleration [m/s^2]
			P = P*pow((Tk-a1*(z))/Tk,exponent);
			/// Reference: Burman et al. (1987)
			}
		}
}


/***************************AirDensity*************************************/
AirDensity::AirDensity(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("kg/m3");
	dailyTemperature_ 			= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");
	actualVaporPressure_		= pSD->getSibling("actualVaporPressure");
	airPressure_				= pSD->getSibling("airPressure");
}
std::string AirDensity::getName() const {
	return "airDensity";
}

DerivativeBase * newInstantiationAirDensity(
		SimulaDynamic* const pSD) {
	return new AirDensity(pSD);
}

/***************************************************************************/
void AirDensity::calculate(const Time &t, double& rho) {
/****************************************************************************/
	double temperature;
	dailyTemperature_->get(t,temperature);
	double Tk = temperature + 273.15; ///< get Kelvin from Celsius

	double e_a;
	actualVaporPressure_->get(t,e_a);

	const double Rv = 461.4964; /// gas constant for water vapor, J/(kg*degK) = 461.495 = R/Mv
	const double Rd = 287.0531; /// gas constant for dry air, J/(kg*degK) 	= 287.05  = R/Md
	/*	   R  = universal gas constant = 8314.32 (in 1976 Standard Atmosphere)
	 Md = molecular weight of dry air 	= 28.964 gm/mol
	 Mv = molecular weight of water vapor = 18.016 gm/mol
	 */
	/// The pressure of water vapor is equal to the saturation pressure of water vapor at the dew point temperature
	double Pv = e_a * 100.; // at dewpoint // actual pressure of water vapor can be calculated by Pv =  100 * e_a in Pa

	/// total pressure, Absolute pressure is zero-referenced against a perfect vacuum, so it is equal to gauge pressure plus atmospheric pressure.
	double P = 101325;
	airPressure_->get(t,P);
	/// default
	//rho = 1.2; /// density of air (kg/m^3)
	double Pd = P - Pv; // in Pa, absolute or total air pressure P can be the measured air pressure from a weather report
	rho = (Pd / (Rd * Tk)) + (Pv / (Rv * Tk));
//	std::cout << "air pressure in kg/m³, rho = " << rho << std::endl;

	/// alternative: Stahl 1980: Feuchtigkeit und Trocknen in der pharmazeutischen Technologie
	// rho = 1.293 * 273.15/Tk *(1.-0.378*e_a/P)
}

/*****************************************************************************************************************************/
		/// --- specific heat capacity of air ---
SpecificHeatCapacityOfAir::SpecificHeatCapacityOfAir(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("J/kg/degreesC");
	dailyTemperature_ 			= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");
	airPressure_				= pSD->getSibling("airPressure");
}
std::string SpecificHeatCapacityOfAir::getName() const {
	return "specificHeatCapacityOfAir";
}

DerivativeBase * newInstantiationSpecificHeatCapacityOfAir(
		SimulaDynamic* const pSD) {
	return new SpecificHeatCapacityOfAir(pSD);
}

/***************************************************************************/
void SpecificHeatCapacityOfAir::calculate(const Time &t, double& c) {
/****************************************************************************/
	double temperature;
	dailyTemperature_->get(t,temperature);

	c = 1013.;///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
	double P;
    airPressure_->get(t,P);

	if(P >= 100000){ // At normal atmospheric pressure of 1.013 bar - the specific heat of dry air - cp - will vary with temperature
			c = 1006.;
		if(temperature < -50 || temperature >= 60){
			c = 1009.;
		}
	}
	else if(P > 709280){
		c = 1016.;
	}
	else{
		c = 1005.; // dry air at sea level
	}
}


//==================registration of the classes=================
class AutoRegisterAirParameterClassInstantiationFunctions {
public:
	AutoRegisterAirParameterClassInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["airDensity"] = newInstantiationAirDensity;
		BaseClassesMap::getDerivativeBaseClasses()["airPressure"] = newInstantiationAirPressure;
		BaseClassesMap::getDerivativeBaseClasses()["specificHeatCapacityOfAir"] = newInstantiationSpecificHeatCapacityOfAir;
	}
};

// our one instance of the proxy
static AutoRegisterAirParameterClassInstantiationFunctions p45100656745465435;

