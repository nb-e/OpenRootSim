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
#include "VaporPressure.hpp"
#include "../../cli/Messages.hpp"

#include <math.h>


/// Arden Buck equation
#define	 a    6.1121
#define	 b   18.678
#define	 c  257.14
#define  d  234.5
#define rangeLow1  -80.
#define rangeHigh1  50.
///  Sonntag1990; SHTxx Humidity & Temperature Sensmitter, Application Note Dew-point Calculation (Err < -0.35°C)
#define aa   6.112
#define bb  17.62
#define cc 243.12
#define rangeLow2  -45.
#define rangeHigh2  60.
		/// Other approaches for these parameters:
		///
		/// See: Barenbrug, A.W.T., Psychrometry and Psychrometric Charts, 3rd Edition, Cape Town, S.A.: Cape and Transvaal Printers Ltd., 1974
		/// http://www.paroscientific.com/dewpoint.htm
		///
		/// NOAA: a = 6.112, b = 17.67,c= 243.5
		/// These valuations provide a minimum accuracy of 0.1%, for -30 C <= T <= 35 C.
		/// http://www.srh.noaa.gov/images/epz/wxcalc/rhTdFromWetBulb.pdf
		/// taken from a 1980 paper by David Bolton; "The computation of equivalent potential temperature", Monthly Weather Review, vol.108, pg.1047, Eq.10
		///
		/// Also, in the Journal of Applied Meteorology and Climatology,
		/// Arden Buck presents several different valuation sets,
		/// with different minimum accuracies for different temperature ranges.
		/// Two particular sets provide a range of -40°C → +50°C between the two,
		/// with even greater minimum accuracy than all of the other, above sets (maximum error at given |C°| extreme):
		///
		/// a = 6.1121 mb.; b = 17.368; c = 238.88 °C;  Range:   0 C <= T <= 50; (Err <= 0.05%)
		/// a = 6.1121 mb.; b = 17.966; c = 247.15 °C;  Range: -40 C <= T <= 0;  (Err <= 0.06%)
		///


/************************ActualVaporPressure*********************************/

ActualVaporPressure::ActualVaporPressure(SimulaDynamic* const pSV) : // Prototyp
		DerivativeBase(pSV) // Initialisierung von DerivativeBase
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("hPa");
	//dailyTemperature=ORIGIN->getChild("environment")->getChild("atmosphere")->getChild("averageDailyTemperature","degreesC");
	dailyTemperature_ 		= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");
	dewpointtemperature_ 	= pSD->existingPath("/environment/atmosphere/dewPointTemperature"); // , "degreesC"
	relativeHumidity_ 		= pSD->existingPath("/environment/atmosphere/relativeHumidity"); //, "percent100"

	minTemperature_ 		= pSD->existingPath("/environment/atmosphere/minTemperature", "degreesC");
	maxTemperature_ 		= pSD->existingPath("/environment/atmosphere/maxTemperature", "degreesC");
	minrelativeHumidity_	= pSD->existingPath("/environment/atmosphere/minrelativeHumidity", "degreesC");
	maxrelativeHumidity_	= pSD->existingPath("/environment/atmosphere/maxrelativeHumidity", "degreesC");

	if (!(relativeHumidity_ || dewpointtemperature_)){
		msg::error("ActualVaporPressure: No relative humidity or dew point temperature supplied.");
	}
}

std::string ActualVaporPressure::getName() const {
	return "e_a";
}

DerivativeBase * newInstantiationActualVaporPressure(SimulaDynamic* const pSD) {
	return new ActualVaporPressure(pSD);
}

