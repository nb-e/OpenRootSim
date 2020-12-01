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
#include "DiffusionResistance.hpp"
#include "../../cli/Messages.hpp"
#include <math.h>


/********************stomatal resistance****************************/
StomatalResistance::StomatalResistance(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("s/m");
	stomatalConductance_ 		 = pSD->existingSibling("meanStomatalConductance","mol/m2/s");
	dailyBulkStomatalResistance_ = pSD->existingSibling("dailyBulkStomatalResistance");
	leafAreaIndex_ 				 = pSD->getSibling("meanLeafAreaIndex"); //, "m/s"
}

std::string StomatalResistance::getName() const {
	return "stomatalResistance";
}

DerivativeBase * newInstantiationStomatalResistance(
		SimulaDynamic* const pSD) {
	return new StomatalResistance(pSD);
}


/***************************************************************************/
void StomatalResistance::calculate(const Time &t, double& r_s) {
/****************************************************************************/
	double LAI = 3;
	leafAreaIndex_->get(t,LAI);
	if (stomatalConductance_) {
		double g_sV;
		// TODO what gives the LICOR?
		stomatalConductance_->get(t, g_sV); //mol/m2/s
		double absoluteHumidity= 2; //30 g/m3 / 18g/mol =~ 2mol/m3 (30 is upper limit might be much less)
		//also need active lai
		r_s = absoluteHumidity / g_sV ;//rs is in second/m
		//todo this ssems wrong, r_s should be around 100
	} else {
//		Parameter rl is the inverse of the stomatal conductance per unit leaf area.
//		Several earlier studies have fixed the value of rl at 100 s m−1 for well-watered
//		agricultural crops ( Monteith, 1965, Szeicz and Long, 1969 and Allen et al., 1989) when calculations were made on a 24-h basis.
//		However, it is recognized that rl varies during the course of a day with levels of solar radiation,
//		leaf temperature, and vapor pressure gradient ( Jarvis, 1976, Stewart, 1989 and Price and Black, 1989)
//		and rl increases with environmental stresses such as soil moisture deficit
//		( Stewart, 1988, Stewart and Verma, 1992 and Hatfield and Allen, 1996).
// TODO coupling
		double r_l(100.); // default value, (m/s) is the daily bulk stomatal resistance of the well-illuminated (single) leaf
		// The bulk stomatal resistance, rl, is the average resistance of an individual leaf,
		// and has a value of about 100 s/m under well-watered conditions.
// TODO could be fitted to a crop average?
		if (dailyBulkStomatalResistance_){
			dailyBulkStomatalResistance_->get(t, r_l);
		}else{
			msg::warning("StomatalResistance: dailyBulkStomatalResistance not found, using default value of 100 m/s");
		}
		double lai_active;
		if(LAI > 0.8/0.3)
			lai_active = 0.5 * LAI;
		else if(LAI >= 1){
			lai_active = LAI / (0.3 * LAI+ 1.2);
		}else{
			lai_active=1./1.5;
		}
		// the active (sunlit) leaf area index, which takes into consideration
		/// the fact that generally only the upper half of dense clipped grass
		/// is actively contributing to the surface heat and vapour transfer.
		/// Allen et al. (1989) proposed equations to estimate bulk surface resistance to water flux based on
		/// the crop height of grass or alfalfa in terms of the estimate crop LAI.
		///
		/// --- stomata/ canopy diffusion resistance ---
		///
		/// If the crop is not transpiring at a potential rate, the resistance depends also on the water status of the vegetation.
		/// An acceptable approximation to a much more complex relation of the surface resistance of dense full cover vegetation is:
		//
		r_s = r_l / lai_active;
		//
		// This procedure also presents difficulties. Firstly, evaporation losses from soil must be
		// negligible. If not, r_s will not be representative of total evaporation losses.
		// Secondly, spatial and temporal sampling problems are encountered in obtaining
		// representative values of r_AD and r_AB for a plant community. Thirdly, lai is tedious
		// and often difficult to measure, particularly in developing crops. Despite these
		// limitations, this approach provides the only available method for obtaining resistance
		// values without prior knowledge of evaporation.
		// Nevertheless, in a paper Paw U and Meyers (1989), using a higher-
		// order canopy turbulence model, show clearly that the parallel resistance
		// weighted by leaf area index is problematic, even when the soil is dry, and can
		// generate serious errors when used to estimate the bulk canopy resistance.
		//
		// By assuming a crop height of 0.12 m, the surface resistance for the grass reference surface approximates to 70 s/m.
		// It approximates to 45 s/m for a 0.50-m crop height.
		// The hourly r_c (=r_s) values of 50 (clipped grass) or 30 (alfalfa) and 200 s/m (both crops), for day and nighttime respectively,
		// were concluded to be fairly accurate in matching reference evapotranspiration calculated with daily data (Walter et al., 2002).

	}
}


