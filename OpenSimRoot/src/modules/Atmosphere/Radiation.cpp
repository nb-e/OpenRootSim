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

#include "Radiation.hpp"
#include "../../cli/Messages.hpp"
#include <math.h>
#include "../../tools/Time.hpp"



/***************************Radiation************************************/

Radiation::Radiation(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("MJ/m2/day");

	dailyTemperature_   	= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");
	latitude_ 				= pSD->getPath("/environment/atmosphere/latitude", UnitRegistry::noUnit());
	actualDurationofSunshine_ = pSD->existingPath("/environment/atmosphere/actualDurationofSunshine");// , "hours"
	netSolarRadiation_ 		= pSD->existingPath("/environment/atmosphere/netSolarRadiation");

	minTemperature_ 		= pSD->existingPath("/environment/atmosphere/minTemperature", "degreesC");
	maxTemperature_ 		= pSD->existingPath("/environment/atmosphere/maxTemperature", "degreesC");
	actualVaporPressure_	= pSD->getSibling("actualVaporPressure"); //given in hPa

	std::string name=pSD->getName();
	if(name.find("Soil")!=std::string::npos){
		albedo_ = pSD->getPath("/environment/atmosphere/albedoSoil");
	}else{
		albedo_ = pSD->getPath("/environment/atmosphere/albedoCrop");
	}

	angstrom_as_  = pSD->existingPath("/environment/atmosphere/angstrom_as");
	angstrom_bs_  = pSD->existingPath("/environment/atmosphere/angstrom_bs");
}





std::string Radiation::getName() const {
	return "Radiation";
}

DerivativeBase * newInstantiationRadiation(
		SimulaDynamic* const pSD) {
	return new Radiation(pSD);
}