/****************************************************************************/
void ActualVaporPressure::calculate(const Time &t, double& e_a){
/****************************************************************************/
	double temperature;
	dailyTemperature_->get(t, temperature);

//	if (minTemperature_ && maxTemperature_) {
//		double tempmin, tempmax;
//		minTemperature_->get(t, tempmin);
//		maxTemperature_->get(t, tempmax);
//		if (fabs(0.5 * (tempmin + tempmax) - temperature) > 0.001) {
//			msg::error(
//					"ActualVaporPressure: minTemperature and maxTemperature are set but do not fit to the averageDailyTemperature.");
//		}
//	}

	double RH;
    double T_dewpoint;
    double e_s;

    /// Just a plausibility Check:
	if(relativeHumidity_ && dewpointtemperature_){
		relativeHumidity_->get(t,RH);
		dewpointtemperature_->get(t, T_dewpoint);
		double RH_calc = 5. * (T_dewpoint - temperature) + 100.; // 1% accuracy
		if (fabs(1.- RH/RH_calc) > 0.02) {
		msg::warning("ActualVaporPressure: Plausibility check: Misfit values in relative humidity and dew point --> Using supplied RH "
			"With the supplied dew point, the supplied relative humidity has a >2% difference to the implicitly relative humidity computed with a very simplified formula");
		}
	}


	/// derive e_a from dew point
	if (dewpointtemperature_) {
		dewpointtemperature_->get(t, T_dewpoint);
		if (T_dewpoint >= rangeLow1 && T_dewpoint <= rangeHigh1) {
			e_a = a
					* exp(
							(b - T_dewpoint / d)
									* (T_dewpoint / (c + T_dewpoint)));
		} else if (T_dewpoint >= rangeLow2 && T_dewpoint <= rangeHigh2) {
			e_a = aa * exp(bb * T_dewpoint / (cc + T_dewpoint));
		} else {
			msg::error(
					"ActualVaporPressure: Dew point temperature out of Range.");
		}
	}
	/// derive e_a from RH
	else {
		if(minTemperature_ && maxrelativeHumidity_ && !minrelativeHumidity_){
	    /// When using equipment where errors in estimating RHmin can be large,
		/// or when RH data integrity are in doubt, then one should use only RHmax
			double RHmax;
			maxrelativeHumidity_->get(t, RHmax);
			if (RHmax < 1. || RHmax > 100.) {
				msg::error(
						"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
			}
			double Tmin;
			minTemperature_->get(t, Tmin);
			double e_s_min;
			if (Tmin <= rangeHigh1 && Tmin >= rangeLow1) {
				e_s_min = a * exp((b - Tmin / d) * (Tmin / (c + Tmin)));
				e_a = e_s_min * RHmax * 0.01;
			} else if (Tmin <= rangeHigh2 && Tmin >= rangeLow2) {
				e_s_min = aa * exp(bb * Tmin / (cc + Tmin));
				e_a = e_s_min * RHmax * 0.01;
			}
			else {
				msg::error("VaporPressure: Temperature out of Range.");
			}
		}
		else if (minTemperature_ && maxTemperature_) {
			double Tmin, Tmax;
			minTemperature_->get(t, Tmin);
			maxTemperature_->get(t, Tmax);
			double e_s_min, e_s_max;

			if (Tmax <= rangeHigh1 && Tmin >= rangeLow1) {
				e_s_min = a * exp((b - Tmin / d) * (Tmin / (c + Tmin)));
				e_s_max = a * exp((b - Tmax / d) * (Tmax / (c + Tmax)));
				e_s = 0.5 * (e_s_min + e_s_max);
				if (maxrelativeHumidity_ && minrelativeHumidity_) {
					double RHmin, RHmax;
					minrelativeHumidity_->get(t, RHmin);
					maxrelativeHumidity_->get(t, RHmax);
					if (RHmin < 1. || RHmax > 100.) {
						msg::error(
								"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
					}
					e_a = 0.5
							* (RHmin * 0.01 * e_s_max + RHmax * 0.01 * e_s_min);
				} else if (relativeHumidity_) {
					relativeHumidity_->get(t, RH);
					/// Range check
					if (RH < 1. || RH > 100.) {
						msg::error(
								"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
					}
					e_a = RH * 0.01 * e_s;
				}
			} /// the range fits only for non modified formula
			else if (Tmax <= rangeHigh2 && Tmin >= rangeLow2) {
				e_s_min = aa * exp(bb * Tmin / (cc + Tmin));
				e_s_max = aa * exp(bb * Tmax / (cc + Tmax));
				e_s = 0.5 * (e_s_min + e_s_max);
				if (maxrelativeHumidity_ && minrelativeHumidity_) {
					double RHmin, RHmax;
					minrelativeHumidity_->get(t, RHmin);
					maxrelativeHumidity_->get(t, RHmax);
					if (RHmin < 1. || RHmax > 100.) {
						msg::error(
								"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
					}
					e_a = 0.5
							* (RHmin * 0.01 * e_s_min + RHmax * 0.01 * e_s_max);
				} else if (relativeHumidity_) {
					relativeHumidity_->get(t, RH);
					/// Range check
					if (RH < 1. || RH > 100.) {
						msg::error(
								"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
					}
					e_a = RH * 0.01 * e_s;
				}
			} else {
				msg::error("VaporPressure: Temperature out of Range.");
			}
			/**************************************************************/
		} else { /// only an average daily temperature is given
			if (temperature <= rangeHigh1 && temperature >= rangeLow1) {
				if (relativeHumidity_) {
					relativeHumidity_->get(t, RH);
					/// Range check
					if (RH < 1. || RH > 100.) {
						msg::error(
								"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
					}
					e_s =
							a
									* exp(
											(b - temperature / d)
													* (temperature
															/ (c + temperature)));
					e_a = RH * 0.01 * e_s;
				}
			} /// non modified formula
			else if (temperature <= rangeHigh2 && temperature >= rangeLow2) {
				if (relativeHumidity_) {
					relativeHumidity_->get(t, RH);
					/// Range check
					if (RH < 1. || RH > 100.) {
						msg::error(
								"ActualVaporPressure: Range check: Invalid relative humidity provided. Range: 1% <= RH <= 100%");
					}
					e_s = aa * exp(bb * temperature / (cc + temperature));
					e_a = RH * 0.01 * e_s;

//// THE SAME BUT DIRECTLY CALCULATED WITH DEW POINT
//			double g = log(RH*0.01)+((b*temperature)/(c+temperature));
//			T_dewpoint = c*g /(b-g);
//			std::cout<< "Magnus formula for T_dew = " << T_dewpoint << std::endl;
//
//			e_a = formula(T_dewpoint, a, b, c);
//			std::cout << "actual vapor pressure in mb, e_a = " << e_a << std::endl;
///////////////////////////////////////////////////////////////////////////7

				}
			} else {
				msg::error("VaporPressure: Temperature out of Range.");
			}
		}
	}
//	std::cout << "actual vapor pressure in mb, e_a = " << e_a << std::endl;

//// OLD CODE //////////////////////////////////////////
	/// For situations where simplicity is desirable and slightly less accuracy is acceptable,
	/// the following equation offers good results, especially at the higher ambient air temperatures
	/// where the saturation pressure becomes significant for the density altitude calculations.

	// double fraction_value = 237.3 + temperature; // this should be air temperature
	/// Tetens 1930 approximation
	/// Valid for: 0 ºC < T_air < 60 ºC , 1% < RH < 100%,  0 ºC < T_dewpoint< 50 ºC
	// e_s = 6.1078 * exp(17.2694 * temperature / fraction_value); ///< saturation vapor pressure of water vapor, mb
	/// note that 6.1078 equals the vapor pressure at the dew point of water

	/// alternative e_s below is the same as above:
	// double s	= (7.5*temperature)/(const_3);
	// double e_s_alternative  = const_0*pow(10.,s);
/////////////////////////////////////////////////////////////////////////

//// OLD CODE//////////////////////////////////////////////////////////
	//e_a = 6.1078 * exp(17.2694 * T_dewpoint / fraction_value); ///< mb.

	// alternative e_a below is the same as above:
	// Sd  = ( 7.5 * dewpoint_temperature ) / ( 237.3 + dewpoint_temperature );
	// e_a =6.1078 * pow(10.,Sd);

	// RH = 100.* e_a/e_s; // in %
	/* Relative humidity is the amount of moisture in the air compared to what the air can "hold" at that temperature.
	 * When the air can't "hold" all the moisture, then it condenses as dew.
	 */
////////////////////////////////////////////////////////////////

} // end calc.




/************************SaturatedVaporPressure*********************************/

SaturatedVaporPressure::SaturatedVaporPressure(SimulaDynamic* const pSV) : // Prototyp
		DerivativeBase(pSV) // Initialisierung von DerivativeBase
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("hPa");
	//dailyTemperature=ORIGIN->getChild("environment")->getChild("atmosphere")->getChild("averageDailyTemperature","degreesC");
	dailyTemperature_ 		= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");
	minTemperature_ 		= pSD->existingPath("/environment/atmosphere/minTemperature", "degreesC");
	maxTemperature_ 		= pSD->existingPath("/environment/atmosphere/maxTemperature", "degreesC");
}

std::string SaturatedVaporPressure::getName() const {
	return "e_s";
}

DerivativeBase * newInstantiationSaturatedVaporPressure(SimulaDynamic* const pSD) {
	return new SaturatedVaporPressure(pSD);
}

/****************************************************************************/
void SaturatedVaporPressure::calculate(const Time &t, double& e_s){
/****************************************************************************/
	double temperature;
	dailyTemperature_->get(t, temperature);

	if (minTemperature_ && maxTemperature_) {
		double Tmin, Tmax;
		minTemperature_->get(t, Tmin);
		maxTemperature_->get(t, Tmax);
		double e_s_min, e_s_max;
		if (Tmax <= rangeHigh1 && Tmin >= rangeLow1) {
			e_s_min = a* exp((b-Tmin/d)*(Tmin /(c+Tmin)));
			e_s_max = a* exp((b-Tmax/d)*(Tmax /(c+Tmax)));
			e_s     = 0.5*(e_s_min + e_s_max);
		} else if (Tmax <= rangeHigh2 && Tmin >= rangeLow2) {
			e_s_min = aa* exp(bb*Tmin / (cc+Tmin));
			e_s_max = aa* exp(bb*Tmax / (cc+Tmax));
			e_s     = 0.5*(e_s_min + e_s_max);
		}
		else {
			msg::error("VaporPressure: Temperature out of Range.");
		}
	}
	else { /// if min and max temperature are not available
		if (temperature <= rangeHigh1 && temperature >= rangeLow1) {
			e_s = a* exp((b-temperature/d)*(temperature / (c+temperature)));

		} else if (temperature <= rangeHigh2 && temperature >= rangeLow2) {
			e_s = aa* exp(bb*temperature / (cc+temperature));

		} else {
			msg::error("VaporPressure: Temperature out of Range.");
		}
	}
} // end calc. sat. vap. press.



/************************SlopeVaporPressure*********************************/

SlopeVaporPressure::SlopeVaporPressure(SimulaDynamic* const pSV) : // Prototyp
		DerivativeBase(pSV) // Initialisierung von DerivativeBase
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("hPa/degreesC");
	//dailyTemperature=ORIGIN->getChild("environment")->getChild("atmosphere")->getChild("averageDailyTemperature","degreesC");
	dailyTemperature_ 		= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");

}

std::string SlopeVaporPressure::getName() const {
	return "delta_e_s";
}

DerivativeBase * newInstantiationSlopeVaporPressure(SimulaDynamic* const pSD) {
	return new SlopeVaporPressure(pSD);
}

/****************************************************************************/
void SlopeVaporPressure::calculate(const Time &t, double& delta_e_s){
/****************************************************************************/
	double temperature;
	double e_s;

	dailyTemperature_->get(t, temperature);

		if (temperature <= rangeHigh1 && temperature >= rangeLow1) {
			e_s = a* exp((b-temperature/d)*(temperature /(c+temperature)));
			double q = c+temperature;
			/// --- gradient of saturation vapor pressure, derivative of e_s ---
			delta_e_s = e_s* (b * c * d - temperature*(2.*c+temperature)) / (d*q*q);
		} else if (temperature <= rangeHigh2 && temperature >= rangeLow2) {
			e_s = aa* exp(bb*temperature / (cc+temperature));
			double q = cc+temperature;
			delta_e_s = e_s* bb * cc / (q*q);
		}
		else {
			msg::error("VaporPressure: Temperature out of Range.");
		}

} // end calc. sat. slope



//==================registration of the classes=================
class AutoRegisterVaporPressureClassInstantiationFunctions {
public:
	AutoRegisterVaporPressureClassInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["e_a"] = newInstantiationActualVaporPressure;
		BaseClassesMap::getDerivativeBaseClasses()["e_s"] = newInstantiationSaturatedVaporPressure;
		BaseClassesMap::getDerivativeBaseClasses()["delta_e_s"] = newInstantiationSlopeVaporPressure;
	}
};

// our one instance of the proxy
static AutoRegisterVaporPressureClassInstantiationFunctions p45561000745465435;
