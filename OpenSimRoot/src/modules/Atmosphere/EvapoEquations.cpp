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
#include "EvapoEquations.hpp"
#include <math.h>
#include "../../cli/Messages.hpp"

/*******************PenmanMonteith*************************/
PenmanMonteith::PenmanMonteith(SimulaDynamic* const pSV) :
		ETbaseclass(pSV) {
	//check if unit given in input file agrees with this function
	airDensity_ = pSD->getSibling("airDensity", "kg/m3");
	stomatalResistance_ = pSD->getPath(
			"/plants/stomatalResistance", "s/m");
	aerodynamicResistance_ = pSD->getSibling(
			"aerodynamicResistance", "s/m");
}
std::string PenmanMonteith::getName() const {
	return "monteithEQ";
}

DerivativeBase * newInstantiationPenmanMonteith(
		SimulaDynamic* const pSD) {
	return new PenmanMonteith(pSD);
}

 /***************************************************************************/
 void PenmanMonteith::calculateET(const Time &t, double& ETCROP, double& ETSOIL){
 /***************************************************************************/
	/// PenmanMonteith, ET_0
	///
	/// references listed by SNAPS/SOIL:
	/// Feddes, Water, Heat and Crop Growth, 1971, equation (26)-(29)
	/// Feddes, R.A.; Kowalik, P.J.; Zaradny, H., Simulation of field water use and crop yield, PUDOC, Wageningen, Netherlands 1978 ISBN 90-220-0676-X
	/// Monteith, J.L., Evaporation and Environment, Proc. Symp. Soc. Exp. Biol. 19 (1965), 205-234
	/// Penman, H.L., Natural evaporation from open water, bare, soil and grass, Proc. Roy. Soc., London 1948, pp. 120-145
	//
	this->checkMode(t);
	//--------------------------------------------------------------------
	double temperature;
	dailyTemperature_->get(t, temperature);
	/*********************************************************************/
	/// --- latent heat of vaporization ---
	double lambda = this->calculateLatentHeat(temperature);
	/*********************************************************************/
	///  --- pressure of water vapor in the air ---
	double e_a, e_s; // actual and saturated vapor pressure
	// --- gradient of saturation vapor pressure, derivative of e_s ---
	double delta; // 1.9 mb./C at 25 deg. Celsius, 0.5 mb./C at  0 deg. Celsius
	actualVaporPressure_->get(t, e_a);
	saturatedVaporPressure_->get(t, e_s);
	slopeVaporPressure_->get(t, delta);
	/// vapor pressure deficit of air (in mb.=hPa)
	double VPD = e_s - e_a;
	/*********************************************************************/
	double rho = 1000.;
	airDensity_->get(t, rho);
	double P;
	airPressure_->get(t, P);
	double c = 1013.; ///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
	specificHeatCapacityOfAir_->get(t, c);
	/// psychrometer constant
	const double MWratio = 0.622; ///< ratio molecular weight of water vapor/dry air
	//double gamma = 0.66; ///< mb./C at 20 deg. Celsius
	double gamma = (c * P * 0.01) / (lambda * MWratio);
	/*********************************************************************/
	double H_soil, H_crop;
	this->calculateRadiation(t, H_soil, H_crop);
	/*********************************************************************/
	/// --- aerodynamic resistance
	double r_a(1.);
	aerodynamicResistance_->get(t, r_a);
	/*********************************************************************/
	double r_s(0.);
	stomatalResistance_->get(t, r_s);
	/*********************************************************************/
	double dg = delta + gamma;
	const double conversion_day = 86400.; // conversion from second to 1 day
	/*********************************************************************/
	double E_wet_crop = (delta * H_crop + conversion_day * rho * c * VPD / r_a)
			/ (lambda * dg);
	E_wet_crop *= 0.1; // mm in cm
	/// This formula for ET is from SNAPS/SOIL (ecobas), but E_i influence slightly modified
	/// Evap. of a fully developed plant stock without water stress:
	ETCROP = E_wet_crop * dg / (delta + gamma * (1. + r_s / r_a));
//	if (interception_) {
//		double E_i = 0.;
//		interception_->get(t, E_i);
//		ETCROP += E_i; // Interception on plant
//	}

	double E_wet_soil = (delta * H_soil + conversion_day * rho * c * VPD / r_a)
			/ (lambda * dg);
	E_wet_soil *= 0.1; // mm in cm
	ETSOIL = E_wet_soil;

}


