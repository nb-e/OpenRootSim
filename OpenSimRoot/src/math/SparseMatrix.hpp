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
 * SparseMatrix.hpp
 *
 *  Created on: March 26, 2015
 *      Author: Christian
 */


#ifndef SPARSEMATRIX_H
#define SPARSEMATRIX_H


#include <valarray>
#include <vector>
#include <map>
#include <iostream>
#include "../cli/Messages.hpp"

// container for columns
typedef std::map<std::size_t, double> col_container;
// iterators
typedef col_container::iterator col_iterator;
typedef col_container::const_iterator const_col_iterator;
typedef std::vector< col_container >::iterator row_iterator;
typedef std::vector< col_container >::const_iterator const_row_iterator;


/// GENERAL SPARSE MATRIX, NONSYMMETRIC
class SparseMatrix {

public:
	SparseMatrix();
	SparseMatrix(std::size_t _rows);
	SparseMatrix(const SparseMatrix& t); //copy
	~SparseMatrix();

	double operator() (std::size_t r, std::size_t c) const;
	const col_container& operator[] (std::size_t i) const;

	void diagonal(std::valarray<double>& diag) const;
	void diagonal_inverse(std::valarray<double>& diag_inv) const;

	std::size_t size() const;
	void resize(std::size_t);
	void resetToZero();
	void insert(std::size_t r, std::size_t c, double value);
	void addValueSafely(std::size_t r, std::size_t c, double value);
	void addValueUnsafely(std::size_t r, std::size_t c, double value);
	void vectorMultiply(const std::valarray<double>& u,std::valarray<double>& product) const;
	void sumOfRows(std::valarray<double>& sum) const;
	void solveLowerTriangle_diag_1(const std::valarray<double>& rhs, std::valarray<double>& x) const;
	void solveLowerTriangle(const std::valarray<double>& rhs, std::valarray<double>& solution) const;
	void solveUpperTriangle(const std::valarray<double>& rhs, std::valarray<double>& solution) const;
	void multLowerTriangle(const std::valarray<double>& u, std::valarray<double>& solution) const;

	void print_sparse(std::ostream& os=std::cout) const;
	void ILU(SparseMatrix & A1) const;
	void ILU(SparseMatrix & L, SparseMatrix & U) const;
	void L1_inv_times_rhs(const std::valarray<double>& rhs, std::valarray<double>& x) const;
private:
	std::vector< col_container > *s;
};


inline void SparseMatrix::resetToZero(){
	 row_iterator it;
	 col_iterator jt;
	 for (it= s->begin(); it != s->end(); ++it){
		 for (jt = it->begin(); jt != it->end(); ++jt){
			jt->second = 0.;
		 }
	 }
}


inline std::size_t SparseMatrix::size() const{
	 std::size_t n = s->size();
	 return n;
}

inline void SparseMatrix::resize(std::size_t n){
	 s->resize(n,col_container());
}

/***************
 * write
 ****************************/
inline void SparseMatrix::insert(std::size_t r, std::size_t c, double value){
		if(r+1>s->size())
			s->resize(r+1,col_container()); //vector[] does not check boundaries
		((*s)[r])[c] = value;
}

inline void SparseMatrix::addValueSafely(std::size_t r, std::size_t c, double value){
		if(c+1>s->size()) s->resize(c+1,col_container()); //vector[] does not check boundaries
		col_container &m=((*s)[r]);
		auto it=m.find(c);
		if(it!=m.end()){
			it->second += value;
		}else{
			((*s)[r])[c] = value;
		}
}

//be careful, if the value does not exist in the matrix, this will result in unpredictable values as it will not initialize the value to 0
inline void SparseMatrix::addValueUnsafely(std::size_t r, std::size_t c, double value){
		((*s)[r])[c] += value;
}


/***************
 * operator() // read only
 ****************************/
//NOTE THAT THIS ASSUMES THE MATRIX HAS THE RIGHT SIZE
inline double SparseMatrix::operator()(std::size_t r, std::size_t c) const {
	col_container m(s->at(r));
	col_iterator it(m.find(c));
	if (it != m.end()) {
		return it->second;
	} else {
		return 0.0;
	}
}

/***************
 * operator[]
 ****************************/
inline const col_container& SparseMatrix::operator[](std::size_t i) const {
	return s->at(i); // get a map
}


