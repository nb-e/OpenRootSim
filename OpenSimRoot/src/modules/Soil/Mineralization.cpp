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


#include "Mineralization.hpp"


Mineralization::Mineralization(const Mesh_base &mesh)
{

	auto &x(mesh.getCordX());
	auto &y(mesh.getCordY());
	auto &z(mesh.getCordZ());

	// allocate r9
	int size_z = z.size();
	r9.resize(size_z);

	SimulaBase* p;
	p = ORIGIN->getPath(
			"environment/soil/organic/initialRelativeMineralisationRate");
	double Y(0.0);
	for (int i = 0; i != size_z; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		r9[i] = Y;
	}

	// eliminate deeper layers without mineralization from loop
	numl = size_z ;
	for (int i = size_z-1; i >= 0; --i) {
		if (r9[i] > 1e-4) {
			numl = i;
			break;
		}
	}

	orgcpl.resize(numl);
	orgnpl.resize(numl);
	fv.resize(numl);
	soa.resize(numl);

	//get parameters from simroot database
	p = ORIGIN->getPath(
			"environment/soil/organic/carbonContent");
	for (unsigned int i = 0; i != numl; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		orgcpl[i] = Y;
	}

	p = ORIGIN->getPath("environment/soil/bulkDensity");
	for (unsigned int i = 0; i != numl; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		orgnpl[i] = Y;
	}

	orgcpl *= 1.e6 * orgnpl; // orgcpl=1e6*orgcpl*orgnpl ! convert g/g to ug/cm3: (g/g)*(g/cm3)=g/cm3

	p = ORIGIN->getPath("environment/soil/organic/CNratio");
	for (unsigned int i = 0; i != numl; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		orgnpl[i] = Y;
	}

	orgnpl = orgcpl / orgnpl; //convert cn ratio to absolute N pools

	p = ORIGIN->getPath("environment/soil/organic/speedOfAging");
	for (unsigned int i = 0; i != numl; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		soa[i] = Y;
	}
	double X(0);

	p = ORIGIN->getPath("environment/soil/organic/assimilationEfficiencyMicrobes");
	p->get(X, Y);
	da = Y;

	p = ORIGIN->getPath("environment/soil/organic/CNRatioMicrobes");
	p->get(X, Y);
	cnm = Y;

	p = ORIGIN->getPath("environment/soil/organic/timeOffset");
	p->get(X, Y);
	ltd = Y;
	ltd /= 365.;
	ltdpre=ltd;
}

void Mineralization::mineralisationYang1994(const double & dt, const double &reduct,const tVector &prhead, tVector &rndiss){
	// output:  rndiss
	if(rndiss.size()!=numl) rndiss.resize(numl);

	//local
	double dtsoa,K,fdiss,cdiss,cass,cconv,nass,nconv;
//	double dum; // set butnot used

	//currently ltd just follows normal time and we assume that soa is for the given temperature (there is no temperature in the model)
	//in the future we can use janssens temperature function, or that of somebody else (janssen is for when soa is calibrated at 9 degrees C)
	ltdpre = ltd;
	ltd = ltdpre + (dt / 365.);  //time of the mineralisation model is in years

	     //moisture effect
	     soilMoistureEffectsOnMineralisation(prhead,fv);
	     //yangs mineralisation model

	     for(unsigned int i=0; i < numl; ++i){
	         if(r9[i] < 1.e-4) continue;
	         if(orgcpl[i] == 0.) continue;
//	         dum = orgcpl[i];
//	         dum = orgnpl[i];
	         dtsoa   = std::pow(ltd,(1-soa[i])) - std::pow(ltdpre,(1-soa[i]));
	         K       = -r9[i]*dtsoa;
	         fdiss   = std::exp(K)-1.;
	         cdiss   = reduct*fv[i]*fdiss*orgcpl[i];
	         //assimilation rate en total C conversion
	         cass    = cdiss*da;
	         cconv   = cdiss + cass;
	         //Nitrogen assimilation and total Nconversion
	         nass    = cass/cnm;
	         nconv   = cconv*orgnpl[i]/orgcpl[i];
	         //Nitrogen dissimilation
	         rndiss[i]  = nconv-nass;
	         //integrate with forward euler
	         orgcpl[i] += cdiss;
	     }

	    orgnpl += rndiss;
	    rndiss /= dt;
	    // todo, would be good to check decline in organic N with solute balance

}

void Mineralization::soilMoistureEffectsOnMineralisation(const tVector & prhead, tVector & fv){
	   //result: fv

	   double prhd;

	   for(tIndex i = 0; i < fv.size(); ++i){
	     prhd = -0.01 * prhead[i];
	     if(prhd > 10.) {
	    	 fv[i] = 0.;
	     } else if (prhd > 0.85) {
	    	 fv[i] = 0.97-0.118*std::log(prhd);
	     } else if (prhd>0.33) {
	    	 fv[i] = 1.;
	     } else if (prhd>0.07) {
	    	 fv[i] = 1.51+0.453*std::log(prhd);
	     } else {
	    	 fv[i] = 0.;
	     }
	   }

}
