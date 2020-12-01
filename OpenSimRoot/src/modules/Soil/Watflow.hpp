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

#ifndef WATFLOW_HPP_
#define WATFLOW_HPP_

#include "../SimRootTypes.hpp"

#include "../../engine/SimulaTable.hpp"

#include "../../math/SparseSymmetricMatrix.hpp"

#include "Mesh.hpp"

#include <deque>
#include "Material.hpp"


//*************************************************
//todo can be removed?
class Water {
public:
	Water(SimulaDynamic* const pSV);
	std::string getName() const;
protected:
	void calculate(const Time &t, double& theta);
};
//*************************************************

class Watflow {
public:
	//todo no idea why this passes a pSD pointer.
	Watflow(SimulaDynamic* const pSD);
	~Watflow() {
		//if(Balance_out.is_open()) Balance_out.close();
		delete SSM;
	}


	void calculate(tIndex & ItCum, const tIndex & MaxIt, double & dt,
			const double & dtMin, const double & dtOld, const double & tOld,
			const tIndex & TLevel, tIndex & Iter);

	const tVector & getConOld() const {
		//par.calc_fk(hOld, ConO);//todo this is used by solute, which interpolates the values. Possibly we could just run this on interpolated solute values.
		return ConO;
	}
	const tVector & getCon() const {
		return Con;
	}

	const tVector & getQ()const { return Q; }
	const double  & getQ(const Coordinate &pos)const {
		tIndex index;
		waterMesh.getIndex(pos, index);
		return Q[index]; }
	const tVector & gethNew()const { return hNew; }
	const tVector & gethOld()const { return hOld; }
	const tVector & getthNew()const { return thNew; }
	const double & getthNew(const Coordinate &pos)const {
		tIndex index;
		waterMesh.getIndex(pos, index);
		return thNew[index]; }
	const tVector & getthOld()const { return thOld; }

	double getthTopav() const {
		const std::valarray<tIndex> & tbn(waterMesh.getTopBoundaryNodes());
	    const double thTop_av = thNew[tbn].sum()/thNew[tbn].size();
		return thTop_av;
	}

	const double & getthOld(const Coordinate &pos)const {
		tIndex index;
		waterMesh.getIndex(pos, index);
		return thOld[index]; }

	const tVector & getthSat()const { return par.getthSat(); }

	const VanGenuchten & getPar() const { return par;}
	const tVector & getwSink() const{ return Sink;}
	const Mesh_base & getMesh()const { return waterMesh; }

	void updateLists(const Time &t, const std::deque<SimulaBase *> & rootNodeQueue_);


private:

	void reportTotWaterColumn(const Time&t, const double & dt)const;

	double getrTop() const {
		return rTop;
	}

	void setHydraulicHead(Mesh_base &mesh);
	void setrootsurfacevalues(const Time & t,double dtMin, unsigned int j=0.);
	void setWaterSink(const double & t0, const double & t1); // tOld and t
	void set_atmosphere(const Time & t);

	void calc_wCumA_wCumT(const double & dt);

	Mesh6 waterMesh;

	bool AtmInF;
	bool explic;
	bool FreeD; //free drainage
	const tIndex NumNP;
	const tIndex NumEl;
	const tIndex NumBP;
	tVector Q; // in ml/day

	SparseSymmetricMatrix *SSM;

	VanGenuchten par;

	//input of soil parameters
	SimulaBase *thetaDry_, *thetaFieldCapacity_, *soilScalingExponent_,
			*thetaR_, *thetaSat_, *depthOfTheta_;
	SimulaBase *prec_, *interception_, *evaporation_,
			*potentialEvaporation_, *evaporationEn_, *depthWaterTable_;



	const tVector & x;
	const tVector & y;
	const tVector & z;

	tVector hNew;
	tVector hOld;
	tVector hTemp;
	int matchmode;
	tVector B, RHS;
	tVector B1;
	tVector DS;
	tVector Sink;
	tVector Con;
	tVector ConO;//todo, this is mostly here so that solute can work with a weighted average of the conductivity. We can easily remove this
	tVector Cap;
	tVector thOld;
	tVector thNew;
	tVector Adiag_boundary_nodes_;

	double TolTh;
	double TolH;
	const tFloat & rLen;

	double  rTop,prec, rSoil;

	double hCritS;
	double hCritA;

	double VolumeOfDomain;
	const tIndex Nsub;

	void neumann_boundary_condition();
	void computeQBackwards(double dt);
	void dirichlet_boundary_condition(const Time &t);

	void reset_system(const double & told, const double & dt,const tIndex & Iter);

	// ------BOUNDARY ------- EVAPORATION:
	//void getPotentialTranspiration(const double& t,double & Tpot)const;
	void getscaling(const double& th, double& scaling);


	std::deque<SimulaBase *> waterUptake_;
	std::deque<SimulaTable<Time>*> volumetricWaterContent_;
	std::deque<SimulaTable<Time>*> hydraulicHead_;
	std::deque< std::vector<int> > femIndexes_;
	std::deque< std::vector<double> > femWeights_;

	//output to tables
	SimulaTable<Time> *totWaterInColumn, *totWaterChange, *sumTopBoundaryFlux_, *sumBottomBoundaryFlux_, *totSink_;

};

class WaterMassBalanceTest: public DerivativeBase {
public:
	WaterMassBalanceTest(SimulaDynamic* pSD);
	std::string getName() const;
protected:
	void calculate(const Time &t, double &var);

	SimulaBase *totWaterChange, *sumTopBoundaryFlux_, *sumBottomBoundaryFlux_, *totalWaterUptake_, *totSink_;
	SimulaTable<Time> *relMassBalanceError_;
	double ref;//reference for relative balance error

};


#endif
