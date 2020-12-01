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

/*

 * BiCGSTAB.cpp
 *
 *  Created on: March 27, 2015
 *      Author: Christian
 *
 *
 *   __ (Pseudo)code and Description: ________________________
 *  | Andreas Meister: Numerik linearer Gleichungssysteme 	|
 *  | Eine Einführung in moderne Verfahren 					|
 *  | Mit MATLAB-Implementierungen von C. Vömel			|
 *  | 4., ueberarbeitete Auflage							|
 *  |_______________________________________________________|
 *  and
 *  the Preconditioned BiCGSTAB follows the algorithm described
 *  described on p. 24 of the SIAM Templates book.
 */

#include "BiCGSTAB.hpp"
#include <iomanip>      /* std::setprecision */
#include <iostream>
#include <math.h> /*fabs*/
#include <string>

void BiCGSTAB::solve(const SparseMatrix& A, const std::valarray<double>& b,
		std::valarray<double>& x, const double tolerance,
		std::size_t maxit, Solvertype type) {

	std::size_t n = A.size();

	// catch inputs
	if (n == 0) {
		return;
	}
	if (b.size() != static_cast<unsigned>(n)) {
		return;
	}
	if (x.size() != n) {
		x.resize(n);
	}

	/*	catch zero solution
	 if(L1norm(b) < 1.e-32) { // check if zero
	 x = 0.0;
	 return;
	 }
	 */

	// Start solution, rhs // this is commented out because we get a start solution as x in the parameter list
	// x = b;
	double tol = tolerance;
	double normr;
	double normb = L2Norm(b);
	if (normb < 1.e-32 ){ normb = 1; }
	double tolb  = tol * normb;


	switch (type) {
	case Jacobi: {
		double resid;
		double rho_1, rho_2(1.), alpha(0.), beta(0.), omega(1.);
		std::valarray<double> phat(0.,n), s(0.,n), shat(0.,n), t(0.,n), v(0.,n), r(n), Pinv(0.,n);

		A.vectorMultiply(x, r);
		r = b - r;
		std::valarray<double> p(r);
		std::valarray<double> rtilde(r);

		for (std::size_t i = 1; i <= maxit; ++i) {
			normr = L2Norm(r);
			if (normr <= tolb) {
				// converged
				return;
			}
			rho_1 = innerProduct(rtilde, r);
			if (fabs(rho_1) < 1.e-32) {
				// tol = L2Norm(r) / normb;
				msg::error(
						"Preconditioned BiCGSTAB break-down! At iteration "
								+ std::to_string(i));
			}
			if (i > 1) {
				beta = (rho_1 / rho_2) * (alpha / omega);
				p = r + beta * (p - omega * v);
			}
			A.diagonal_inverse(Pinv);
			phat = p * Pinv;
			A.vectorMultiply(phat, v);
			alpha = rho_1 / innerProduct(rtilde, v);
			s = r - alpha * v;
			if ((resid = L2Norm(s)) < tolb) {
				x += alpha * phat;
				// tol = resid;
				break;
			}
			A.diagonal_inverse(Pinv);
			shat = s * Pinv;
			A.vectorMultiply(shat, t);
			omega = innerProduct(t, s) / innerProduct(t, t);
			x += alpha * phat + omega * shat;
			r = s - omega * t;

			rho_2 = rho_1;
			if ((resid = L2Norm(r)) < tolb) {
				// tol = resid;
				// maxit = i;
				return;
			}
			if (fabs(omega) < 1.e-32) {
				// tol = L2Norm(r) / normb;
				msg::error(
						"Preconditioned BiCGSTAB break-down! At line "
								+ std::to_string(__LINE__));
			}
		}
		// tol = resid;
		A.vectorMultiply(x, r);
		r = b - r;
		normr = L2Norm(r);
		if (normr > tolb) {
			msg::error("BiCGSTAB not converged, higher maxit needed?");
		}
		break;
	}
	case ILU: {
		std::valarray<double> phat1(n);
		SparseMatrix L(n),U(n);
		A.ILU(L,U);

		double resid;
		double rho_1, rho_2(1.), alpha(0.), beta(0.), omega(1.);
		std::valarray<double> phat(n), s(n), shat(n), t(0.,n), v(0.,n), r(n);

		A.vectorMultiply(x, r);
		r = b - r;
		std::valarray<double> p(r);
		std::valarray<double> rtilde(r);

		for (std::size_t i = 1; i <= maxit; ++i) {
			normr = L2Norm(r);
			if (normr <= tolb) {
				// converged
				return;
			}
			rho_1 = innerProduct(rtilde, r);
			if (fabs(rho_1) < 1.e-32) {
				// tol = L2Norm(r) / normb;
				msg::error(
						"Preconditioned BiCGSTAB break-down! At iteration "
								+ std::to_string(i));
			}
			if (i > 1) {
				beta = (rho_1 / rho_2) * (alpha / omega);
				p -= omega * v;
				p *=beta;
				p += r;
				// p = r + beta * (p - omega * v);
			}
			L.solveLowerTriangle_diag_1(p, phat1);
			U.solveUpperTriangle(phat1, phat);
			A.vectorMultiply(phat, v);
			alpha = rho_1 / innerProduct(rtilde, v);
			s = r - alpha * v;
			if ((resid = L2Norm(s)) < tolb) {
				x += alpha * phat;
				// tol = resid;
				break;
			}
			L.solveLowerTriangle_diag_1(s, phat1);
			U.solveUpperTriangle(phat1, shat);
			A.vectorMultiply(shat, t);
			omega = innerProduct(t, s) / innerProduct(t, t);
			x += alpha * phat + omega * shat;
			r = s - omega * t;

			rho_2 = rho_1;
			if ((resid = L2Norm(r)) < tolb) {
				// tol = resid;
				// maxit = i;
				return;
			}
			if (fabs(omega) < 1.e-32) {
				// tol = L2Norm(r) / normb;
				msg::error(
						"Preconditioned BiCGSTAB break-down! At line "
								+ std::to_string(__LINE__));
			}
		}
		// tol = resid;
		A.vectorMultiply(x, r);
		r = b - r;
		normr = L2Norm(r);
		if (normr > tolb) {
			msg::error("BiCGSTAB not converged, higher maxit needed?");
		}
		break;
	}
	case GaussSeidel:
	{
		  double resid;
		  double rho_1, rho_2(1.), alpha(0.), beta(0.), omega(1.);
		  std::valarray<double>  phat(n), s(n), shat(n), t(n), v(n), r(n);

		  A.vectorMultiply(x, r);
		  r = b - r;
		  std::valarray<double>  p(r);
		  std::valarray<double> rtilde(r);


		  for (std::size_t i = 1; i <= maxit; ++i) {
			normr = L2Norm(r);
			if (normr <= tolb) {
				// converged
				return;
			}
		    rho_1 = innerProduct(rtilde, r);
		    if ( fabs(rho_1) < 1.e-32 ) {
		    	// tol = L2Norm(r) / normb;
		    	msg::error("Preconditioned BiCGSTAB break-down! At iteration "+std::to_string(i));
		    }
		    if (i > 1){
		    	beta = (rho_1/rho_2) * (alpha/omega);
		    	p = r + beta * (p - omega * v);
		    }
		    A.solveLowerTriangle(p,phat);
		    A.vectorMultiply(phat,v);
		    alpha = rho_1 / innerProduct(rtilde, v);
		    s = r - alpha * v;
		    if ((resid = L2Norm(s)) < tolb) {
		      x += alpha * phat;
		      // tol = resid;
		       break;
		    }
		    A.solveLowerTriangle(s,shat);
		    A.vectorMultiply(shat,t);
		    omega = innerProduct(t,s) / innerProduct(t,t);
		    x += alpha * phat + omega * shat;
		    r = s - omega * t;

		    rho_2 = rho_1;
		    if ((resid = L2Norm(r) ) < tolb) {
		    	// tol = resid;
		    	// maxit = i;
		    	return;
		    }
		    if (fabs(omega) < 1.e-32) {
		    	// tol = L2Norm(r) / normb;
		    	msg::error("Preconditioned BiCGSTAB break-down! At line "+std::to_string(__LINE__));
		    }
		  }
		  // tol = resid;
		  A.vectorMultiply(x, r);
		  r = b - r;
		  normr = L2Norm(r);
		  if( normr > tolb ){
		  		msg::error("BiCGSTAB not converged, higher maxit needed?");
		  }
		break;
	}
	case NO_PRECOND:
	default: {
		std::valarray<double> r0(n);
			// Start residual
			A.vectorMultiply(x, r0);
			r0 = b - r0;
			std::valarray<double> r(r0);
			double rr0 = innerProduct(r,r0);
			std::valarray<double> p(r);

			std::valarray<double> v(n);
			std::valarray<double> s(n);
			std::valarray<double> t(n);
		// iterate
		for(std::size_t i = 0; i< maxit; ++i){
			normr = L2Norm(r);
			if(normr <= tolb){
				//converged
				break;
			}
			A.vectorMultiply(p,v);
			double vr0 = innerProduct(v,r0);
			if( std::fabs(vr0)< 1.e-32 ){ // 	check if zero
				msg::error("BiCGSTAB break-down! At line "+std::to_string(__LINE__));
			}
			if( std::fabs(rr0)< 1.e-32 ){ // 	check if zero
				msg::error("BiCGSTAB solution stagnates!");
			}
			double alpha = rr0/vr0;
			s = r - alpha * v;
			A.vectorMultiply(s,t);
			double ts = innerProduct(t,s);
			double tt = innerProduct(t,t);
			if( tt< 1.e-28  || std::fabs(ts)< 1.e-32 ){ // 	check if zero
				msg::error("BiCGSTAB break-down! At line "+std::to_string(__LINE__));
			}
			double omega = ts/tt;
			x += alpha * p + omega* s;
			r = s - omega * t;
			double r1r0 = innerProduct(r,r0);
			double beta = (alpha*r1r0)/(omega*rr0);
			p = r + beta * (p - omega * v);
			rr0 = r1r0;
		}
		A.vectorMultiply(x, r);
		r = b - r;
		normr = L2Norm(r);
		if( normr > tolb ){
			msg::error("BiCGSTAB not converged, higher maxit needed?");
		}
		break;
	}	///end default
	} ///end switch
	return;
} // end solve

