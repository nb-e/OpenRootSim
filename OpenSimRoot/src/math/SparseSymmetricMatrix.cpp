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
 * SparseSymmetricMatrix.cpp
 *
 *  Created on: April 28, 2014
 *      Author: Christian
 */

#include "SparseSymmetricMatrix.hpp"

#include <iostream>

 // if matrix size is variable
 SparseSymmetricMatrix::SparseSymmetricMatrix() {
 	s = new std::vector< col_container >(2,col_container()); // empty vector of maps
 }
 SparseSymmetricMatrix::SparseSymmetricMatrix(std::size_t _rows) {
	std::size_t n(2);
	if(n<_rows) n=_rows;
 	s = new std::vector< col_container >(n,col_container());
 }
 SparseSymmetricMatrix::SparseSymmetricMatrix(const SparseSymmetricMatrix& t)// copy
 {
	s = new std::vector< col_container >(t.s->size());
 	for(auto it=t.s->begin(); it!=t.s->end(); ++it){
 		s->push_back(*it);
 	}
 }

 SparseSymmetricMatrix::~SparseSymmetricMatrix()
 {
 	delete s;
 }

 void SparseSymmetricMatrix::print_sparse(std::ostream& os) const{
	 row_iterator it;
	 col_iterator jt;
	 std::size_t count = 0;
	 for (it= s->begin(); it != s->end(); ++it){
		 os << count << ": ";
		 for (jt = it->begin(); jt != it->end(); ++jt){
			os << "(" << jt->first << ") = " << jt->second << "\t";
		 }
		 os << std::endl;
		 ++count;
	 }
 }


 void SparseSymmetricMatrix::ILU(SparseMatrix & L, SparseMatrix & U) const {

	std::size_t n = s->size();

	U.resize(n); // or copy R(A) ?
	L.resize(n);

	col_iterator jt;
	col_iterator col;

	for (std::size_t i = 0; i < n - 1; ++i) { // n-1 iterations
		col = ((*s)[i]).begin();
		double a_ii = col->second;
		L.insert(i,i,1.);
		U.insert(i,i,a_ii);
		if (std::fabs(a_ii) <= 1.e-30) {
			msg::error("Sparse ILU: Division by really small value");
		}
		for (col_iterator k_iter = ++col; col != ((*s)[i]).end(); ++col) { // loop over (the rest of the) rows but this is symmetric version so loop over entries in this map
			std::size_t k = k_iter->first;
			// computation of L
			// A(k,i) := A(k,i) / A(i,i) // check for division by zero
			double a_ki = k_iter->second / a_ii;
			// set new entry of ILU matrix
			L.insert(k, i, a_ki);

			for (jt = ++(((*s)[i]).begin()); jt != ((*s)[i]).end(); ++jt) { // loop over columns of the rest of the matrix
				// computation of U
				// A(k,j) := A(k,j) - A(k,i) * A(i,j)
				double a_ij = jt->second;
				std::size_t j = jt->first;
				double a_kj = (*s)[k].find(j)->second; // or (*s)[k][j] ?
				// meaning: a_kj = a_kj - a(k,i) * a_ij;
				a_kj -=  a_ki * a_ij; // sum
				U.addValueSafely(k, j, a_kj); // we need to add this U.insert(k, j, a_kj);
			}
		}
	}

	return;
}


 void SparseSymmetricMatrix::ICholesky(SparseSymmetricMatrix & L) const {

	std::size_t n = s->size();

	L.resize(n);

	col_iterator jt;
	col_iterator col;
	col_iterator k_iter;
	for (std::size_t i = 0; i < n - 1; ++i) { // n-1 iterations
		col = ((*s)[i]).begin();
		double a_ii = std::sqrt(col->second);
		L.insert(i, i, a_ii);

		if (std::fabs(a_ii) <= 1.e-30) {
			msg::error("Sparse ICholesky: Division by really small value");
		}
		for (k_iter = ++col; col != ((*s)[i]).end(); ++col) { // loop over (the rest of the) rows but this is symmetric version so loop over entries in this map
			std::size_t k = k_iter->first;
			// computation of L
			// A(k,i) := A(k,i) / A(i,i) // check for division by zero
			double a_ki = k_iter->second / a_ii;
			// set new entry of ILU matrix
			L.insert(k, i, a_ki);
		}
		col = ((*s)[i]).begin();
		for (jt = ++col; col != ((*s)[i]).end(); ++jt) {
			//for (jt = ++(((*s)[i]).begin()); jt != ((*s)[i]).end(); ++jt) { // loop over columns of the rest of the matrix
			std::size_t j = jt->first;
			for (std::size_t k = j; k != n; ++k) {
				// computation of U
				// A(k,j) := A(k,j) - A(k,i) * A(i,j)
				double a_ij = jt->second;
				std::size_t j = jt->first;
				col_container colums = (*s)[k];
				col_iterator ptr_col = colums.find(j);
				if (ptr_col != colums.end()) {
					double a_kj = ptr_col->second;
					// meaning: a_kj = a_kj - L(k,i) * a_ij;
					a_kj  -=  L(k,i) * a_ij; // sum
					L.addValueSafely(k, j, a_kj); // L.insert(k, j, a_kj);
				} else {
					continue;
				}
			}
		}
	}

}