/*******************Penman*************************/
 Penman::Penman(SimulaDynamic* const pSV) :
 		ETbaseclass(pSV) {
 	//check if unit given in input file agrees with this function
	airDensity_					= pSD->getSibling("airDensity","kg/m3");
	aerodynamicResistance_		= pSD->getSibling("aerodynamicResistance","s/m");
 }
 std::string Penman::getName() const {
 	return "penmanEQ";
 }

 DerivativeBase * newInstantiationPenman(
 		SimulaDynamic* const pSD) {
 	return new Penman(pSD);
 }

 /***************************************************************************/
 void Penman::calculateET(const Time &t, double& ETCROP, double& ETSOIL) {
 /***************************************************************************/
	this->checkMode(t);
	//--------------------------------------------------------------------
	double temperature;
	dailyTemperature_->get(t, temperature);
	/*********************************************************************/
	/// --- latent heat of vaporization ---
	double lambda = this->calculateLatentHeat(temperature);
	/*********************************************************************/
	///  --- pressure of water vapor in the air ---
	double e_a, e_s; // actual and saturated vapor pressure
	// --- gradient of saturation vapor pressure, derivative of e_s ---
	double delta; // 1.9 mb./C at 25 deg. Celsius, 0.5 mb./C at  0 deg. Celsius
	actualVaporPressure_->get(t, e_a);
	saturatedVaporPressure_->get(t, e_s);
	slopeVaporPressure_->get(t, delta);
	/// vapor pressure deficit of air (in mb.=hPa)
	double VPD = e_s - e_a;
	/*********************************************************************/
	double rho = 1000.;
	airDensity_->get(t, rho);
	double P;
	airPressure_->get(t, P);
	double c = 1013.; ///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
	specificHeatCapacityOfAir_->get(t, c);
	/// psychrometer constant
	const double MWratio = 0.622; ///< ratio molecular weight of water vapor/dry air
	//double gamma = 0.66; ///< mb./C at 20 deg. Celsius
	double gamma = (c * P * 0.01) / (lambda * MWratio);
	/*********************************************************************/
	double H_soil, H_crop;
	this->calculateRadiation(t, H_soil, H_crop);
	/*********************************************************************/
	/// --- aerodynamic resistance
	double r_a(1.);
	aerodynamicResistance_->get(t, r_a);

	const double conversion_day = 86400.; // conversion from second to 1 day

	ETSOIL =(delta * H_soil + rho * c * conversion_day * VPD / r_a)
					/ (lambda * (delta + gamma));
	ETSOIL *= 0.1; // mm in cm

	ETCROP =(delta * H_crop + rho * c * conversion_day * VPD / r_a)
					/ (lambda * (delta + gamma));
	ETCROP *= 0.1; // mm in cm
//	if (interception_) {
//		double E_i(0.);
//		interception_->get(t, E_i);
//		ETCROP += E_i;
//	}

}


/*******************Grass_reference_evapotranspiration***********************/
Grass_reference_evapotranspiration::Grass_reference_evapotranspiration(SimulaDynamic* const pSV) :
 		ETbaseclass(pSV) {
 	//check if unit given in input file agrees with this function
 }
 std::string Grass_reference_evapotranspiration::getName() const {
 	return "Grass_reference_evapotranspiration";
 }

 DerivativeBase * newInstantiationGrass_reference_evapotranspiration(
 		SimulaDynamic* const pSD) {
 	return new Grass_reference_evapotranspiration(pSD);
 }


 /***************************************************************************/
