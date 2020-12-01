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

#include "../../engine/BaseClasses.hpp"

/**
 * Simple class for estimating the root dryweight from the allocated carbon, not from the root geometry.
 * This is very similar as the stemDryWeight class.
 *
 * Needs a sibling carbonAllocation2Stems
 *
 * Optionally uses carbonToDryWeightRatio three levels up, fall back is 0.45 g C/g DW
 *
 */

class RootDryWeight:public DerivativeBase {
public:
	RootDryWeight(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *c2sSimulator, *CinDryWeight;
};


RootDryWeight::RootDryWeight(SimulaDynamic* const pSV):
	DerivativeBase(pSV),c2sSimulator(nullptr), CinDryWeight(nullptr)
{
	//check if unit given in input file agrees with this function
	pSD->checkUnit("g");
	//plant parameters
	c2sSimulator=pSD->getSibling("carbonAllocation2Roots","g");
	//carbon conversion factor.
	CinDryWeight=pSD->getParent(3)->existingChild("carbonToDryWeightRatio","100%");
}
void RootDryWeight::calculate(const Time &t, double& cal){
		//get portion of stem allocated carbon
		c2sSimulator->getRate(t,cal);
		//C in dry weight
		double cdw;
		if(CinDryWeight){ ///todo instead of existing, we could create a database::get("param",number) where is it is missing number will be inserted as a simulaconstant.
			CinDryWeight->get(t,cdw);
		}else{
			cdw=0.45;//backwards compatible default
		}
		cal/=cdw;
}


std::string RootDryWeight::getName()const{
	return "rootDryWeight";
}
DerivativeBase * newInstantiationRootDryWeight(SimulaDynamic* const pSV){
   return new RootDryWeight(pSV);
}

static class AutoRegisterRateClassInstantiationFunctionsDryWeight324 {
public:
   AutoRegisterRateClassInstantiationFunctionsDryWeight324() {
	  BaseClassesMap::getDerivativeBaseClasses()["rootDryWeight"] = newInstantiationRootDryWeight;
   };
} p97873969763;



