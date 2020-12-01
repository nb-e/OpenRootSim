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
#include "Interception.hpp"

#include <math.h>

/***************************Interception*************************************/

Interception::Interception(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("cm/day");
	precipitation_ 	= pSD->getPath("/environment/atmosphere/precipitation", "cm/day"); ///< only used if interception is modeled
	leafAreaIndex_ 	= pSD->getPath("/plants/meanLeafAreaIndex"); //, "m/s"
	splitparam_		= pSD->getPath("/plants/meanExtinctionCoefficient");
}
std::string Interception::getName() const {
	return "Interception";
}

DerivativeBase * newInstantiationInterception(
		SimulaDynamic* const pSD) {
	return new Interception(pSD);
}

/*****************************************************************************/
void Interception::calculate(const Time &t, double& E_i) {
/*****************************************************************************/
	/// Input: precipitation, leafAreaIndex, KDF
	///
	/// Output: E_i  in cm/d
	//
	/// --- interception of precipitation ---
	//
	//  interception of precipitation, maybe important later when soil is dry
	//	and conductivity is zero or near zero at the top soil and rain falls
    //  directly evaporated water from canopy surfaces, standing water,
	// 	etc., maybe dew, too

		double p;
		precipitation_->get(t, p);/// cm/d
//		std::cout << "Rainfall " << p << " cm/day" << std::endl;
		double LAI;
		leafAreaIndex_->get(t, LAI);
		double soilCovering;
		double KDF;
		splitparam_->get(t,KDF);


		// Literature: Belmans, Dekker, Bouma, 1982;
		// Obtaining soil physical data for simulating soil moisture regimes and associated potato growth

		if (LAI < 4.) // in Belmans, Dekker, Bouma, 1982 soilCovering is liked to be measured.
		{	soilCovering = 1. - exp(-KDF * LAI);}
		else{
			soilCovering = 1.;}

		if(p < 1.8){ // in cm/day
			E_i = soilCovering* 0.044* pow(p, 0.53 - 0.18*p);
		}else{
			E_i = soilCovering* 0.04966367034; //0.05;
		}
		///daily infiltration (as long as no ponding occured) is P-E_i

} // end calculate E_i


/***************************Interception Version 2***************************/

InterceptionV2::InterceptionV2(SimulaDynamic* const pSV) :
		DerivativeBase(pSV) {
	//check if unit given in input file agrees with this function
	pSD->checkUnit("cm/day");
	precipitation_ 	= pSD->getPath("/environment/atmosphere/precipitation", "cm/day"); ///< only used if interception is modeled
	leafAreaIndex_ 	= pSD->getPath("/plants/meanLeafAreaIndex"); //, "m/s"
	splitparam_		= pSD->getPath("/plants/meanExtinctionCoefficient");
}
std::string InterceptionV2::getName() const {
	return "Interception";
}

DerivativeBase * newInstantiationInterceptionV2(
		SimulaDynamic* const pSD) {
	return new InterceptionV2(pSD);
}

/*****************************************************************************/
void InterceptionV2::calculate(const Time &t, double& E_i) {
/*****************************************************************************/
	/// Input: precipitation, leafAreaIndex, KDF
	///
	/// Output: E_i  in cm/d
	//
	/// --- interception of precipitation ---

		double p;
		precipitation_->get(t, p);/// cm/d
		double LAI;
		leafAreaIndex_->get(t, LAI);
		double soilCovering;
		double KDF;
		splitparam_->get(t,KDF);
		if (LAI < 4.){
			soilCovering = 1. - exp(-KDF * LAI);}
		else{
			soilCovering = 1.;}

		double b = soilCovering;
		if(LAI > 1e-4)
		{
			double a = 0.025/100.;
			E_i = a * LAI * (1.- 1./(1.+(b*p /(a*LAI))));
		}else{
			E_i = 0.;
		}

} // end calculate

//==================registration of the classes=================
class AutoRegisterEvaporationClassInstantiationFunctions {
public:
	AutoRegisterEvaporationClassInstantiationFunctions() {
		BaseClassesMap::getDerivativeBaseClasses()["Interception"] = newInstantiationInterception;
		BaseClassesMap::getDerivativeBaseClasses()["InterceptionV2"] = newInstantiationInterceptionV2;
	}
};

// our one instance of the proxy
static AutoRegisterEvaporationClassInstantiationFunctions p4556745465435;
