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
#include "Material.hpp"

#include "../../math/MathLibrary.hpp"

#include <math.h>


VanGenuchten::VanGenuchten(const Mesh_base &mesh) {
	myNumNP = mesh.getNumNP();
	tha.resize(myNumNP);
	thr.resize(myNumNP);
	ths.resize(myNumNP);
	thm.resize(myNumNP);
	thk.resize(myNumNP);
	alpha.resize(myNumNP); // TODO why is this a vector ? Answ.: Can have different soil types in a soil domain
	n.resize(myNumNP);     // TODO why is this a vector ?
	m.resize(myNumNP);     // TODO why is this a vector ?
	kk.resize(myNumNP);
	ks.resize(myNumNP);


	const tVector & x = mesh.getCordX();
	const tVector & y = mesh.getCordY();
	const tVector & z = mesh.getCordZ();

	SimulaBase* p = ORIGIN->getPath("environment/soil/water/residualWaterContent");
	tFloat Y;
	for (tIndex i = 0; i != myNumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		tha[i] = Y;
		thr[i] = Y; // standard van genuchten
	}

	p = ORIGIN->getPath("environment/soil/water/saturatedWaterContent");
	for (tIndex i = 0; i != myNumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		ths[i] = Y;
		thm[i] = Y; // standard van genuchten thm=ths
		thk[i] = Y; // measured conductivity (not used), used if we have different "cut offs"
	}

	p = ORIGIN->getPath(
			"environment/soil/water/vanGenuchten:alpha");
	for (tIndex i = 0; i != myNumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		alpha[i] = Y;
	}

	p = ORIGIN->getPath("environment/soil/water/vanGenuchten:n");
	for (tIndex i = 0; i != myNumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		n[i] = Y;
	}

	pm_ = ORIGIN->existingPath("environment/soil/water/vanGenuchten:m");
	if(pm_){
		for (tIndex i = 0; i != myNumNP; ++i) {
			p->get(0, Coordinate(x[i], z[i], y[i]), Y);
			m[i] = Y;
		}
	}else{
		m = 1. - 1. / n;
	}

	p = ORIGIN->getPath("environment/soil/water/saturatedConductivity");
	for (tIndex i = 0; i != myNumNP; ++i) {
		p->get(0, Coordinate(x[i], z[i], y[i]), Y);
		Y*=mesh.getpotmask()[i];
		ks[i] = Y; // Ks
		kk[i] = Y; // measured conductivity (not used)

	}

	const tFloat zero = 0.;
	const tFloat one  = 1.;

	hSat.resize(myNumNP);
	thR.resize(myNumNP);
	thSat.resize(myNumNP);
	tVector	Hr(myNumNP);
	calc_fh(zero, Hr);
	calc_fh(one, hSat);
	calc_fq(Hr, thR);
	thSat = ths; // is the same as	calc_fq(zeros, thSat);

	return;
}


// old version
// =============================================================================
//void VanGenuchten::calc_fk(const tVector &h, tVector & fk) const {
//	double Kr, HMin, HH, Qees, Qeek, Hs, Hk, Qee, Qe, Qek, FFQ, FFQk;
//	for (tIndex i = 0; i < myNumNP; ++i) {
//		HMin = -std::pow(1.e300, (1. / n[i])) / std::fmax(alpha[i], 1.0);
//		HH = std::fmax(h[i], HMin);
//		Qees = std::fmin((ths[i] - tha[i]) / (thm[i] - tha[i]),
//				.999999999999999);
//		Qeek = std::fmin((thk[i] - tha[i]) / (thm[i] - tha[i]), Qees);
//		Hs = -1. / alpha[i] * std::pow((std::pow(Qees, (-1. / m[i])) - 1.), (1. / n[i]));
//		Hk = -1. / alpha[i] * std::pow((std::pow(Qeek, (-1. / m[i])) - 1.), (1. / n[i]));
//		if (h[i] < Hk) {
//			Qee = std::pow((1. + std::pow((-alpha[i] * HH), n[i])), (-m[i]));
//			Qe = (thm[i] - tha[i]) / (ths[i] - tha[i]) * Qee;
//			Qek = (thm[i] - tha[i]) / (ths[i] - tha[i]) * Qeek;
//			FFQ = 1. - std::pow((1. - std::pow(Qee, (1. / m[i]))), m[i]);
//			FFQk = 1. - std::pow((1. - std::pow(Qeek, (1. / m[i]))), m[i]);
//			if (FFQ < 0.0) {
//				FFQ = m[i] * std::pow(Qee, (1. / m[i]));
//			}
//			Kr = std::pow((Qe / Qek), 0.5) * std::pow((FFQ / FFQk), 2.0) * kk[i] / ks[i];
//			fk[i] = std::fmax(ks[i] * Kr, 1.e-37);
//		} else {
//			if (h[i] < Hs) {
//				Kr = (1. - kk[i] / ks[i]) / (Hs - Hk) * (h[i] - Hs) + 1.0;
//				fk[i] = ks[i] * Kr;
//			} else {
//				fk[i] = ks[i];
//			}
//		}
//	}
//}
// =============================================================================

