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

#include "MichealisMenten.hpp"

#include "../SimRootTypes.hpp"

#include "../../engine/Origin.hpp"
#include "../../engine/SimulaTable.hpp"
#include "../PlantType.hpp"


MichaelisMenten::MichaelisMenten(SimulaDynamic* pSD) :
		TotalBaseLabeled(pSD){
	//plant type
	std::string plantType;
	PLANTTYPE(plantType,pSD);
	//root type
	std::string rootType;
	pSD->getParent(4)->getChild("rootType")->get(rootType);
	//get nutrient
	std::string nutrient(pSD->getParent()->getName());
	//plant parameters
	//SimulaBase *param(GETROOTPARAMETERS(plantType,rootType));

	//TODO assuming diameter to be constant
	diameter=pSD->getParent(2)->getChild("rootDiameter", "cm");

	//length
	if(pSD->getUnit()=="umol"){
		length=pSD->getParent(2)->getChild("rootSegmentLength", "cm");
	}else if (pSD->getUnit()=="umol/cm"){
		length=nullptr;
	}else{
		msg::error("MichaelisMenten: unknown unit "+pSD->getUnit().name+". Expected umol or umol/cm");
	}

	//mm parameters
	sImax=pSD->getSibling("Imax", "umol/cm2/day");
	sKm=pSD->getSibling("Km", "umol/ml");
	sCmin=pSD->getSibling("Cmin", "umol/ml");

	//concentration at root surface
	surfconc=pSD->getSibling("nutrientConcentrationAtTheRootSurface","umol/cm3");

	//optional volumetric water content
	surfWaterContent=pSD->getParent(2)->existingChild("volumetricWaterContentAtTheRootSurface","cm3/cm3");

	//optional roothairs
	roothairs=pSD->getParent(2)->existingChild("rootHairSurfaceArea","cm2/cm");
	if(!roothairs) msg::warning("MichaelisMenten: no roothairs found. Using root surface area without that of root hairs");
	
	//turn this off for base segments
	if(pSD->getParent(2)->getName()=="dataPoint00000"){
		sImax=nullptr;
	}
}

void MichaelisMenten::calculate(const Time &t, double &fluxDensity) {
	if(!sImax){//base of the root is inside parent root and does not take up nutrients.
		fluxDensity=0;
		return;
	}
	//Get the different parameters
	double Cmin;
	sCmin->get(t,Cmin);

	//get concentration at root surface
	double conc;
	surfconc->get(t,conc);

	//check the equation for fluxDensity, compare with eq. 5.7 of Barber
	conc-=Cmin;//5* for soft landing
	if(conc<=0){
		fluxDensity=0;
	}else{
		//for debuggin only
		//surfconc->dumpXML(std::cout);
		//Get the different parameters
		double Imax,Km;
		sImax->get(t,Imax);
		sKm->get(t,Km);

		//circumference of root segment
		double circ;
		diameter->get(t,circ);
		circ*=PI;

		//roothairs
		if(roothairs){
			double d;
			roothairs->get(t,d);
			circ+=d;
		}

		fluxDensity=circ*Imax*conc/(Km+conc);
		if(std::isnan(fluxDensity)) {
			msg::error("MichaelisMenten: Fluxdensity is not a number. Debugging info: "
					+std::to_string(circ)+" "
					+std::to_string(Imax)+" "
					+std::to_string(conc)+" "
					+std::to_string(Km));
			fluxDensity=0;
		}
	}

	if(length){
		double l;
		length->get(t,l);
		fluxDensity*=l;
	}

//todo took this out as it seems to limit uptake much more than intended
//it seems that fluxes can be large compared to current concentrations.
	if(surfWaterContent && fluxDensity>0){
		//get watercontent
		double th;
		surfWaterContent->get(t,th);
		//limit uptake to available nutrients
		///@todo assuming here that a node has 1cm3
		Time depletiontime(0.5);
		double ul(th*conc/depletiontime);
		if(th>0 && ul<fluxDensity){
			fluxDensity=ul;
			//msg::warning("MichealisMenten: limiting nutrient uptake, for soil is close to empty");
		}
	}

	if(fluxDensity<0)
		msg::error("MichealisMenten: negative results for nutrient uptake");
}
std::string MichaelisMenten::getName()const{
	return "michaelis_menten_nutrient_uptake";
}


DerivativeBase * newInstantiationMichaelisMenten(SimulaDynamic* const pSD) {
	return new MichaelisMenten(pSD);
}




//==================registration of the classes=================
class AutoRegisterRateClassInstantiationMichaelis_menten {
public:
   AutoRegisterRateClassInstantiationMichaelis_menten() {
	  BaseClassesMap::getDerivativeBaseClasses()["michaelis_menten_nutrient_uptake"] = newInstantiationMichaelisMenten;
   }
};


// our one instance of the proxy
static AutoRegisterRateClassInstantiationMichaelis_menten p435486354532727;