void BiCGSTAB::solve(const CSRmatrix& A, const std::valarray<double>& b,
		std::valarray<double>& x, const double tolerance,
		std::size_t maxit) {

	std::size_t n = A.dimensionOfRows();

	// catch inputs
	if (n == 0) {
		return;
	}
	if (b.size() != static_cast<unsigned>(n)) {
		return;
	}
	if (x.size() != n) {
		x.resize(n);
	}

	/*	catch zero solution
	 if(L1norm(b) < 1.e-32) { // check if zero
	 x = 0.0;
	 return;
	 }
	 */

	// Start solution, rhs // this is commented out because we get a start solution as x in the parameter list
	// x = b;
	double tol = tolerance;
	double normr;
	double normb = L2Norm(b);
	if (normb < 1.e-32 ){ normb = 1; }
	double tolb  = tol * normb;

		std::valarray<double> r0(n);
			// Start residual
			A.vectorMultiply(x, r0);
			r0 = b - r0;
			std::valarray<double> r(r0);
			double rr0 = innerProduct(r,r0);
			std::valarray<double> p(r);

			std::valarray<double> v(n);
			std::valarray<double> s(n);
			std::valarray<double> t(n);
		// iterate
		for(std::size_t i = 0; i< maxit; ++i){
			double normr = L2Norm(r);
			if(normr <= tolb){
				//converged
				break;
			}
			A.vectorMultiply(p,v);
			double vr0 = innerProduct(v,r0);
			if( std::fabs(vr0)< 1.e-32 ){ // 	check if zero
				msg::error("BiCGSTAB break-down! At line "+std::to_string(__LINE__));
			}
			if( std::fabs(rr0)< 1.e-32 ){ // 	check if zero
				msg::error("BiCGSTAB solution stagnates!");
			}
			double alpha = rr0/vr0;
			s = r - alpha * v;
			A.vectorMultiply(s,t);
			double ts = innerProduct(t,s);
			double tt = innerProduct(t,t);
			if( tt< 1.e-28  || std::fabs(ts)< 1.e-32 ){ // 	check if zero
				msg::error("BiCGSTAB break-down! At line "+std::to_string(__LINE__));
			}
			double omega = ts/tt;
			x += alpha * p + omega* s;
			r = s - omega * t;
			double r1r0 = innerProduct(r,r0);
			double beta = (alpha*r1r0)/(omega*rr0);
			p = r + beta * (p - omega * v);
			rr0 = r1r0;
		}
		A.vectorMultiply(x, r);
		r = b - r;
		normr = L2Norm(r);
		if( normr > tolb ){
			msg::error("BiCGSTAB not converged, higher maxit needed?");
		}

	return;
} // end solve