void Grass_reference_evapotranspiration::calculateET(const Time &t,double& ETCROP, double& ETSOIL) {
 /***************************************************************************/
	this->checkMode(t);
	//--------------------------------------------------------------------
 	double temperature;
 	dailyTemperature_->get(t,temperature);
	//--------------------------------------------------------------------
	///  --- pressure of water vapor in the air ---
	double e_a, e_s; // actual and saturated vapor pressure
	// --- gradient of saturation vapor pressure, derivative of e_s ---
	double delta; // 1.9 mb./C at 25 deg. Celsius, 0.5 mb./C at  0 deg. Celsius
	actualVaporPressure_->get(t,e_a);
	saturatedVaporPressure_->get(t,e_s);
	slopeVaporPressure_->get(t,delta);
	double VPD = e_s - e_a;
	//--------------------------------------------------------------------
 	/// The latent heat of vaporization is set equal to 2.45 MJ kg-1 in FAO-56 Penman-Monteith evapotranspiration estimates.
 	double lambda = this->calculateLatentHeat(temperature);
 	double P;
    airPressure_->get(t,P);
 	double c = 1013.;///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
 	specificHeatCapacityOfAir_->get(t,c);
 	/// psychrometer constant
 	const double MWratio = 0.622; ///< ratio molecular weight of water vapor/dry air
 	//double gamma = 0.66; ///< mb./C at 20 deg. Celsius
 	double gamma = (c * P *0.01) / (lambda * MWratio);
 	//--------------------------------------------------------------------
 	/// --- Radiation: ---
	double H_soil, H_crop;
	this->calculateRadiation(t, H_soil, H_crop);
	//--------------------------------------------------------------------
	double U2;
	if (windSpeed_) {
		windSpeed_->get(t, U2); ///< U2 being the wind speed at 2-m height in m/s
	} else {
		U2 = 0.;
	}
	//--------------------------------------------------------------------
	// because 1/lambda is 0.408 in kg/MJ and other coefficients as well set
	// and the other parameters to Units from formula, kPa and MJ, too:
	// H*=0.000001; // J to MJ
	H_crop*=0.000001; // J to MJ
	H_soil*=0.000001; // J to MJ
	VPD*=0.1; // hPa to kPa
	gamma*=0.1;
	delta*=0.1;
	/// T, temperature = mean daily or hourly air temperature at 1.5 to 2.5-m height [°C]
	/// With constants Cn, Cd for 12 cm (=low) grass
	const double Cn(900.), Cd(0.34); ///< Daily or monthly
	ETCROP = ( 0.408 * delta*H_crop + gamma * Cn*U2*VPD /(temperature+273)) / ( delta + gamma*(1.+Cd*U2) );
	ETCROP *= 0.1; // mm in cm
//	if(interception_){
//		double E_i(0.);
//		interception_->get(t,E_i);
//		ETCROP +=E_i;
//	}
	ETSOIL = ( 0.408 * delta*H_soil + gamma * Cn*U2*VPD /(temperature+273)) / ( delta + gamma*(1.+Cd*U2) );
	ETSOIL *= 0.1; // mm in cm

 }

