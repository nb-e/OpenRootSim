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
#include "ETbase.hpp"
#include <math.h>
#include "../../cli/Messages.hpp"


/// Input:  Time at which evaporation rate is needed, Parameters which go into
///			the formula for computation of evaporation:
///			air temperature, solar radiation, relative humidity or dew point,
///			wind speed (input or random (Weibull distributed), Leaf Area Index
///
/// Output: reference evaporation rate ET0, so called EvapoTranspiration

	/**	Further Description:
	 *  "For a surface the vapor pressure is less than the saturation vapor pressure at surface temperature,
	 *  so we can not apply Penman (1948) or Slatyer & McIlroy (1961)." \cite{Monteith1965}
	 *
	 *  SYMBOLS, Monteith1965
	 * 	lambda	: 	latent evaporation heat of water
	 * 	lambda*E: 	total latent head of evaporation
	 * 	rho		: 	density of air
	 * 	c		:	specific heat capacity of air
	 *   rho*c	:	specified head of air
	 *   gamma	:	psychrometer constant, or
	 *  			function of the physical properties of dry air,
	 *  			is 0.66 mb.degree Celsius^-1 at 20 degrees Celsius and 1 atm. pressure
	 *	delta	: 	"slope", the change of saturation vapour pressure with temperature,
	 *				increases with temperature.
	 *				(At 0 degree Celsius, delta = 0.5 mb. degrees Celsius^-1
	 *				at 25 degree Celsius, delta = 1.9 mb. degrees Celsius^-1)
	 *	H		:	Radiation Balance, or the rate of heat supply
	 *	r_a		:	aerodynamical diffusion resistance, or
	 *				time in which 1 cubic cm of air exchanges heat with 1 cm^2 of surface
	 *	r_s/r_a	:	surface resistance of a crop canopy, or stomata diffusion resistance
	 *	1/r_a	: 	the rate of change in cubic centimeters of air per square centimeter of surface per second
	 *	e_s(T)	:	atm. saturation vapour pressure at T
	 *	e_a(T)	:	vapour pressure (mb.), actual
	 *
	 *	LITERATURE: Evaporation and Environment, J.L.Monteith, pp.205-234,
	 *	EVAPORATION OF A FULLY DEVELOPED PLANT STOCK WITHOUT WATER STRESS
	 */
	/************************************************************************/