double VanGenuchten::computeConductivity(const double & currentSe, const double & m, const double & n, const double & Ks)const{
    double p = m + 1. / n;
    double q = 1. - 1. / n;
    double z = std::pow(currentSe, (1. / m));
    double myBeta = incompleteBetaFunction(p, q, z);
    return(Ks * currentSe * (myBeta*myBeta));
}

void VanGenuchten::calc_fk(const tVector &h,const tVector &waterContent, tVector & fk) const {
	// if separate m is used
	if(pm_){
	    tVector conductivity(0.,myNumNP);
	    double currentSe;
	    for(tIndex i = 0; i < myNumNP; ++i  ){
	    	currentSe = (waterContent[i] - thr[i]) / (ths[i] - thr[i]);
	        fk[i] = computeConductivity(currentSe, m[i], n[i], ks[i]);
	    }
	}
	else{
		tVector S = std::pow( - alpha * h,n);
		tVector Se = 1.+S;
		for(tIndex i = 0; i < myNumNP; ++i  ){
			fk[i] = ks[i] * std::pow(1. - (S[i] / Se[i]), 2. )  / std::pow(Se[i] , m[i]*0.5) ;
		}
	}
}

void VanGenuchten::calc_fc(const tVector &h, tVector & fc) const {
	double HMin, HH, Qees, Hs, C1, C2;
	for (tIndex i = 0; i < myNumNP; ++i) {
		HMin = -std::pow(1.e300, (1. / n[i])) / std::fmax(alpha[i], 1.);
		HH = std::fmax(h[i], HMin);
		Qees = std::fmin((ths[i] - tha[i]) / (thm[i] - tha[i]),
				.999999999999999);
		Hs = -1. / alpha[i]
				* std::pow((std::pow(Qees, (-1. / m[i])) - 1.), (1. / n[i]));
		if (h[i] < Hs) {
			C1 = std::pow((1. + std::pow((-alpha[i] * HH), n[i])), (-m[i] - 1.));
			C2 = (thm[i] - tha[i]) * m[i] * n[i] * (std::pow(alpha[i],n[i])) * std::pow((-HH), (n[i] - 1.)) * C1;
			fc[i] = std::fmax(C2, 1.e-37);
		} else {
			fc[i] = 0.0;
		}
	}
}
// ***********************************************************************
//  FQ is for theta
void VanGenuchten::calc_fq(const tVector &h, tVector &fq) const {
	double HMin, HH, Qees, Hs, Qee;
	for (tIndex i = 0; i < myNumNP; ++i) {
		HMin = -std::pow(1.e300, (1. / n[i])) / std::fmax(alpha[i], 1.);
		HH = std::fmax(h[i], HMin);
		Qees = std::fmin((ths[i] - tha[i]) / (thm[i] - tha[i]), .999999999999999);
		Hs = -1. / alpha[i] * std::pow((std::pow(Qees, (-1. / m[i])) - 1.), (1. / n[i]));
		if (h[i] < Hs) {
			Qee = std::pow((1. + std::pow((-alpha[i] * HH), n[i])), (-m[i]));
			fq[i] = std::fmax(tha[i] + (thm[i] - tha[i]) * Qee, 1.e-37);
		} else {
			fq[i] = ths[i];
		}
	}
}
// old version
// =============================================================================
// determines h (water potential) used in constructor only.
//void VanGenuchten::calc_fh(const double &Qe, tVector &fh ) {
//	double HMin, QeeM, Qee;
//	for (tIndex i = 0; i < myNumNP; ++i) {
//		HMin = -std::pow(1.e300, (1. / n[i])) / std::fmax(alpha[i], 1.);
//		QeeM = std::pow((1. + std::pow((-alpha[i] * HMin), n[i])), (-m[i]));
//		Qee  = std::fmin(std::fmax(Qe * (ths[i] - tha[i]) / (thm[i] - tha[i]), QeeM), 0.999999999999999);
//		fh[i] = std::fmax( -1. / alpha[i] * std::pow((std::pow(Qee, (-1. / m[i])) - 1.), (1. / n[i])), -1.e37);
//	}
//}
// =============================================================================


void VanGenuchten::calc_fh(const double &Qe, tVector &fh){
	double hh, hhh;
	for (tIndex i = 0; i < myNumNP; ++i) {
		hh = (ths[i]-thr[i])/(Qe-thr[i]);
        hhh = std::pow(hh,(1./m[i]));
        fh[i] = - ( std::pow((hhh - 1.),(1./n[i])) / alpha[i]);
	}
}