/*******************Tall_reference_Crop*************************/
 Tall_reference_Crop::Tall_reference_Crop(SimulaDynamic* const pSV) :
		ETbaseclass(pSV) {
  	//check if unit given in input file agrees with this function
  }
  std::string Tall_reference_Crop::getName() const {
  	return "Tall_reference_Crop";
  }

  DerivativeBase * newInstantiationTall_reference_Crop(
  		SimulaDynamic* const pSD) {
  	return new Tall_reference_Crop(pSD);
  }


  /**************************************************************************/
  void Tall_reference_Crop::calculateET(const Time &t, double& ETCROP, double& ETSOIL) {
  /**************************************************************************/
	this->checkMode(t);
	//--------------------------------------------------------------------
	double temperature;
  	dailyTemperature_->get(t,temperature);
	//--------------------------------------------------------------------
 	///  --- pressure of water vapor in the air ---
 	double e_a, e_s; // actual and saturated vapor pressure
 	// --- gradient of saturation vapor pressure, derivative of e_s ---
 	double delta; // 1.9 mb./C at 25 deg. Celsius, 0.5 mb./C at  0 deg. Celsius
 	actualVaporPressure_	->get(t,e_a);
 	saturatedVaporPressure_	->get(t,e_s);
 	slopeVaporPressure_		->get(t,delta);
 	double VPD = e_s - e_a;
 	//--------------------------------------------------------------------
  	/// The latent heat of vaporization is set equal to 2.45 MJ kg-1 in FAO-56 Penman-Monteith evapotranspiration estimates.
  	double lambda = this->calculateLatentHeat(temperature); ///< J/kg,  lambda !=0
  	double P;
    airPressure_->get(t,P);
  	double c = 1013.;///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
  	specificHeatCapacityOfAir_->get(t,c);
  	/// psychrometer constant
  	const double MWratio = 0.622; ///< ratio molecular weight of water vapor/dry air
  	//double gamma = 0.66; ///< mb./C at 20 deg. Celsius
  	double gamma = (c * P *0.01) / (lambda * MWratio);
  	//--------------------------------------------------------------------
  	/// --- Radiation: ---
	double H_soil, H_crop;
	this->calculateRadiation(t, H_soil, H_crop);
	//--------------------------------------------------------------------
 	double U2;
 	if (windSpeed_) {
 			/// --- Wind speed ---  /// --- Weibull distributed ---
 			windSpeed_->get(t, U2); ///< U2 being the wind speed at 2-m height in m/s
 	}else{
 		U2 = 0.;
 	}
 	//--------------------------------------------------------------------
	// because 1/lambda is 0.408 in kg/MJ and other coefficients as well set
	// and the other parameters to Units from formula, kPa and MJ, too:
	H_crop*=0.000001; // J to MJ
	H_soil*=0.000001; // J to MJ
	VPD*=0.1; // hPa to kPa
	gamma*=0.1;
	delta*=0.1;
	// provide Cn and Cd
	// Example: Tall Referance Crop, 50 cm vegetation height
	const double Cn(1600.), Cd(0.38);
	ETCROP = ( 0.408 *delta*H_crop + gamma * Cn*U2*VPD /(temperature+273) ) / ( delta + gamma*(1.+Cd*U2) );
	ETCROP *= 0.1; // mm in cm
//	if(interception_){
//		double E_i(0.);
//		interception_->get(t,E_i);
//		ETCROP +=E_i;
//	}
	ETSOIL = ( 0.408 *delta*H_soil + gamma * Cn*U2*VPD /(temperature+273) ) / ( delta + gamma*(1.+Cd*U2) );
	ETSOIL *= 0.1; // mm in cm

  }


 /**************************Stanghellini********************************/
 Stanghellini::Stanghellini(SimulaDynamic* const pSV) : ETbaseclass(pSV) {
 	//check if unit given in input file agrees with this function
	stomatalResistance_ = pSD->getPath(
			"/plants/stomatalResistance", "s/m");
	aerodynamicResistance_ = pSD->getSibling(
			"aerodynamicResistance", "s/m");
 }
 std::string Stanghellini::getName() const {
 	return "Stanghellini";
 }

 DerivativeBase * newInstantiationStanghellini(
 		SimulaDynamic* const pSD) {
 	return new Stanghellini(pSD);
 }


 /***************************************************************************/
 void Stanghellini::calculateET(const Time &t, double& ETCROP, double& ETSOIL) {
 /***************************************************************************/
	this->checkMode(t);
	//--------------------------------------------------------------------
	double temperature;
 	dailyTemperature_->get(t,temperature);
 	double LAI;
	leafAreaIndex_->get(t,LAI);
	//--------------------------------------------------------------------
 	/// The latent heat of vaporization is set equal to 2.45 MJ kg-1 in FAO-56 Penman-Monteith evapotranspiration estimates.
 	double lambda = this->calculateLatentHeat(temperature);
 	//--------------------------------------------------------------------
 	double P;
    airPressure_->get(t,P);
 	double c = 1013.;///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
 	specificHeatCapacityOfAir_->get(t,c);
 	/// psychrometer constant
 	const double MWratio = 0.622; ///< ratio molecular weight of water vapor/dry air
 	//double gamma = 0.66; ///< mb./C at 20 deg. Celsius
 	double gamma = (c * P *0.01) / (lambda * MWratio);

 	double gamma_s = gamma * 0.1; // hPa in kPa
 	//--------------------------------------------------------------------
 	/// --- Radiation: ---
	double H_soil, H_crop;
	this->calculateRadiation(t, H_soil, H_crop);
	//--------------------------------------------------------------------
	/// --- Better source: ---
	/// Villarreal-Guerrero et al.:
	/// Comparison of three evapotranspiration models for a greenhouse cooling
	/// strategy with natural ventilation and variable high pressure fogging; Scientia Horticulturae 134 (2012) 210-221;
	///
	/// Stanghellini (1987) revised the Penman-Monteith model to represent conditions in greenhouse, air velocities are typically less than 1.0 m/s.
	/// The Stanghellini model includes the internal (canopy) and external (aerodynamic) resistance terms
	/// as well as more complex calculations of the solar radiation heat flux derived from the empirical characteristics
	/// of short wave and long wave radiation absorption in a multi-layer canopy. A leaf area index is used to account for
	/// energy exchange from multiple layers of leaves on greenhouse plants. The equation for hourly estimates of reference
	/// evapotranspiration (ET_0, mm/h) is derived from the form published in Kirnak and Short (2001), and Prenger et al. (2002):
	///
	/// see http://agsys.cra-cin.it/tools/evapotranspiration/help/Penman-Monteith.html
	///
	/// An approximation of rho for greenhouse environments (Stanghellini equation) is used (ASAE Standards, 1998), obtained with P=100000 and c=1000:
	const double Rd = 287.0531;
	double Tk = temperature + 273.15;
	double rho_green = 100000 / (Rd * Tk); // kg/m3
	//--------------------------------------------------------------------
	const double cc = 6.894757; // conversion coefficient pound-force per square inch (lbf in-2) to kPa, equal to 6.894757
	double tR = 491.67 + 1.8 * temperature; // air temperature in Rankine (°R) (TR=491.67+1.8 T)
	double term = -10440. / tR - 11.29 - 0.02702 * tR + 0.00001289 * tR * tR
			- 0.000000002478 * tR * tR * tR + 6.546 * log(tR);
	double e_s_green = cc * exp(term); // kPa
	//--------------------------------------------------------------------
	double rh(-1.);
	if (rhp_) {
		rhp_->get(t, rh);
	} else if (rh < 1. || rh > 100.) {
		msg::error(
				"EvapoTranspiration: Stanghellini formula for Greenhouse mode not possible. Set RH in range from 1% to 100%.");
	}

	double e_a_green = rh * 0.01 * e_s_green; // kPa
	/*********************************************************************/
	double r_a(524.);
	aerodynamicResistance_->get(t, r_a);
	/*********************************************************************/
	double r_s(70.);
	stomatalResistance_->get(t, r_s);
/// Canopy resistance was approximated to constant values for greenhouse environments (Stanghellini equation),
//	double const r_s_day = 70.; //s/m for daytime !!! NOT dependent on LAI here !!!
// double const r_s_night = 200.; // s/m for nighttime, (Oke, 1993)
// low wind
//	double const r_a = 524.; // wind is set to 0.5 m/s , 665 for no wind ?
	//--------------------------------------------------------------------
//for greenhouse there is an alternative equation by Stanghellini, ASCE Standards 1998
	double delta_e_s_S = 0.04145 * exp(0.06088 * temperature); // kPa/°C

	const double conversion_day = 86400.; // conversion from second to 1 day
	ETCROP = 1. / lambda
			* (delta_e_s_S * H_crop
					+ 2. * LAI * conversion_day * (e_s_green - e_a_green)
							* rho_green * c / r_a)
			/ (delta_e_s_S + gamma_s * (1. + r_s / r_a));
	ETCROP *= 0.1; // mm/day in cm/day
//	if (interception_) {
//		double E_i(0.);
//		interception_->get(t, E_i);
//		ETCROP += E_i;
//	}
/// TODO for greenhouse different formula!?
	ETSOIL = 1. / lambda
			* (delta_e_s_S * H_soil
					+ 2. * LAI * conversion_day * (e_s_green - e_a_green)
							* rho_green * c / r_a)
			/ (delta_e_s_S + gamma_s * (1. + r_s / r_a));
	ETSOIL *= 0.1; // mm/day in cm/day

 }


 /**************************Priestley_Taylor********************************/
 PriestleyTaylor::PriestleyTaylor(SimulaDynamic* const pSV) : ETbaseclass(pSV) {
 	//check if unit given in input file agrees with this function
 }
 std::string PriestleyTaylor::getName() const {
 	return "PriestleyTaylor";
 }

 DerivativeBase * newInstantiationPriestleyTaylor(
 		SimulaDynamic* const pSD) {
 	return new PriestleyTaylor(pSD);
 }


 /***************************************************************************/
 void PriestleyTaylor::calculateET(const Time &t, double& ETCROP, double& ETSOIL) {
 /***************************************************************************/
	this->checkMode(t);
	//--------------------------------------------------------------------
//	 if(this->checkMode(t)!=getName()){
//		 msg::warning("PriestleyTaylor: suggested mode is "+this->checkMode(t));
//	 }
    double temperature;
	dailyTemperature_->get(t,temperature);

 	// --- gradient of saturation vapor pressure, derivative of e_s ---
 	double delta; // 1.9 mb./C at 25 deg. Celsius, 0.5 mb./C at  0 deg. Celsius
 	slopeVaporPressure_->get(t,delta);

 	/// The latent heat of vaporization is set equal to 2.45 MJ kg-1 in FAO-56 Penman-Monteith evapotranspiration estimates.
 	double lambda = this->calculateLatentHeat(temperature);

 	double P;
    airPressure_->get(t,P);
 	double c = 1013.;///< J/kg/C  , this is proposed by FAO, constant pressure and 20°C
 	if(specificHeatCapacityOfAir_) specificHeatCapacityOfAir_->get(t,c);

 	/// psychrometer constant
 	const double MWratio = 0.622; ///< ratio molecular weight of water vapor/dry air
 	//double gamma = 0.66; ///< mb./C at 20 deg. Celsius
 	double gamma = (c * P *0.01) / (lambda * MWratio);

 	/// --- Radiation: ---
	double H_soil, H_crop;
	this->calculateRadiation(t, H_soil, H_crop);

	/// Empirical model, is a simplification of the FAO Penman-Monteith equation, where the aerodynamic part is neglected.
	/// Since the relative importance of the aerodynamic part of the FAO Penman-Monteith equation is high
	/// during night and winter, one can presume that Priestley-Taylor will underestimate evapotranspiration during those periods.
	///
	/// The Priestley-Taylor method (Priestley and Taylor, 1972) for the calculation of daily ET0
	/// replaces the aerodynamic term of Penman-Monteith equation by a dimensionless empirical multiplier (Priestley-Taylor coefficient):
	///
	/// The Priestley-Taylor coefficient (a, unitless), or coefficient of advectivity,
	/// represents the fraction of surface moisture available for evaporation in the Priestley-Taylor equation,
	/// either used as a constant input or as a function of some weather variables.
	///
	/// bowen_ratio is the ratio of sensible to latent heat flux (Bowen ratio). A default value is b=0.6
	///
	/// The ratio of sensible heat to latent heat is called the Bowen ratio
	/// and the physics of the various processes mean that this ratio is kept to
	/// a minimum -- a moist surface will hardly increase in temperature while evaporation is occurring,
	/// but once it has dried out there will be a rapid rise in temperature as the sensible heat flux takes over.
	///
	double bowen_ratio = 0.6;
	/// The modified Priestley-Taylor coefficient by Holtslag and Van Ulden (1983) is:
	double priestleyTaylorCoefficient = (1.+gamma/delta)/(1.+bowen_ratio);
	ETCROP = priestleyTaylorCoefficient/lambda * delta*H_crop / (delta + gamma); // mm/day
	ETCROP *=0.1; // cm/day
//	if(interception_){
//		double E_i(0.);
//		interception_->get(t,E_i);
//		ETCROP +=E_i;
//	}
	ETSOIL = priestleyTaylorCoefficient/lambda * delta*H_soil / (delta + gamma); // mm/day
	ETSOIL *=0.1; // cm/day

 }


 //==================registration of the classes=================
 class AutoRegisterEvapoEquationClassInstantiationFunctions {
 public:
 	AutoRegisterEvapoEquationClassInstantiationFunctions() {
 		BaseClassesMap::getDerivativeBaseClasses()["PriestleyTaylor"] = newInstantiationPriestleyTaylor;
 		BaseClassesMap::getDerivativeBaseClasses()["Stanghellini"] 	= newInstantiationStanghellini;
 		BaseClassesMap::getDerivativeBaseClasses()["Grass_reference_evapotranspiration"] 	= newInstantiationGrass_reference_evapotranspiration;
 		BaseClassesMap::getDerivativeBaseClasses()["Tall_reference_Crop"] 	= newInstantiationTall_reference_Crop;
 		BaseClassesMap::getDerivativeBaseClasses()["penmanEQ"] 	= newInstantiationPenman;
 		BaseClassesMap::getDerivativeBaseClasses()["monteithEQ"] = newInstantiationPenmanMonteith;
 	}
 };

 // our one instance of the proxy
 static AutoRegisterEvapoEquationClassInstantiationFunctions p4557114544329965435;