ETbaseclass::ETbaseclass(SimulaDynamic* const pSV):DerivativeBase(pSV){
	//check if unit given in input file agrees with this function
	if(pSD->getType()=="SimulaVariable"){
		pSD->checkUnit("cm");
	}else{
		pSD->checkUnit("cm/day");
	}
	//dailyTemperature=ORIGIN->getChild("environment")->getChild("atmosphere")->getChild("averageDailyTemperature","degreesC");
	dailyTemperature_			= pSD->getPath("/environment/atmosphere/averageDailyTemperature", "degreesC");

	windSpeed_ 					= pSD->existingPath("/environment/atmosphere/windSpeed","m/s"); //, "m/s"

	actualVaporPressure_		= pSD->getSibling("actualVaporPressure");
	saturatedVaporPressure_	    = pSD->getSibling("saturatedVaporPressure");
	slopeVaporPressure_			= pSD->getSibling("slopeVaporPressure");

	sensibleHeat_ 			    = pSD->existingPath("/environment/atmosphere/sensibleHeat");
	if(sensibleHeat_){
		Unit u=sensibleHeat_->getUnit();
		if(u=="W/m2"){
			convSH=11.57; // W is 1 joule per second  1MJ/Day = 11.57 j/s =
		}else if (u=="J/m2/day"){
			convSH=1;
		}else if (u=="MJ/m2/day"){
			convSH=1000000;
		}else{
			msg::error("ETBaseClass: Unknown unit for netRadiation");
		}

	}
	//this radiation corrected for reflection, and since the albedo of the crop is different from that of soil, we have two numbers
	netRadiation_				= pSD->existingSibling("netRadiation");
	if(!netRadiation_) {
		netRadiation_= pSD->getPath("/environment/atmosphere/netRadiation");
	}
	netRadiationSoil_			= pSD->existingSibling("netRadiationSoil");
	if(!netRadiationSoil_) {
		netRadiationSoil_= pSD->getPath("/environment/atmosphere/netRadiationSoil");
	}

	Unit u=netRadiation_->getUnit();
	if(u=="W/m2"){
		conv=11.57; // W is 1 joule per second  1MJ/Day = 11.57 j/s =
	}else if (u=="J/m2/day"){
		conv=1;
	}else if (u=="MJ/m2/day"){
		conv=1000000;
	}else{
		msg::error("ETBaseClass: Unknown unit for netRadiation");
	}
	u=netRadiationSoil_->getUnit();
	if(u=="W/m2"){
		convS/=11.57;
	}else if (u=="J/m2/day"){
		convS=1;
	}else if (u=="MJ/m2/day"){
		convS=1000000;
	}else{
		msg::error("ETBaseClass: Unknown unit for netRadiationSoil");
	}



 	airPressure_		        = pSD->getSibling("airPressure","Pa");
 	specificHeatCapacityOfAir_  = pSD->getSibling("specificHeatCapacityOfAir","J/kg/degreesC");

	interception_				= pSD->getSibling("interception","cm/day");

	leafAreaIndex_ 				= pSD->getPath("/plants/meanLeafAreaIndex"); //, "m/s"
	splitparam_					= pSD->getPath("/plants/meanExtinctionCoefficient");

 	// for the checkMode method
	rhp_						= pSD->existingPath("/environment/atmosphere/relativeHumidity");
	stomatalResistance_ 		= pSD->existingPath("/plants/stomatalResistance", "s/m");

 	SimulaBase * p=pSD->existingChild("mode");
 	if(p){
 		std::string ps;
 		p->get(ps);
 		if(ps=="evaporation"){
 			mode=1;
 		}else if(ps=="transpiration"){
 			mode=2;
 		}else{
 			mode=0;
 		}
 	}else{
 		mode=0;
 	}

//	if(mymap.empty()){
//			mymap["Stanghellini"] = Stanghellini; //Greenhouse formula?!
//			mymap["PriestleyTaylor"] = PriestleyTaylor;
//			mymap["Grass_reference_evapotranspiration"] = Grass_reference_evapotranspiration;
//			mymap["Tall_reference_Crop"] = Tall_reference_Crop;
//			mymap["monteithEQ"] = monteithEQ;
//			mymap["penmanEQ"] = penmanEQ;
//	}
}

//ETbaseclass::MyMap ETbaseclass::mymap;

void ETbaseclass::calculate(const Time &t, double& ET){
	double LAI;
	leafAreaIndex_->get(t,LAI);
	double ETCROP, ETSOIL;
	calculateET(t, ETCROP, ETSOIL );

	double KDF;
	splitparam_->get(t,KDF);
	ETCROP = ETCROP*(1.-exp(-KDF *LAI));
	ETSOIL = ETSOIL*exp(-KDF *LAI);// exp(..) equals 1-SC, Soil without crop
	switch (mode) {
		case 1:
			ET = ETSOIL;
			break;
		case 2:
			ET = ETCROP;
			break;
		default:
			ET = ETCROP + ETSOIL;
			if (interception_) {
					double E_i = 0.;
					interception_->get(t, E_i);
					ET += E_i; // Interception on plant
			}
			break;
	}
}

double ETbaseclass::calculateLatentHeat(double &temperature) {
	if (temperature < 0. || temperature > 45.) {
		msg::warning(
				"ETbaseclass::calculateLatentHeat: Valid temperature range to compute lambda (latent evaporation heat of water) is for surface temperature, air temperature can be higher, but is not distinguished.");
		msg::error(
				"ETbaseclass::calculateLatentHeat: Invalid temperature. Out of range: 0 ºC <= T <= 45 ºC");
	}
	/// --- latent heat of vaporization ---
	/// The latent heat of vaporization is set equal to 2.45 MJ kg-1 in FAO-56 Penman-Monteith evapotranspiration estimates.
	//double lambda = 2450000.; ///< J/kg,  lambda !=0
	/// by Harrison, L.P. 1963. Fundamentals concepts and definitions relating to humidity.
	/// In Wexler, A. (Ed.) Humidity and moisture. Vol 3, Reinhold Publishing Co., New York, NY, USA.
	double lambda = 2501020. - 2361. * temperature;
	/// this is a linear approximation derived from values given in TableA3 in Monteith and Unsworth 1990,
	/// valid for T(at surface level) between 0 and 45 degreesC.
	return lambda;
}


