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

#ifndef SOLUTE_HPP_
#define SOLUTE_HPP_

#include "../SimRootTypes.hpp"

#include "Mesh.hpp"
#include "../../math/SparseMatrix.hpp"
#include "Material.hpp"
#include "Mineralization.hpp"
#include "Watflow.hpp"


//compiler switches
//#define lUpW  //upwind or not
//#define lArtD  //artificial dispersion or not


class Mesh;

class Solute {
public:

	Solute(const Watflow & water, const std::string & nameNutrient, const int meshrefinement=1);
	~Solute() {
		if(mineral) delete mineral;
	}
	Solute(const Solute& ) = default;
	Solute(Solute&& ) = default;

	void calculate(const double & t, const double & dt); //main calculation routine, forwarding the model one timestep
	void updateLists(const Time &t, const std::deque<SimulaBase *> & rootNodeQueue_);	//new root nodes being added

	//read only interfaces to given components
	const tVector & getConc() const {
		return Conc;
	}
	const Mesh_base & getMesh() const {
		return soluteMesh;
	}
	const tVector & getcSink() const {
		return cSink;
	}
	 double  getMaxTimeStep() const {
		return dtMaxC;
	}
	const std::string & getNameNutrient() const {
		return nutrient;
	}
	void interpolate(const tVector & source, tVector & target, const int factor)const;
	 int  getfactor() const {
		return factor;
	}
	const tVector & getBulkDensity() const {
		return bulkDensity;
	}
	const tVector getSoilBufferPower() const {
		return -Ac; // van Rees: b = theta + bulkDensity * slope of adsorption isotherm of solute
	}
	const tVector & getDiffusion() const {
		return Dif; // effective diffusion of solute
	}

private:

	void reportTotSolidColumn(const Time & t, const tVector & Q, const double & cPrec); //write balance out
	void setSoluteSink(const double & t0, const double & t1, tVector & cSink); //get the root uptake and mineralization
	void setrootsurfacevalues(const Time & t, const unsigned int j=0); //set concentration at the root surface etc
	void peclet_courant(const double & dt, const tVector & theta_, const tVector & Vx, const tVector & Vy, const tVector & Vz); //peclet courant
	void computeS(const bool forward, const double frac, const double dt, const tVector &theta, const tVector &hhead, const tVector &hconductivity,  const tVector & Fc, tVector &B);
#ifdef lUpW
	void upstream_weighing_factors(const int j, const tVector & Vx, const tVector & Vy, const tVector & Vz, tVector &WeTab)const; //upwind weights
#endif
	void dispersion_coeff(const double & dt, const tVector & theta_, const tVector & Vx, const tVector & Vy, const tVector & Vz); //dispersion & diffusion
	void velocities(const tVector & hNew_, const tVector & Con_, tVector & Vx, tVector & Vy, tVector & Vz) const; ///velocity vectors for dispersion
	void cbound_condition(const double t, const double eps, const tVector & Q, tVector & B, double & cPrec);// boundary conditions

	SimulaTable<Time> * d95Solute, *totSoluteInColumn, *totSoluteInColumnDissolved, *totSoluteInColumnAbsorbed, *totSoluteChange, *massBalanceError_,
			 *totSink_, *sumTopBoundaryFlux_, *sumBottomBoundaryFlux_, *totalMineralization_;

	SimulaBase *totalNutrientUptake_, *sPrec_, *sinterception_, *scPrec_;

	const Watflow & water_;
	const int factor;
	Mesh6 soluteMesh;
	Mineralization *mineral; //todo this is just now an extension, but could be a separate model, if simulabase interface would allow

	const tIndex NumNP, NumEl, NumBP;
	const tVector & x, &y, &z;
	SparseMatrix soluteMatrix;
	tFloat PeCr, PeCrMx, Peclet, Courant, dtMaxC; //max timestep allowed according to peclet courant criteria
	tVector Conc; //concentration
	tVector cSink; //solute sink


	tVector Dispxx, Dispyy, Dispzz, Dispxy, Dispxz, Dispyz;
	const tVector & ListNE; /// this is the number of neighbors but needed as double

	//tVector DS;
	tVector bulkDensity, satDiffusionCoeff, longitudinalDispersivity, transverseDispersivity;
	tVector bulkDens_times_adsorpCoeff, adsorptionCoeff; // Adsorption isotherm coefficient, ks

	const std::string nutrient;
	double seedReserves, ConVolI;

	//todo enum would give more readable code here.
	int matchmode; //int indicating how the root nodes should be matched with the solute mesh,
	const tIndex Nsub;
	bool fine;

	std::deque<SimulaBase *> nutrientUptake_;
	std::deque<SimulaTable<Time>*> nutrientConcentration_;
	std::deque<std::vector<int> > femIndexes_;
	std::deque<std::vector<double> > femWeights_;

	tVector Ac; // this is adsorption, and -Ac = b soil buffer power (van Rees)
	tVector Dif;
};

class SoluteMassBalanceTest: public DerivativeBase {
public:
	SoluteMassBalanceTest(SimulaDynamic* pSD);
	std::string getName() const;
protected:
	void calculate(const Time &t, double &var);

	SimulaBase *totSoluteChange, *sumTopBoundaryFlux_, *sumBottomBoundaryFlux_, *totalMineralization_, *totalNutrientUptake_, *totSink_;
	SimulaTable<Time> *massBalanceError_;
	double seedReserves,ref;

};

#endif
