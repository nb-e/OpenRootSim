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
 * CSRmatrix.hpp
 *
 *  Created on: April 13, 2015
 *      Author: Christian
 */


#ifndef CSRMATRIX_H
#define CSRMATRIX_H

#include <valarray>
#include <vector>
#include <iostream>
#include <cstring>
#include "../cli/Messages.hpp"

typedef std::size_t IndexType;
/// set symmetric and non-symmetric flag!

struct csr_sparse{
	double *values = nullptr;
	std::size_t *ia = nullptr;
	std::size_t *ja = nullptr;
	std::size_t numRows = 0;
	std::size_t numCols = 0;
	std::size_t numVal = 0;
	};


class CSRmatrix {

public:
	CSRmatrix();
	CSRmatrix(std::size_t _nval, std::size_t _nrow, double values[], IndexType ia[], IndexType ja[]);
	CSRmatrix(std::size_t _nval, std::size_t nRows, std::size_t nCols);
	CSRmatrix(const csr_sparse& t); //copy
	~CSRmatrix();

	double  operator() (std::size_t i, std::size_t j) const;
	double  getValue(std::size_t i, std::size_t j)    const;

	std::size_t dimensionOfRows() const;
	std::size_t sizeOfvalues() const;
	void resetToZero();

	void vectorMultiply(const IndexType numRows, const IndexType ia[], const IndexType ja[], const double values[],  const std::valarray<double>& v,std::valarray<double>& result) const;
	void vectorMultiply(const std::valarray<double>& v, std::valarray<double>& result) const;

	void forwardElimination(std::valarray<double>& rhs) const; /// solve lower triangle
	void backwardElimination(std::valarray<double>& rhs) const; /// solve upper triangle

	void ILU(CSRmatrix & L, CSRmatrix & U) const;

	void get_diagonal(std::valarray<double>& diag_val) const;
	void get_diag_ptr(std::valarray<IndexType>& diag_ptr) const;

	void D_ILU(std::valarray<double> & pivots) const;
	void LUsolve(const std::valarray<double> & pivots,const std::valarray<double> & x, const IndexType diag_ptr[], std::valarray<double> & y) const;

	void print_sparse(std::ostream& os=std::cout) const;

private:
	csr_sparse *s;

};


inline void CSRmatrix::resetToZero(){ // neighboring stays the same
	std::size_t n = sizeof((*s).values)/sizeof((*s).values[0]);
	std::memset(s->values, 0., n*sizeof(double));
}


inline std::size_t CSRmatrix::sizeOfvalues() const{
	std::size_t n = sizeof((*s).values)/sizeof((*s).values[0]);
	return n;
}

inline std::size_t CSRmatrix::dimensionOfRows() const{
	std::size_t n = sizeof((*s).ia)/sizeof((*s).ia[0]);
	return n;
}


/***************
// read
 ****************************/
inline double CSRmatrix::operator()(std::size_t i, std::size_t j) const {
	double myValue = 0.;

    // "search column in ja from ia[i] : ia[i + 1]

    for ( IndexType jj = (*s).ia[i]; jj < (*s).ia[i + 1]; ++jj )
    {
        IndexType col = (*s).ja[jj];

        if(col < (*s).numCols){
        	msg::error("column index at pos "+std::to_string( jj )+" = " +std::to_string(col)+" out of range");
        }

        if ( col == j )
        {
            // "found column j  at  jj  value =  values[jj]
            myValue = (*s).values[jj];
            break;
        }
    }

    return myValue;
}

inline double CSRmatrix::getValue(std::size_t i, std::size_t j) const {

	    double myValue = 0;

	    // "search column in ja from ia[i] : ia[i + 1]

	    for ( IndexType jj = (*s).ia[i]; jj < (*s).ia[i + 1]; ++jj )
	    {
	        IndexType col = (*s).ja[jj];

	        if(col < (*s).numCols){
	        	msg::error("column index at pos "+std::to_string( jj )+" = " +std::to_string(col)+" out of range");
	        }

	        if ( col == j )
	        {
	            // "found column j  at  jj  value =  values[jj]
	            myValue = (*s).values[jj];
	            break;
	        }
	    }

	    return myValue;
}



// overloaded method if arrays are given directly
inline void CSRmatrix::vectorMultiply(const IndexType numRows,
		const IndexType ia[], const IndexType ja[], const double values[],
		const std::valarray<double>& v, std::valarray<double>& result) const {

	for (IndexType i = 0; i < numRows; ++i) {
		result[i] = 0.0;
		for (IndexType jj = ia[i]; jj < ia[i + 1]; ++jj) {
			//IndexType j = ja[jj];
			result[i] += values[jj] * v[ja[jj]];
		}
	}
}

inline void CSRmatrix::vectorMultiply(const std::valarray<double>& v, std::valarray<double>& result) const {
	for (IndexType i = 0; i < (*s).numRows; ++i) {
		result[i] = 0.0;
		for (IndexType jj = (*s).ia[i]; jj < (*s).ia[i + 1]; ++jj) {
			//IndexType j = (*s).ja[jj];
			result[i] += (*s).values[jj] * v[(*s).ja[jj]];
		}
	}
}


/// A standard preconditioning operation
/// consider a lower triangular system of the form Lx = b
/// w.l.o.g. assume that L is unit lower triangular, if not scale s or better write new member method
/// ones on diagonal are not stored
/// Output: rhs x and result are the same after calling this method.  x=rhs on input, solution on output.
inline void CSRmatrix::forwardElimination(std::valarray<double>& x) const {
	for (IndexType i = 1; i < (*s).numRows; ++i) {
		for (IndexType jj = (*s).ia[i]; jj < (*s).ia[i + 1]; ++jj) {
			// IndexType j = (*s).ja[jj];
			x[i] -= (*s).values[jj] * x[(*s).ja[jj]];
		}
	}
}

/// solve Ux=rhs where x and rhs are dense.  x=rhs on input, solution on output.
inline void CSRmatrix::backwardElimination(std::valarray<double>& x) const {
	for (int j = (*s).numRows - 1; j >= 0; --j) {
		x[j] /= (*s).values[(*s).ia[j + 1] - 1];
		for (IndexType p = (*s).ia[j]; p < (*s).ia[j + 1] - 1; ++p) {
			x[(*s).ja[p]] -= (*s).values[p] * x[j];
		}
	}

}



#endif