void ETbaseclass::calculateRadiation(const Time &t,double & H_soil,  double & H_crop){
	/// --- Radiation: ---
	/////////////////////////////////////////////
	//  H = - rho*c*(T2-T0)/r_a;? TODO better H (and G) !?
	/// A relationship proposed by Choudhury (1989) for predicting soil heat flux density (G) under daylight conditions
	//  G = 0.4* exp(-LAI_a)*R_n;
	//  thus, if R_n is given,
	//  H = R_n*(1.-0.4* exp(-LAI_a));
	/// This Equation can be used to check the functioning and relative accuracy of soil heat flux plates
	/// after correcting measurements for temperature change of soil above the plates.
	/// The relationship of this Equation does not hold for 24-hour data, as a positive 24-hour soil heat flux estimate would always result.
	/// The user must be aware that this Eq. is only approximate and does not consider effects of plant spacing, sun angle, soil colour,
	/// soil moisture, or soil texture, nor the sensible heat balance at the surface on the ratio of G to Rn.
	/// Generally, more than one soil heat flux plate is used due to spatial variation in soil, soil water content, and vegetation.
	////////////////////////////////////////////////
//	double H_soil(0.);
//	double H_crop(0.);
	if (sensibleHeat_) {
		double H(0.);
		sensibleHeat_->get(t, H);// H in MJ/m2/day:
		H *= convSH; // get Joule
		msg::warning("ETbaseclass::calculateRadiation: radiation based on sensible heat, and no distiction made between soil and crop.");
		H_soil = H;  // in J/m2/d
		H_crop = H;  // in J/m2/d
	} else {
		//msg::warning("ETbaseclass::calculateRadiation: Net Radiation Rn is computed. ");
		//msg::warning("ETbaseclass::calculateRadiation: Soil Heat Flux G is assumed to be zero! ");
		double G = 0.; // for day and ten-day periods
		double Rn_soil;
		netRadiationSoil_->get(t, Rn_soil);
		Rn_soil = Rn_soil*convS;
		H_soil = Rn_soil - G;

		double Rn_crop;
		netRadiation_->get(t, Rn_crop);
		Rn_crop = Rn_crop*conv;
		H_crop = Rn_crop - G;
	}
}


void ETbaseclass::checkMode(const Time & t){

	//std::string myautomode = "monteithEQ";
	std::string myMode 	=  this->getName();

	if(!rhp_){
		if(myMode!="PriestleyTaylor") msg::warning("ETbaseclass::checkMode: No relative humidity input is given. Use \"PriestleyTaylor\" instead of "+myMode);
		//myautomode = "PriestleyTaylor";
	}

	if(!stomatalResistance_){
		if(myMode == "Tall_reference_Crop"){
			msg::warning("ETbaseclass::checkMode: No stomatal diffusion parameter input is given. Default Values are taken.");
			//myautomode = "Tall_reference_Crop";
		}else if (myMode == "Grass_reference_evapotranspiration") {
			msg::warning("ETbaseclass::checkMode: No stomatal diffusion parameter input is given. Default Values are taken.");
			//myautomode = "Grass_reference_evapotranspiration";
		}else{
			msg::warning("ETbaseclass::checkMode: No stomatal diffusion parameter input is given. Default Values are taken. Suggested mode is \"Grass_reference_evapotranspiration\" instead of "+myMode);
			//myautomode = "Grass_reference_evapotranspiration";
		}
	}

	if (windSpeed_) {
		/// --- Wind speed ---  /// --- Weibull distributed ---
		double U2;
		windSpeed_->get(t, U2); ///< U2 being the wind speed at 2-m height in m/s
		if(U2<=0.5){
			if(myMode != "Stanghellini") msg::warning("ETbaseclass::checkMode: Windspeed <= 0.5 m/s. Suggested mode is Stanghellini instead of "+myMode);
		   //myautomode = "Stanghellini";
		}
	} else {
		if(myMode != "PriestleyTaylor") msg::warning("ETbaseclass::checkMode: No wind speed is set; Evaporation function of PriestleyTaylor is suggested where no wind speed is needed instead of "+myMode);
		//myautomode = "PriestleyTaylor";
	}


}
