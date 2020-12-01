/*
Copyright © 2016, The Pennsylvania State University
All rights reserved.

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
#include "CarbonBalance.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../engine/Origin.hpp"
//#include "../../DataDefinitions/Units.hpp"
#include "../PlantType.hpp"
//#include "Constants.hpp"
#include <math.h>

//BALANS CALCULATION: - TOTAL CARBON USED BY RESPIRATION
//                    - TOTAL CARBON USED FOR GROWTH
//                    + TOTAL CARBON PRODUCED
//                    BALANS CHECK
//TODO currently there is no way to force this check, check will only be performed when user is requesting it in output
PlantCarbonBalance::PlantCarbonBalance(SimulaDynamic* pSD) :
	DerivativeBase(pSD) {
	//plant respiration simulator in g carbon
	rSim = pSD->existingSibling("plantRespiration", "g");
	//exudates
	eSim = pSD->existingSibling("rootCarbonCosts", "g");
	if(!eSim) eSim = pSD->existingSibling("rootCarbonCostOfExudates", "g");
	//reserves/storage
	sSim = pSD->existingSibling("carbonReserves", "g");

	//plant carbon production in g carbon
	cSim = pSD->getSibling("plantCarbonIncome", "g");
	caSim = pSD->getSibling("carbonAvailableForGrowth", "g");
	//plant dry weight in g dryweight
	dSim = pSD->getSibling("plantDryWeight", "g");
	///@todo: no litter or root turn over included.

	//just for checking more in detail
	raSim = pSD->getSibling("carbonAllocation2Roots", "g");
	rwSim = pSD->getSibling("rootDryWeight", "g");
	rl1Sim = pSD->getSibling("rootLength", "cm");
	rl2Sim = pSD->getSibling("rootLongitudinalGrowth", "cm");
	saSim = pSD->getSibling("carbonAllocation2Shoot", "g");
	swSim = pSD->getSibling("shootDryWeight", "g");
	rraSim = pSD->getSibling("relativeCarbonAllocation2Roots", "100%");
	rsaSim = pSD->getSibling("relativeCarbonAllocation2Shoot", "100%");

	//carbon conversion factor.
	CinDryWeight=pSD->getSibling("carbonToDryWeightRatio","100%");

}

///@todo fix storage and exudate modules
#define RELATIVE 100/tinc
void PlantCarbonBalance::calculate(const Time &t, double &balance) {
	//current cumulative income
	balance=0.;
	double tinc;
	cSim->get(t, tinc);
	if (tinc != 0) {
		double tdw, tresp(0), texu(0), tstore(0);
		//current total weight in carbon
		dSim->get(t, tdw);
		//current cumulative respiration
		if (rSim)
			rSim->get(t, tresp);
		//current cumulative spendature on exudates
		if (eSim)
			eSim->get(t, texu);
		//current stored carbon
		if (sSim)
			sSim->get(t, tstore);
		//C in dry weight
		double cdw;
		CinDryWeight->get(t,cdw);
		//balance
		balance = tinc - ((tdw * cdw) + tresp + texu + tstore);//tinc - ((tdw * cdw) + tresp + texu + tstore);
		//relative error
		double terr(balance * RELATIVE);
		//try to be more verbose about where the possible error is
		//dry weight of roots versus total allocated carbon
		double ra, rdw;
		raSim->get(t, ra);
		rwSim->get(t, rdw);
		double rerr((ra - (rdw * cdw)) * RELATIVE);
		//dry weight of shoot versus total allocated carbon
		double sa, sdw;
		saSim->get(t, sa);
		swSim->get(t, sdw);
		double serr((sa - (sdw * cdw)) * RELATIVE);
		//allocated versus income
		double allocerr((tinc - (sa + ra + tresp + texu + tstore)) * RELATIVE);
		double ca;
		caSim->get(t, ca);
		std::size_t nmbpred=pSD->getPredictorSize();
		double caerr((ca + tresp + texu + tstore - tinc) * RELATIVE);//error in available carbon
		if (fabs(caerr) > 5e-1 && nmbpred==pSD->getPredictorSize())
			msg::warning("PlantCarbonBalance: carbonAvailableForGrowth!=plantCarbonIncome-plantRepiration-exudates-storage for "+pSD->getParent()->getName(), -1);
		double rra, rsa;
		rraSim->get(t, rra);
		rsaSim->get(t, rsa);
		if (fabs(rra + rsa - 1.0) > 5e-1) {
			msg::warning("PlantCarbonBalance: ignoring balance since relative carbon allocation does not add up to 1 for "+pSD->getParent()->getName(), -1);
			return;
		}
		double aerr(sa + ra - ca);
		if (fabs(aerr) > 5e-1) {
			msg::warning("PlantCarbonBalance: ignoring balance since carbon allocation does not add up to 100% for "+pSD->getParent()->getName(), -1);
			//std::cout<<"\n cbalanace t, sa, ca, ratio:"<<t<<'\t'<<sa<<'\t'<<ca<<'\t'<<sa/ca;
			return;
		}
		if ( fabs(rerr) > 0.5 && fabs(terr-rerr) < 1e-2){
			double rl1,rl2;
			rl1Sim->get(t,rl1);
			rl2Sim->get(t,rl2);
			rl1=(rl1-rl2)/rl2;
			if(fabs(rl1)>0.1){
				msg::warning("PlantCarbonBalance: Carbon balance is off due to an inaccuracy in root length. Compare rootLength with rootLongitudinalGrowth for "+pSD->getParent()->getName(), -1);
				//return;
			}
		}
		//total dryweight check
		double dwerr((tdw - (rdw + sdw)) * RELATIVE);
		//generate error message
		if (fabs(terr) > 0.5 || fabs(rerr) > 0.5 || fabs(serr) > 0.5 || fabs(allocerr) > 0.5
				|| fabs(dwerr) > 0.5) {
			std::string errormsg("PlantCarbonBalance: balance error at " + convertToString(t, 2) + " days: error (%)");
			errormsg += " tot=" + convertToString(terr, 4);
			errormsg += " root=" + convertToString(rerr, 4);
			errormsg += " alloc=" + convertToString(allocerr, 4);
			if (fabs(serr) > 1e-2)
				errormsg += " shoot=" + convertToString(serr, 4);
			if (fabs(dwerr) > 1e-2)
				errormsg += " sum dw=" + convertToString(dwerr, 4);
			errormsg += " for "+pSD->getParent()->getName();
			msg::warning(errormsg, -1);
		}
	}
}

std::string PlantCarbonBalance::getName()const{
	return "plantCarbonBalance";
}

DerivativeBase * newInstantiationPlantCarbonBalance(SimulaDynamic* const pSD) {
	return new PlantCarbonBalance(pSD);
}


//registration of classes
class AutoRegisterCarbonBalance {
public:
	AutoRegisterCarbonBalance() {
		BaseClassesMap::getDerivativeBaseClasses()["plantCarbonBalance"] = newInstantiationPlantCarbonBalance;
	}

}static p;

