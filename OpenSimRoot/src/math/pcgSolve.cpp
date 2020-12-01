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
 * pcgSolve.cpp
 *
 *  Created on: April 29, 2014
 *      Author: Christian
 */
#include "pcgSolve.hpp"

#include <iomanip>      /* std::setprecision */
#include <iostream>
#include <math.h> /*fabs*/

/*
* pcgSolve.cpp
*
*  Created on: April 29, 2014
*      Author: Christian
*
*
* Reference:
* inspired of pseudecode by J. Shewchuk, page 51 of
* http://www.cs.cmu.edu/~quake-papers/painless-conjugate-gradient.pdf
*
*/

void Pcg::solve(const SparseSymmetricMatrix& A, const std::valarray<double>& b,
		std::valarray<double>& x, const double tolerance,
		const std::size_t maxK, Solvertype type) {

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
	// Start solution, rhs
	//x = b;

	std::valarray<double> Ad(n);
	std::valarray<double> d(n);
	std::valarray<double> r(n);
	std::valarray<double> s(n);

	// Start residual
	A.vectorMultiply(x, r);
	r = b - r;

/*	workaround: this should be catched in while condition for tolerance
	if(maxNorm(r)<10e-6) {
		std::cout<<std::endl<<"returning old x" ;
		//return;
	}
*/

	double delta_new = 0.;
	double delta_old = 0.;
	double delta_0 = 0.;
	double alpha = 0.;
	double beta = 0.;
	double inner = 0.;

	// for convergence speed output, use either ref or norm_0
					double ref = 0;
					const size_t refIter= 2;
//					double norm_0 = L2Norm(r);

	switch (type) {
	// -----------------------------------------------------------!
	// SSOR implementation is experimental(!), use SGS instead!
	// -----------------------------------------------------------!
//	case SSOR: {
//		/// apply preconditioner:  d = P^-1 r
//		/********************************************************/
//		const double w = 1.6;
//		A.solvePreconditionSSOR(w, r, d);
//		/********************************************************/
//		delta_new = innerProduct(r, d);
//		delta_0 = delta_new;
//		std::size_t k = 0;
//		while (/*k < 5 ||*/(delta_new > tolerance * tolerance * delta_0
//				&& k < maxK)) {
//			A.vectorMultiply(d, Ad);
//			inner = innerProduct(d, Ad); //should be positive, because A should be positive definite
//			if (inner == 0 ) { // compiler should optimize this
//				//msg::error("pcgSolver: Matrix is numerically semi definite and division by a very small value occured. d'*A*d = " +std::to_string(inner));
//				return;// we have a solution, residual is zero
//			}
//			alpha = delta_new / inner;
//			x += alpha * d;
//			if (k % 20 == 0 && k > 0) {
//				A.vectorMultiply(x, r);
//				r = b - r; // recalculate the exact residual once every 20 iterations to remove accumulated floating point error.
//			} else {
//				r -= alpha * Ad; // fast recursive formula for the residual
//			}
//			//apply preconditioner: s = P^-1 r
//			/***********************************/
//			A.solvePreconditionSSOR(w, r, s);
//			/***********************************/
//			delta_old = delta_new;
//			delta_new = innerProduct(r, s);
//			beta = delta_new / delta_old;
//			d = s + beta * d;
//			++k;
//			if(k == refIter) ref= L2Norm(r);
//		}
//		//if(k>50) msg::warning("pcgSolver: using more than 50 iterations"); // this warning is not necessary
//		if (k == maxK) {
//			A.vectorMultiply(x, r);
//			r = b - r;
//			double norm_k = L2Norm(r);
//			double crate=pow(norm_k / ref, 1. / (k-refIter));
//			msg::error("pcgSolver: did not find a solution; Covergence Rate is "+std::to_string(crate));
//		}
//		// For output:
//		A.vectorMultiply(x, r); r = b - r; double norm_k = L2Norm(r);
////		//std::cout << "L1norm " << L1Norm(r) << std::endl;
//		std::cout<< std::endl << "L2norm " << L2Norm(r) << std::endl;
//		std::cout << "Steps: " << k << std::endl;
//		std::cout << "Convergence Rate: " << pow(norm_k / ref, 1. / (k-refIter)) << std::endl;
//		std::cout << std::endl;
//		break;
//	} /// end case
	case SYM_GAUSS_SEIDEL: {
		/// apply preconditioner:  d = P^-1 r
		/********************************************************/
		/// solve L(D^-1)R d = r
		///	L: lower triangular matrix of A
		///	R: upper
		/// Idea: solve Ld=r then compute y=Dd then solve Rd=y
		//
		// solve Ld = r , get d
		A.solveLowerTriangle(r, d);
		// y = Dd
		std::valarray<double> y(n);
		A.diagonal(y);
		y = y * d;
		// solve Rd = y , get d
		A.solveUpperTriangle(y, d);
		/********************************************************/
		delta_new = innerProduct(r, d);
		//delta_0 = delta_new; // OLD VERSION
		std::size_t k = 0;
		// if delta zero is small we have a solution,
		while (/*k < 5 ||*/(delta_new > tolerance*n  && k < maxK)) { //OLD STOPING CONDITION: delta_new > tolerance * tolerance * delta_0
			A.vectorMultiply(d, Ad);
			inner = innerProduct(d, Ad); //should be positive, because A should be positive definite
			if (inner==0) {// compiler should optimize this
				//msg::error("pcgSolver: Matrix is numerically semi definite and division by a very small value occured. d'*A*d = " +std::to_string(inner));
				return; // we have a solution, residual is zero
			}
			alpha = delta_new / inner;
//			if (alpha > 10e2) {
//							msg::warning("pcgSolver: Alpha is very large  " +std::to_string(alpha));
//						}
			x += alpha * d;
			if (k % 20 == 0 && k > 0) {
				A.vectorMultiply(x, r);
				r = b - r; // recalculate the exact residual once every 50 iterations to remove accumulated floating point error.
			} else {
				r -= alpha * Ad; // fast recursive formula for the residual
			}
			//apply preconditioner: s = P^-1 r
			/***********************************/
			A.solveLowerTriangle(r, s);
			A.diagonal(y);
			y *= s;
			A.solveUpperTriangle(y, s);
			/***********************************/
			delta_old = delta_new;
			delta_new = innerProduct(r, s);
			beta = delta_new / delta_old;
			d = s + beta * d;
			++k;
			if(k == refIter) ref= L2Norm(r);
		}
		// if(k>50) msg::warning("pcgSolver: using more than 50 iterations");
		if (k == maxK) {
			A.vectorMultiply(x, r);
			r = b - r;
			double norm_k = L2Norm(r);
			double crate=pow(norm_k / ref, 1. / (k-refIter));
			msg::error("pcgSolver: did not find a solution; Covergence Rate is "+std::to_string(crate));
		}
		// For output:
//		A.vectorMultiply(x, r); r = b - r; double norm_k = L2Norm(r);
//		//std::cout << "L1norm " << L1Norm(r) << std::endl;
//		std::cout<< std::endl << "L2norm " << L2Norm(r) << std::endl;
//		std::cout << "Steps: " << k << std::endl;
//		std::cout << "Convergence Rate: " << std::setprecision(5) << pow(norm_k / ref, 1. / (k-refIter)) << std::endl;
//		std::cout << "Dimension: " << n << std::endl;
		break;
	} /// end case
	case DEFAULT:
	case JACOBI:
	default: {
		msg::warning("PCG: Jacobi Preconditioner is used ");
		//apply preconditioner
		/******************************/
		std::valarray<double> Pinv(n);
		A.diagonal_inverse(Pinv);
		d = Pinv * r;
		/******************************/
		delta_new = innerProduct(r, d);
		delta_0 = delta_new;
		std::size_t k = 0;
		while (/*k < 5 ||*/(delta_new > tolerance * tolerance * delta_0
				&& k < maxK)) {
			A.vectorMultiply(d, Ad);
			inner = innerProduct(d, Ad);
			if (inner == 0) {
				// msg::error("pcgSolver: Matrix is numerically semi definite and division by zero could occur.");
				return; // we have a solution, residual is zero
			}
			alpha = delta_new / inner;
			x += alpha * d;
			if (k % 20 == 0 && k > 0) {
				A.vectorMultiply(x, r);
				r = b - r;
			} else {
				r -= alpha * Ad;
			}
			//apply preconditioner
			/************************/
			s = Pinv;
			s *= r; // element wise
			/************************/
			delta_old = delta_new;
			delta_new = innerProduct(r, s);
			beta = delta_new / delta_old;
			d = s + beta * d;
			++k;
			if(k == refIter) ref = L2Norm(r);
		}
		if(k==maxK){
			A.vectorMultiply(x, r);
			r = b - r;
			double norm_k = L2Norm(r);
			double crate=pow(norm_k / ref, 1. / (k-refIter));
			msg::error("pcgSolver: did not find a solution; Covergence Rate is "+std::to_string(crate));
		}
		//A.vectorMultiply(x, r); r = b - r; double norm_k = L2Norm(r);
		//std::cout << "Steps: " << k << std::endl;
		//std::cout << "Convergence Rate: " << pow(norm_k / norm_0, 1. / (k-refIter)) << std::endl;
		break;
	} ///end default
	} ///end switch
//	std::cout<< "MaxNorm: " << maxNorm(r)<<std::endl;
//	std::cout<< "Average Error: " << L1Norm(r)/n<<std::endl;
} // end solve

//***************************************************************************/
double Pcg::innerProduct(const std::valarray<double>& a, const std::valarray<double>& b) {
	double product = 0;

	//if (a.size() != b.size()) msg::error("Pcg::innerProduct: Dimensions don't match");
	std::size_t n = a.size();

	for (std::size_t i = 0; i < n; ++i){
		product += a[i] * b[i];
	}
	return product;
}
//***************************************************************************/
double Pcg::maxNorm(const std::valarray<double>& a) {
	std::size_t n = a.size();
	if (n == 0)	return 0.;
	double min = fabs(a.min());
	double max = fabs(a.max());
	double norm = max > min ? max : min;
	return norm;
}
//***************************************************************************/
double Pcg::L1Norm(const std::valarray<double>& a) {
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
double Pcg::L2Norm(const std::valarray<double>& a) {
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