/// TODO: not that efficient, would be better if diagonal is stored in extra array or at the front position
inline void SparseMatrix::diagonal_inverse(std::valarray<double>& diag_inv) const{
	row_iterator it = s->begin();
	col_iterator colit = it->begin();
	diag_inv[0] = 1./ colit->second;
	++it;
	std::size_t count = 1;
	for ( ; it != s->end(); ++it) {
		colit = it->find(count);
		diag_inv[count] = 1./ colit->second; //((*s)[count])[count];
		++count;
	}
}
inline void SparseMatrix::diagonal(std::valarray<double>& diag) const{
	col_iterator diagit;
	std::size_t count = 0;
	for (row_iterator it = s->begin(); it != s->end(); ++it) {
		diagit = it->find(count);
		diag[count] = diagit->second;
		++count;
	}
}

inline void SparseMatrix::vectorMultiply(const std::valarray<double>& u,std::valarray<double>& product) const{
	product = 0.; // initialize complete valarray
	std::size_t matrix_dim = s->size();
	std::size_t vector_dim = u.size();
	if(matrix_dim != vector_dim){
		msg::error("SparseMatrix::vectorMultiply: size miss-match. Matrix size: "+std::to_string(matrix_dim)+". Vector size: "+std::to_string(vector_dim));
	}
	col_iterator colit;
	int count=0;
	for(row_iterator it = s->begin(); it != s->end(); ++it){
		for(colit = it->begin(); colit != it->end(); ++colit){
			product[count] += u[colit->first] * colit->second;
		}
		++count;
	}
}

inline void SparseMatrix::multLowerTriangle(const std::valarray<double>& u, std::valarray<double>& solution) const{
		solution = 0.; // initialize complete valarray
		std::size_t matrix_dim = s->size();
		std::size_t vector_dim = u.size();
		if(matrix_dim != vector_dim){
			msg::error("SparseMatrix::vectorMultiply: size miss-match. Matrix size: "+std::to_string(matrix_dim)+". Vector size: "+std::to_string(vector_dim));
		}
		col_iterator colit;
		int count=0;
		for(row_iterator it = s->begin(); it != s->end(); ++it){
			for( colit= it->begin() ; colit == it->find(count); ){
				solution[count] += u[colit->first] * colit->second;
				++colit;
			}
			++count;
		}
}


inline void SparseMatrix::solveLowerTriangle_diag_1(const std::valarray<double>& rhs, std::valarray<double>& x) const{
	x = rhs;
	col_iterator colit, cend;
	row_iterator it = ++(s->begin());
	std::size_t count = 1;

	for ( ; it != s->end(); ++it) {
		cend = it->find(count);
		for( colit= it->begin() ; colit != cend; ++colit){
			x[count] -=  colit->second * x[colit->first];
		}
		++count;
	}
}

// applicalble at a Lower triangular matrix with ones on the diagonal
inline void SparseMatrix::L1_inv_times_rhs(const std::valarray<double>& rhs, std::valarray<double>& x) const{
	x = rhs;
	col_iterator colit, cend;
	row_iterator it = ++(s->begin());
	std::size_t count = 1;
	for ( ; it != s->end(); ++it) {
		for( colit= it->begin() ; colit != --(it->end()); ++colit){ // not to the end = diagonal, only until one value before
			x[count] -=  colit->second * x[colit->first];
		}
		++count;
	}
}


inline void SparseMatrix::solveLowerTriangle(const std::valarray<double>& rhs, std::valarray<double>& x) const{
	x = rhs;
	col_iterator colit;
	row_iterator it = s->begin();

	// row 0
	x[0] = x[0] / (it->begin()->second);
	++it;
	std::size_t row = 1;

	for ( ; it != s->end(); ++it) {
		colit= it->begin();
		while(colit != it->find(row)){
			x[row] -=  colit->second * x[colit->first];
			++colit;
		}
		x[row] /= colit->second;
		++row;
	}

}

inline void SparseMatrix::solveUpperTriangle(const std::valarray<double>& rhs, std::valarray<double>& x) const{
	x = 0.; // initialize complete valarray
	std::size_t n = s->size();
	double last_diag = s->back().begin()->second;
	x[n - 1] = rhs[n - 1] / last_diag;
	col_iterator colit;
	// backwards gauss steps
	for (int i = (int)n - 2; i >= 0; --i) {
		double sum = 0.;
		colit = (*s)[i].begin();
		++colit;
		for (; colit != (*s)[i].end(); ++colit) {
				sum +=  colit->second * x[colit->first];
		}
		x[i] = (rhs[i] - sum) / ((*s)[i].begin()->second); // this is s(i,i)
	}
}


inline void SparseMatrix::sumOfRows(std::valarray<double>& sum) const{
	sum.resize(s->size());
	sum = 0.; // initialize complete valarray
	col_iterator colit;
	int count=0;
	for(row_iterator it = s->begin(); it != s->end(); ++it){
		for(colit = it->begin(); colit != it->end(); ++colit){
			sum[count] += colit->second;
			sum[colit->first] += colit->second;
		}
		++count;
	}
}



#endif
