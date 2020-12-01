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

#ifndef NutrientUptake_HPP_
#define NutrientUptake_HPP_

#include "../GenericModules/Totals.hpp"
#include "../../engine/SimulaTable.hpp"
#include <string>
#include <valarray>
#include <fstream>

class Barber_cushman_1981_nutrient_uptake_ode23:public TotalBaseLabeled{
public:
	Barber_cushman_1981_nutrient_uptake_ode23(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
};

class BarberCushmanSolverOde23:public IntegrationBase{
public:
	BarberCushmanSolverOde23();
	~BarberCushmanSolverOde23();
	std::string getName()const;
	virtual void predict(const Time&,double &);
	virtual void predictRate(const Time&,double &);
protected:
	typedef std::valarray<double > Array;
	void doDelayedConstruction();
	virtual void integrate(SimulaVariable::Table & data, DerivativeBase & rateCalculator);
    double  dr,b,Ci,r0,r1,circ,rootHairDensityConversionFactor,Cmin,res, increaseTimeStep;
	static double theta;
	SimulaBase *sKm, *sImax, *sDe, *sv0, *competition, *slh, *sNh, *srh0, *rootDiameter, *spatialRootDensity, *rootLength;
	SimulaTable<Time> *depletionZone;
	Coordinate depth;
	Time lt;
	std::size_t k; //Array::size_type k;
	std::valarray<double> X_old;
	int msgcount;
	inline double calcCh(const double &C,const double &Km,const double &Cmin,const double &S4);
	inline double calcIh(const double &Ch,const double &Imax,const double &Km,const double &Cmin,const double &Ah);
	inline void rk_step(std::valarray<double> &res, const std::valarray<double> &X,const std::valarray<double> &w1,const std::valarray<double> &w2,const std::valarray<double> &w3,const std::valarray<double> &w4,const double &De, const double &b, const double &Cmin, const double &r0, const double &v0, const double &Km,const double &Imax ,const double &dr, const std::valarray<double> &IhRK,const std::size_t &k,const std::valarray<double> &r);
	inline void calcIhRK(std::valarray<double> &res, const std::valarray<double> &X, const std::valarray<double> &S4, const double &Imax, const std::valarray<double> &Ah, const double &Km, const double  &Cmin, const std::size_t &k);
private:
	static double L2Norm(const std::valarray<double> &a);
	/* constants */


	double h, h_new;

	std::ofstream os;

	int mode;
	Time refTImax,refTKm;

};


class Barber_cushman_1981_nutrient_uptake_explicit:public TotalBaseLabeled{
public:
	Barber_cushman_1981_nutrient_uptake_explicit(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
};

class BarberCushmanSolver:public IntegrationBase{
public:
	BarberCushmanSolver();
	~BarberCushmanSolver();
	std::string getName()const;
	virtual void predict(const Time&,double &);
	virtual void predictRate(const Time&,double &);
protected:
	typedef std::valarray<double > Array;
	void doDelayedConstruction();
	virtual void integrate(SimulaVariable::Table & data, DerivativeBase & rateCalculator);
    double  dr,b,Ci,r0,r1,circ,rootHairDensityConversionFactor,Cmin,res;
	static double theta;
	SimulaBase *sKm, *sImax, *sDe, *sv0, *competition, *slh, *sNh, *srh0, *rootDiameter, *spatialRootDensity, *rootLength;
	SimulaTable<Time> *depletionZone;
	Coordinate depth;
	std::valarray<double> X_old;
	int msgcount;
	inline double calcCh(const double &C,const double &Km,const double &Cmin,const double &S4);
	inline double calcIh(const double &Ch,const double &Imax,const double &Km,const double &Cmin,const double &Ah);
	inline void rk_step(std::valarray<double> &res, const std::valarray<double> &X,const std::valarray<double> &w1,const std::valarray<double> &w2,const std::valarray<double> &w3,const std::valarray<double> &w4,const double &De, const double &b, const double &Cmin, const double &r0, const double &v0, const double &Km,const double &Imax ,const double &dr, const std::valarray<double> &IhRK,const std::size_t &k,const std::valarray<double> &r);
	inline void calcIhRK(std::valarray<double> &res, const std::valarray<double> &X, const std::valarray<double> &S4, const double &Imax, const std::valarray<double> &Ah, const double &Km, const double  &Cmin, const std::size_t &k);
	inline double L2Norm(const std::valarray<double> &a);
private:
	/* constants */
	static const double A11;
	static const double A21;
	static const double A22;
	static const double A31;
	static const double A32;
	static const double A33;
	static const double A41;
	static const double A42;
	static const double A43;
	static const double A44;
	static const double A51;
	static const double A52;
	static const double A53;
	static const double A54;
	static const double A55;

