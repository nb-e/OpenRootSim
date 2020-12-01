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

#ifndef MINERALIZATION_HPP_
#define MINERALIZATION_HPP_

#include <fstream>
#include "../../engine/Origin.hpp"
#include "../../engine/SimulaDynamic.hpp"
#include "../../engine/SimulaTable.hpp"
#include "../GenericModules/Totals.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../engine/DataDefinitions/Time.hpp"

#include "Mesh.hpp"


class Mineralization {
public:
	Mineralization(const Mesh_base &mesh);
	~Mineralization(){}
	unsigned int getNumNodesWithMineralization()const{ return numl;};

	/**
	* mineralisation using simple emperical model from yang 1994 (wageningen, phd) \n
	* Yang HS, Janssen BH. 2000. A mono-component model of carbon mineralization with a dynamic rate constant. European Journal of Soil Science 51: 517-529. \n
	* see also yang's phd thesis \n
	* DA         = Dissimilation/Assimilation ratio of microorganisms, 2 for fungi (for each C that is incorporated, one C becomes CO2) \n
	* CNM        = C/N ratio micororganisms, 10 for fungi, 5 for bacteria \n
	* SOA        = Speed of aging, Green manure=0.64,Straw=0.66,Roots=0.67,FYM=0.49,SOM=0.46 \n
	* R9         = The initial relative mineralisation rate in (YEAR**(SOA-1)). Green manure=1.39,Straw=1.11,Roots=0.80,FYM=0.82,SOM=0.057 (0.037 estimate for experimental farm 'vredepeel' based on potential mineralisation measurements)\n
	* ltd(pre)   = time corrected for temperature see Janssen, B.H., 1986. Een een-parametermodel voor de berekening van de decompositie van organisch materiaal. Vakblad voor Biologen. 66 (20):433-436 \n
	* reduct     = reduction coefficient due to lack of mineral N (only used when N fixation occurs)
	* org(c/n)pl = organic C and N pool \n
	* rndiss     = result: n mineralisation rate - rndiss is negative when mineralisation occurs and positive when immobilization occurs. \n
	* dt         = timestep for integration
	*/
	void mineralisationYang1994(const double & dt,const double &reduct,const tVector &prhead, tVector &rndiss);
private:

	/** moisture function \n
	* this is a 'feddes' like function for mineralisation \n
	* Reddy, K.R., R. Khaleel, M.R., overcash, P.W.Westerman, 1979. A nonpoint source model for land areas receiving animal wastes: I. Mineralisation of organic nitrogen. Transations of the ASAE 22, 863-872 \n
	* Cited by: Antonopoulos, V.Z., Comparison of different models to simulate soil temperature and moisture effects on nitrogen mineralization in the soil. Journal of Plant Nutrition and Soil science 162:667-675 \n
	* prhead = pressure head in hPa (hydraulic head)
	*/
	void soilMoistureEffectsOnMineralisation(const tVector & prhead, tVector & fv);


 tVector fv, r9, orgcpl,orgnpl,soa; //we may want to export this at some time to the vtk output?
 double da,cnm;
 double ltd ,ltdpre;  //start after one day
 unsigned int numl;
};

#endif