/********************aerodynamic resistance****************************/
AerodynamicResistance::AerodynamicResistance(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("s/m");
	windSpeed_ 		 = pSD->existingPath("/environment/atmosphere/windSpeed"); //, "m/s"
}

std::string AerodynamicResistance::getName() const {
	return "aerodynamicResistance";
}

DerivativeBase * newInstantiationAerodynamicResistance(
		SimulaDynamic* const pSD) {
	return new AerodynamicResistance(pSD);
}


/***************************************************************************/
void AerodynamicResistance::calculate(const Time &t, double& r_a) {
/****************************************************************************/
	/// http://agsys.cra-cin.it/tools/evapotranspiration/help/Aerodynamic_resistance.html
	///
	/// Where no wind data are available within the region,
	/// a value of 2 m/s can be used as a temporary estimate.
	/// This value is the average over 2000 weather stations around the globe.
	/// In general, wind speed at 2 m, u2, should be limited to about u2 >= 0.5 m/s when used in the ETo equation.
	/// This is necessary to account for the effects of boundary layer instability and buoyancy of air in promoting
	/// exchange of vapor at the surface when air is calm. This effect occurs when the wind speed is small and
	/// buoyancy of warm air induces air exchange at the surface. Limiting u2 >= 0.5 m/s in the ETo equation improves
	/// the estimation accuracy under the conditions of very low wind speed.
	double U2;
	if (windSpeed_) {
		/// --- Wind speed ---  /// --- Weibull distributed ---
		windSpeed_->get(t, U2); ///< U2 being the wind speed at 2-m height in m/s

    // The aerodynamic resistance equation for open air estimations reduces to r_a = 208./U2;
	// An alternative, semi-empirical, equation of ra (Thom and Oliver, 1977)
	// is used for greenhouse conditions (Stanghellini equation), characterized by low wind speed (<1 m s-1)
	} else {
		U2 = 0.;
		msg::warning("EvapoTranspiration: No wind speed is set by user. Zero wind speed is assumed.");
		/// Daily values of wind speed (Ud, m s-1) are generated using a two parameter Weibull probability density function, f(Ud) (Takle and Brown, 1977)
		/// Distribution scale parameter from station Cheung Chau, Hong Kong,
		//const double a(6.0), b(2.1); // TODO fit scale parameters for weibull distribution in XML
		/// a: scale parameter (m/s), b: shape parameter (dimensionless)
	}
	/// If not available, wind speed at 2 m above ground surface can be derived from wind speed, Uz (m/s), measured at any height (h, m):
	/// U2 = Uz * 1./(log(67.8*h-5.42))*4.87;

	if(U2 > 1.){
		r_a = 208./U2;
	}
	else{
		r_a = 665. / (1. + 0.54 * U2); // s/m
	}
}



//==================registration of the classes=================
class AutoRegisterDiffusionResistanceClassInstantiationFunctions {
public:
	AutoRegisterDiffusionResistanceClassInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["stomatalResistance"] = newInstantiationStomatalResistance;
		BaseClassesMap::getDerivativeBaseClasses()["aerodynamicResistance"] = newInstantiationAerodynamicResistance;
	}
};

// our one instance of the proxy
static AutoRegisterDiffusionResistanceClassInstantiationFunctions p4510656771745465435;
