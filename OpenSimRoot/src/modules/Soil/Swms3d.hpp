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

#ifndef SWMS3D_HPP_
#define SWMS3D_HPP_

#include "Output.hpp"
#include "Solute.hpp"
#include "Watflow.hpp"

//todo: to behave nicely, this should have been declared as an integration class.
//todo: now dt for water and nutrients are coupled
class Swms3d: public DerivativeBase {
public:

	Swms3d(SimulaDynamic* const pSD);
	~Swms3d(){
     // close Output Files
		if(outputVTK){
			delete outputVTK;
			outputVTK=nullptr;
		}

		delete water;
		for(auto it:solute)	delete it;
	}

	static void terminate(){
		if(outputVTK) {
			delete outputVTK;
			outputVTK=nullptr;
		}
	};

	void calculate(const Time &time, double &dt);
	std::string getName()const;
	void addObject(SimulaBase *rootNode);

	static Watflow* getWaterPointer() {return primaryWater_;}

protected:
	static std::deque<SimulaBase *> rootNodeQueue_;

private:
	void initialize_();

	void getTimeStepSWMS(double & deltaT, const int& Iter,const double& tPrint,const double& tAtm,const double& lastTime,const double& dtMaxC);

	static Watflow* primaryWater_;
    Watflow *water;
    double dt;
    std::vector<Solute *> solute; //all the solute models
    static OutputSoilVTK  *outputVTK;

    tIndex ItCum;
    std::size_t Iter;
    std::size_t TLevel;
	std::size_t PLevel;

    double cumMineralisation;

    double dtOpt;
    double dtOld;
    double tOld;
    double tAtm;

    std::size_t MaxIt;

    Time TPrint, pdt;
    bool printVTU;


    };

class GetValuesFromSWMS:public DerivativeBase{
public:
	GetValuesFromSWMS(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *link;
};


#endif
