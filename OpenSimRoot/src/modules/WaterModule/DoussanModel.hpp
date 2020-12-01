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
#ifndef WaterUptake_HPP_
#define WaterUptake_HPP_
#include "../../engine/SimulaDynamic.hpp"
#include "../../engine/SimulaTable.hpp"
#include "../../math/SparseSymmetricMatrix.hpp"



class LateralHydraulicConductivity:public DerivativeBase{
public:
	LateralHydraulicConductivity(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *size, *cond, *RCSstage, *RCSeffect ;
	bool inverse;
};

class WaterUptakeDoussanModel:public DerivativeBase{
public:
	WaterUptakeDoussanModel(SimulaDynamic* pSD);
	//~WaterUptakeDoussanModel();
	virtual std::string getName()const;
	virtual void addObject(SimulaBase *newObject);
protected:
	enum Connections{down,up,branch,seed,collar};
	virtual void calculate(const Time &t,double &var);

	void findConnections();
	void build(const Time &t);
	void writeValues(const Time &t);


	std::string plantType_;
	SimulaBase *potentialTranspiration;
	SimulaTable<double> *waterPotentialAtTheCollar;
	std::vector<SimulaTable<double>*> wuList,xpList;
	std::vector<SimulaBase*>  KhList, LrList, psi_sList;
	std::vector<int> ni; //neighbor index
	std::map<SimulaBase*,unsigned int> neighborMap;
	std::valarray<double>
	psi_s,//construct array with root surface water potentials
	Lr,//construct array with root radial conductivities
	b, //right hand side
	psi_x,//solution, xylem potentials
	gravity;//gravity transform
	SparseSymmetricMatrix C;//matrix with conductivity values
	double psiCollar;
	int collarnodeIndex;
	const double largeKh;
	unsigned int nodeCounter;
	double collarKh;
	bool useGravity;
};


class HydraulicConductivityRootSystem:public DerivativeBase{
public:
	HydraulicConductivityRootSystem(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *size, *flowrate, *collarPotential;
};


#endif
