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
 * CSRmatrix.cpp
 *
 *  Created on: April 13, 2015
 *      Author: Christian
 */


#include "CSRmatrix.hpp"

#include <iostream>


CSRmatrix::CSRmatrix() {
	s = new csr_sparse; // empty struct
}

// store matrix
CSRmatrix::CSRmatrix(std::size_t _nval, std::size_t _nrow, double values[], IndexType ia[], IndexType ja[]) {
	s = new csr_sparse;
	(*s).values = new double[_nval];
	(*s).ia = new std::size_t[_nrow+1]; // row pointer
	(*s).ja = new std::size_t[_nval];   // column index

	 memcpy((*s).values, values, _nval*sizeof(double));
	 memcpy((*s).ia, ia, (_nrow+1)*sizeof(std::size_t));
	 memcpy((*s).ja, ja, _nval*sizeof(std::size_t));

	(*s).numCols = _nrow; // quadratic !!!
	(*s).numRows = _nrow;
	(*s).numVal  = _nval;
}

CSRmatrix::CSRmatrix(std::size_t _nval, std::size_t nRows, std::size_t nCols) {
	s = new csr_sparse;
	(*s).values = new double[_nval];
	(*s).ia = new std::size_t[nRows+1];
	(*s).ja = new std::size_t[_nval];
	(*s).numCols = nRows;
	(*s).numRows = nRows;
	(*s).numVal = _nval;
}

CSRmatrix::CSRmatrix(const csr_sparse& t)// copy t to s
{
	 s = new csr_sparse;
	 (*s).numCols = t.numCols;
	 (*s).numVal = sizeof(t.values)/sizeof(t.values[0]);
	 (*s).numRows = sizeof(t.ia)/sizeof(t.ia[0])-1;
	 (*s).values = new double [(*s).numVal];
	 (*s).ia = new std::size_t [(*s).numRows+1];
	 (*s).ja = new std::size_t [(*s).numVal];
	 memcpy((*s).values, t.values, (*s).numVal*sizeof(double));
	 memcpy((*s).ia, t.ia, (*s).numRows*sizeof(std::size_t));
	 memcpy((*s).ja, t.ja, (*s).numVal*sizeof(std::size_t));
}

CSRmatrix::~CSRmatrix()
{
 	delete [] (*s).values;
 	delete [] (*s).ia;
 	delete [] (*s).ja;
 	delete s;
}


void CSRmatrix::print_sparse(std::ostream& os) const{

	    using std::endl;

	    os << "CSR Storage " << (*s).numRows << " x " << (*s).numCols  << ", #values = " << (*s).numVal << endl;

	    for ( IndexType i = 0; i < (*s).numRows; i++ )
	    {
	        os << "Row " << i << " ( " << (*s).ia[i] << " - " << (*s).ia[i + 1] << " ) :";

	        for ( IndexType jj = (*s).ia[i]; jj < (*s).ia[i + 1]; ++jj )
	        {
	            os << " " << (*s).ja[jj] << ":" << (*s).values[jj];
	        }
	        os << endl;
	    }
}


void CSRmatrix::get_diag_ptr(std::valarray<IndexType> & diag_ptr) const {
	IndexType j;
	for (IndexType i = 0; i < (*s).numRows; ++i) {
		j = (*s).ia[i];
		while ((*s).ja[j] < i) {
			++j;
		}
		diag_ptr[i] = j;
	}
}

void CSRmatrix::get_diagonal(std::valarray<double>& diag_val) const {
	IndexType j;
	for (IndexType i = 0; i < (*s).numRows; ++i) {
		j = (*s).ia[i];
		while ((*s).ja[j] < i) {
			++j;
		}
		diag_val[i] = (*s).values[j];
	}
}


/*** consider a matrix split as A = D + L + U, in diagonal, lower and upper triangular part, \n
 *   and an incomplete factorization preconditioner of the form P = (D+L)D⁻1(D+U) = (I + L D⁻1)(D+U) = (D+L)(I+D⁻1 U).
 *   In this way, we only need to store a diagonal matrix D containing the pivots of the factorization.
 *
 *   Hence,it suffices to allocate for the preconditioner only a pivot array of length n_rows.
 *   we will store the inverses of the pivots rather than the pivots themselves.
 *   This implies that during the system solution no divisions have to be performed.
 */
void CSRmatrix::D_ILU(std::valarray<double> & pivots) const { // only allow fill-in on the diagonal
	std::valarray<IndexType> diag_ptr((*s).numRows);
	get_diag_ptr(diag_ptr);
	bool found;
	double element;
	for (IndexType i = 0; i < (*s).numRows; ++i) {
		pivots[i] = 1. / (*s).values[diag_ptr[i]]; // Each elimination step starts by inverting the pivot

		// For all nonzero elements a_ij with j > i,
		// we next check whether a_ji is a nonzero matrix element, since this is the only element that can cause fill with a_ij.
		for (IndexType j = diag_ptr[i] + 1; j < (*s).ia[i + 1]; ++j) {
			found = false;
			for (IndexType k = (*s).ia[(*s).ja[j]]; k < diag_ptr[(*s).ja[j]]; ++k) {
				if ((*s).ja[k] == i) {
					found = true;
					element = (*s).values[k];
				}
			}
			// if so, we update a_jj
			if (found == true) {
				pivots[(*s).ja[j]] -= element * pivots[i] * (*s).values[j];
			}
		}
	}
}


/** The system LUy = x can be solved in the usual manner by introducing a temporary vector z:
 *  Lz = x, Uy = u
 *  This is for solving P = (D+L)(I+D⁻1 U)
 */
void CSRmatrix::LUsolve(const std::valarray<double> & pivots,const std::valarray<double> & x, const IndexType diag_ptr[], std::valarray<double> & y) const {
	y = 0.;
	std::valarray<double> z(y);
	double sum = 0.;
	for(IndexType i = 0; i < (*s).numRows; ++i){
	    sum =  0.;
	    for(IndexType j = (*s).ia[i]; j<diag_ptr[i]; ++j){
	        sum += (*s).values[j] * z[(*s).ja[j]];
	    }
	    z[i] = pivots[i] * (x[i]-sum);
	}
	for(int i = (*s).numRows; i >= 0; --i){
	    sum = 0.;
	    for(IndexType j = diag_ptr[i]+1; j < (*s).ia[i+1]; ++j){
	        sum += (*s).values[j] * y[(*s).ja[j]];
	        y[i] = z[i] - pivots[i] * sum;
	    }
	}
}
