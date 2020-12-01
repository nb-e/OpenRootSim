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
#ifndef MATHLIBRARY_HPP_
#define MATHLIBRARY_HPP_

#if _WIN32 || _WIN64
#define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <vector>
#include <valarray>

#include "../cli/Messages.hpp"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

//=========mathematical constants=================
#define PI M_PI


//===============boundary=========================
//returns true when a value is within bounds
template <class T>
bool withinBounds(const T &lower, const T &upper, const T &value){
	if(lower<upper){//natural order
		if (value<=upper && value>=lower) return true; else return false;
	}else{//opposite order
		if (value>=upper && value<=lower) return true; else return false;
	}
}

#define close2zero(NUMBER) (NUMBER > -1e-7 && NUMBER < 1e-7)

//maximum and minimum

template <class T>
const T& maximum(const T& a, const T& b){return (a > b ? a : b);}

template <class T>
const T& minimum(const T& a, const T& b){return (a < b ? a : b);}

#define maximum(a, b) (((a) > (b)) ? (a) : (b))
#define minimum(a, b) (((a) < (b)) ? (a) : (b))
#define absolute(X) (((X) < (0)) ? ((X)*-1) : (X))

//===============other=========================
#define average(X,Y) (((X)+(Y))/2)
#define surfaceAreaCircle(X) ((X)*(X)*PI)
#define square(X) ((X)*(X))

double betacf(double a, double b, double x);
double incompleteBetaFunction(double a,double b,double x);

enum MeanType {GEOMETRIC, LOGARITHMIC};

double kMean(MeanType meanType, double k1, double k2);

//solving quadratic formula: returns 0 upon success, 1 or 2 when solution does not exist.
int quadraticFormula(const double & a,const double & b,const double & c,double & sol1,double & sol2);

//matrix calculations
void tridiagonalSolver(const double & bl , const double &a, const double &b,const double &c, const double &br, const std::vector<double> &d, std::vector<double> & x);
void tridiagonalSolver(std::vector<double> & adiag, const std::vector<double> & aleft, std::vector<double> & arite, std::vector<double> &x);
void tridiagonalSolver(std::valarray<double> & adiag, const std::valarray<double> & aleft, std::valarray<double> & arite, std::valarray<double> &x);

void thomasBoundaryCondition(std::valarray<double> & a, std::valarray<double>& b,std::valarray<double> & c,std::valarray<double> & d, std::valarray<double> & x);

template<class TArray>
void tridiagonalSolver(const TArray & a, const TArray & b, const TArray & c,const TArray & f, TArray &y){
	//dimensions of the matrix (-1 because in c first element is 0)
	const std::size_t n(y.size());
	//if (n<2) msg::error("tridiagonalSolver: matrix should be at least 3x3");
	//if(b.size() != c.size() ||  a.size() != f.size() || a.size()!=b.size()) msg::error("tridiagonalSolver: vectors of unequal length");
	//set
	TArray w(n,1e-6),v(n,0),z(n,0);
	w[0]=a[0];
	if(w[0]==0) w[0]=1e-6;
	v[0]=c[0]/w[0];
	z[0]=f[0]/w[0];
	//forward
	for (std::size_t i=1; i < n; i++ ){
		w[i] = a[i] - (b[i]*v[i-1]);
		if(w[i]==0) w[i]=1e-6;
		v[i] = c[i]/w[i];
		z[i] = (f[i]-(b[i]*z[i-1]))/w[i];
	}
	//backward
	y[n-1]=z[n-1];
	for(int i=(int)n-2 ; i >= 0 ; --i)	{
		y[i] = z[i] - (v[i] * y[i+1]);
	}
}


//code for arrays
double elementMultiplication(std::vector<double> &d);
double vectorsMinimum(std::vector<double> &d);
double vectorsMaximum(std::vector<double> &d);
double vectorsAverage(std::vector<double> &d);
double vectorsMaxRelativeDeviationFromOne(std::vector<double> &d);


#endif /*MATHLIBRARY_HPP_*/