//***************************************************************************/
double BiCGSTAB::innerProduct(const std::valarray<double>& a, const std::valarray<double>& b) {
	double product = 0;

	//if (a.size() != b.size()) msg::error("Pcg::innerProduct: Dimensions don't match");
	std::size_t n = a.size();

	for (std::size_t i = 0; i < n; ++i){
		product += a[i] * b[i];
	}
	return product;
}
//***************************************************************************/
double BiCGSTAB::maxNorm(const std::valarray<double>& a) {
	std::size_t n = a.size();
	if (n == 0)	return 0.;
	double min = fabs(a.min());
	double max = fabs(a.max());
	double norm = max > min ? max : min;
	return norm;
}
//***************************************************************************/
double BiCGSTAB::L1Norm(const std::valarray<double>& a) {
	double sum;
	std::size_t n = a.size();
	if (n == 0)
		return 0.;

	sum = fabs(a[0]);

	for (std::size_t i = 1; i < n; ++i){
		sum += fabs(a[i]);
	}
	return sum;
}
//***************************************************************************/
double BiCGSTAB::L2Norm(const std::valarray<double>& a) {
	double sum;
	std::size_t n = a.size();
	if (n == 0){
		return 0.;
	}
	sum = a[0] * a[0];

	for (std::size_t i = 1; i < n; ++i){
		sum += a[i] * a[i];
	}
	sum = sqrt(sum);
	return sum;
}