/*****************************************************************************/
void Radiation::calculate(const Time &t, double& netRadiation) {
/*****************************************************************************/
	/// Input:  Time, latitude, date
	///atmosphere
	/// Output: radiation rate, Ra (MJ m-2 day-1).
	//

	/** SYMBOLS
	 *
	 * Gsc 		= solar constant = 0.082 Mj/m2/min
	 * Ra		= extraterestrial (solar) radiation  MJ/m2/d
	 * Rs		= shortwave radiation / global radiation MJ/m2/d
	 * Rso		= clear sky solar radiation MJ/m2/d
	 * Rn 		= net radiatio MJ/m2/d
	 * Rnl		= net longwave radiation MJ/m2/d
	 * Rns		= net solar radiation MJ/m2/d
	 * albedo	= reflection rate at surface
	 * actualSun	 = actual duration of sun [hours]
	 * daylightHours = max. possible daylight hours
	 * G 		= soil heat flux
	 * omega_s	= sunset hour angle [rad]
	 * phi		= latitude [rad]
	 * dr 		= inverse relative distance Earth-Sun
	 * delta	= solar decimation [rad]
	 *
	 */
	double const Gsc = 0.082; // MJ/m2/min // solar constant

	/**
	 * Calculation procedures:
	 *
	 * --- Extraterrestrial radiation for DAILY periods Ra ---
	 *
	 * The extraterrestrial radiation, Ra,
	 * for each day of the year and for different latitudes can be estimated
	 * from the solar constant, the solar declination and the time of the year
	 */
	double phi;
	latitude_->get(t,phi);
	phi = phi * M_PI/180.; // get phi in radians [rad]

	// std::cout << "phi " << phi << std::endl;

	std::size_t myDateNumber = TimeConversion::dateToNumber(t);
//	std::cout << "Day of the Year " << myDateNumber << std::endl;

	double delta 	= 0.409*sin(2.*M_PI/365 *double(myDateNumber)-1.39);
	double dr 		= 1.+ 0.033 * cos(2.*M_PI/365 *double(myDateNumber));

	double tan_phi 	= tan(phi);
	double tan_del 	= tan(delta);
	double omega_s  = acos(-tan_phi * tan_del);
//	std::cout << "omega_s " << omega_s << std::endl;
	// OR the same with atan
//	double x = 1.- tan_phi*tan_phi * tan_del*tan_del; if(x <= 0.0) x = 0.00001;
//	double omega_s 	= M_PI*0.5 - atan(-tan_phi*tan_del/sqrt(x));

	double Ra = 24.*60. / M_PI  * Gsc* dr * (omega_s*sin(phi)*sin(delta) + cos(phi)*cos(delta)*sin(omega_s));
	// Ra is expressed in the above equation in MJ m-2 day-1. The corresponding equivalent evaporation in mm day-1 is obtained by multiplying Ra by 0.408

	// The latitude phi, expressed in radians is positive for the northern hemisphere and negative for the southern hemisphere
	// North Rhine Westphalia: 51°45'N Latitude
	// Cologne: 50°56'N
	// Juelich: 50°55'19"N --> 50.9253 decimal  // North is positive sign
	//
	// if phi in decimaldegrees, which means for juelich 50+55/60 = 50.9253
	// then phi = phi * M_PI/180 = 0.888814 (rad)

	/**
	 * Extraterrestrial radiation for HOURLY or shorter periods (Ra)
	 *
	 * t:	standard clock time at the midpoint of the period [hour]. For example for a period between 14.00 and 15.00 hours, t = 14.5,
	 * Lz:	longitude of the centre of the local time zone [degrees west of Greenwich]. For example, Lz = 75, 90, 105 and 120° for the Eastern, Central,
	 * 		Rocky Mountain and Pacific time zones (United States) and Lz = 0° for Greenwich, 330° for Cairo (Egypt), and 255° for Bangkok (Thailand),
	 * Lm: 	longitude of the measurement site [degrees west of Greenwich],
	 * Sc: 	seasonal correction for solar time [hour]
	 *
	 *
	 * double omega		= M_PI/12. * ( (t + 0.06667*(Lz - Lm)+Sc)-12. ); // solar time angle at midpoint of hourly or shorter period [rad]
	 * double omega_1 	= omega - M_PI* time1 / 24; // solar time angle at beginning of period [rad]
	 * double omega_2 	= omega + M_PI* time1 / 24; // solar time angle at end of period [rad]
	 *  // time1  length of the calculation period [hour]: i.e., 1 for hourly period or 0.5 for a 30-minute period.
	 *
	 * if(omega < -omega_s || omega > omega_s){  // sun is below horizon
	 * Ra = 0.;
	 * return;}
	 *
	 *  // The seasonal correction for solar time is:
	 * double b = 2.*M_PI*(double(myDateNumber)-81.)/364;
	 * double Sc = 0.1645*sin(2.*b)-0.1255*cos(b)-0.025*sin(b);
	 *
	 * The solar time angles at the beginning and end of the period are given by
	 * Ra = 12*60/M_PI * Gsc dr *( (omega_2-omega_1)*sin(phi)*sin(delta)+cos(phi)*cos(delta)*(sin(omega_1)-sin(omega_2) );
	 */

	/// --- Daylight hours ---
	double daylightHours = 24./M_PI * omega_s;
//	std::cout << "N " << daylightHours << std::endl;
	/// --- Solar radiation ---
	double a_s = 0.25;
	double b_s = 0.5;
	// Depending on atmospheric conditions (humidity, dust) and solar declination (latitude and month),
	// the Angstrom values as and bs will vary. Where no actual solar radiation data are available and
	// no calibration has been carried out for improved as and bs parameters,
	// the values as = 0.25 and bs = 0.50 are recommended.
	if (angstrom_as_) {
		angstrom_as_->get(t, a_s);
	}
	if (angstrom_bs_) {
		angstrom_bs_->get(t, b_s);
	}

	double Rs(0.0);
	if (netSolarRadiation_) {
		netSolarRadiation_->get(t, Rs);
	} else if (actualDurationofSunshine_) {
		double actualSunhours;
		actualDurationofSunshine_->get(t, actualSunhours);
		Rs = (a_s + b_s * actualSunhours / daylightHours) * Ra;
	} else {
		msg::error("Radiation::calculate: Provide either netSolarRadiation or actualDurationofSunshine parameter");
	}

	 /// --- Clear-sky solar radiation Rso:
	 double Rso = (a_s + b_s)*Ra;
	 // a_s+b_s fraction of extraterrestrial radiation reaching the earth on clear-sky days (actualSun = daylightHours)

	/// --- Net solar or net shortwave radiation (Rns) ---

	double albedo;
	/// albedo or canopy reflection coefficient, which is 0.23 for the hypothetical grass reference crop [dimensionless]
	/// albedo open water is 0.05
	albedo_->get(t, albedo);

	double Rns 		= (1.-albedo)*Rs;

	double const sigma 	= 0.000000004903; // Stefan-Boltzmann constant [4.903 10-9 MJ K-4 m-2 day-1],
	double stk4;

	if(minTemperature_ && maxTemperature_){
		double Tmin, Tmax;
		minTemperature_->get(t, Tmin);
		maxTemperature_->get(t, Tmax);
		double Tkmin = Tmin+273.15;
		double Tkmax = Tmax+273.15;
		double Tkmin4 = Tkmin*Tkmin*Tkmin*Tkmin;
		double Tkmax4 = Tkmax*Tkmax*Tkmax*Tkmax;
		stk4 = sigma*(Tkmin4 + Tkmax4)*0.5;
	}
	else {
		double temperature;
		dailyTemperature_->get(t, temperature);
		double Tk = temperature+273.15;
		stk4 = sigma * Tk *Tk *Tk *Tk;
	}

	double e_a;
	actualVaporPressure_->get(t,e_a);// in hPa
	e_a = e_a*0.1; // in kPa

//	std::cout << "ea  " << e_a << std::endl;
//	std::cout << "0.34 - 0.14*sqrt(e_a)  " << 0.34 - 0.14*std::sqrt(e_a) << std::endl;
//	std::cout << "1.35*Rs/Rso -0.35  " << 1.35*Rs/Rso -0.35 << std::endl;

	// 0.34 - 0.14*std::sqrt(e_a) this term is the net emissivity,
	// values 0.34 and 0.14 are correlation coefficients [Brunt, 1932; Jensen et al., 1990]
    double Rnl 	= stk4*(0.34 - 0.14*sqrt(e_a))*(1.35*Rs/Rso -0.35);
    /**
     * An average of the maximum air temperature to the fourth power and
     * the minimum air temperature to the fourth power is commonly used
     * in the Stefan-Boltzmann equation for 24-hour time steps. The term (0.34-0.14*sqrt(e_a))
     * expresses the correction for air humidity, and will be smaller if the humidity increases.
     * The effect of cloudiness is expressed by (1.35 Rs/Rso - 0.35).
     * The term becomes smaller if the cloudiness increases and hence Rs decreases.
     * The smaller the correction terms, the smaller the net outgoing flux of longwave radiation.
     * Note that the Rs/Rso term must be limited so that Rs/Rso <= 1.0.
     */

    /// --- net radiation (Rn):
    netRadiation = Rns - Rnl;

//    std::cout << "Ra " << Ra << std::endl;
//    std::cout << "Rso " << Rso << std::endl;
//    std::cout << "Rns " << Rns << std::endl;
//    std::cout << "Rnl " << Rnl << std::endl;
//    std::cout << "Rs " << Rs << std::endl;
//    std::cout << "netRadiation in MJ/m2/d " << netRadiation << std::endl;
//    std::cout << "daylightHours " << daylightHours << std::endl;
//    std::cout << "myActualVaporPressure in Radiation Method in kPa " << e_a << std::endl;

} // end calculate radiation





//==================registration of the classes=================
class AutoRegisterRadiationClassInstantiationFunctions {
public:
	AutoRegisterRadiationClassInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["Radiation"] =
				newInstantiationRadiation;
	}
};

// our one instance of the proxy
static AutoRegisterRadiationClassInstantiationFunctions p4556744351000;