	/* order 5 */
	static const double B11;

	static const double B13;
	static const double B14;

	static const double B16;

	/* order 4 */
	static const double B21;

	static const double B23;
	static const double B24;
	static const double B25;
	static const double B26;

	static const double C1;
	static const double C2;
	static const double C3;
	static const double C4;
	static const double C5;

	static bool MIXED;
	static bool automatic_startstep;
	bool firststep;

	std::ofstream os;

	int mode;
	Time refTImax,refTKm;

};



//calculate fluxdensity at any point
class Barber_cushman_1981_nutrient_uptake:public TotalBaseLabeled{
public:
	Barber_cushman_1981_nutrient_uptake(SimulaDynamic* pSD);
	~Barber_cushman_1981_nutrient_uptake();
	std::string getName()const;
protected:
	typedef std::valarray<double > Array;
	void calculate(const Time &t,double &var);
	double  dr,b,Ci,r0,r1,circ,rootHairDensityConversionFactor,Cmin,res,increaseTimeStep;
	static double theta;
	int mode;
	SimulaBase *sKm, *sImax, *sDe, *sv0, *competition, *slh, *sNh, *srh0, *rootDiameter, *spatialRootDensity, *rootLength;
	Time refTImax,refTKm;
	SimulaTable<Time> *depletionZone;
	Coordinate depth;
	Time lt;
	unsigned int k;
	Array Cn;
	int msgcount;
	std::ofstream os;
	inline double calcCh(const double &C,const double &Km,const double &Cmin,const double &S4);
	inline double calcdCh(const double &C,const double &Km,const double &Cmin,const double &S4);
	inline double calcIh(const double &Ch,const double &Imax,const double &Km,const double &Cmin,const double &Ah);
	inline double calcdIh(const double &Ch,const double &dCh,const double &Imax,const double &Km,const double &Cmin,const double &Ah);
};

class BiologicalNitrogenFixation:public DerivativeBase {
public:
	BiologicalNitrogenFixation(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *optimal;
	double ratio;
};

class Imax:public DerivativeBase {
public:
	Imax(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *imax, *rcs, *rcseffect;
};


//class integrates fluxdensity over the length of a root segment thus calculating nutrient uptake by a segment
/*class SpatialIntegral:public TotalBaseLabeled {
public:
	SpatialIntegral(SimulaDynamic* const pSV);
	std::string getName()const;
protected:
	void calculate(const Time &t, double&);
	SimulaBase *current, *next1, *next2,  *length;
};*/

class SegmentMaxNutrientUptakeRate:public TotalBaseLabeled{
public:
	SegmentMaxNutrientUptakeRate(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase  *surfaceArea;
};

//calculate depletion zones as done in the SimRoot 4 -
/* SimRoot 4 claims this is based on:
 * Architectural Analysis of plant root systems
 * 1. Architectural coorelates of exploitation efficiency
 * A.H. Fitter, T.R. Stickland, M.L. Harvey
 * New Phytology(1991), 118, pp 375-382
 */
class RadiusDepletionZoneSimRoot4:public DerivativeBase{
public:
	RadiusDepletionZoneSimRoot4(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *rootDiameterSimulator;
	double diffusionCoefficient;
};
class RadiusDepletionZoneBarberCushman:public DerivativeBase{
public:
	RadiusDepletionZoneBarberCushman(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *link;
};


class RootSegmentNutrientDepletionVolume:public TotalBaseLabeled{
public:
	RootSegmentNutrientDepletionVolume(SimulaDynamic* pSD);
	std::string getName()const;
protected:
	void calculate(const Time &t,double &var);
	SimulaBase *volSim,*lengthSim,*current,*next1,*next2;
};

//nutrient requirements for growth of this root - based on optimal growth

//nutrient status


#endif /*NutrientUptake_HPP_*/
