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
 * SparseMatrix.cpp
 *
 *  Created on: March 26, 2015
 *      Author: Christian
 */

#include "SparseMatrix.hpp"

#include <iostream>

 // if matrix size is variable
 SparseMatrix::SparseMatrix() {
 	s = new std::vector< col_container >(2,col_container()); // empty vector of maps
 }
 SparseMatrix::SparseMatrix(std::size_t _rows) {
	std::size_t n(2);
	if(n<_rows) n=_rows;
 	s = new std::vector< col_container >(n,col_container());
 }
 SparseMatrix::SparseMatrix(const SparseMatrix& t)// copy
 {
	s = new std::vector< col_container >(t.s->size());
 	for(auto it=t.s->begin(); it!=t.s->end(); ++it){
 		s->push_back(*it);
 	}
 }

 SparseMatrix::~SparseMatrix()
 {
 	delete s;
 }

 void SparseMatrix::print_sparse(std::ostream& os) const{
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

 void SparseMatrix::ILU(SparseMatrix & A1) const {

 	std::size_t n = s->size();

 	if(A1.size() != n) A1.resize(n); // or copy A1(A) ?

 	col_iterator jt;
 	col_iterator col;
double a_kj;
 	for (std::size_t i = 0; i < n - 1; ++i) { // n-1 iterations
 		double a_ki;
 		col = ((*s)[i]).find(i);
 		double a_ii = col->second;

 		if (std::fabs(a_ii) <= 1.e-30) {
 			msg::error("Sparse ILU: Division by really small value");
 		}

 		for (std::size_t k = ++i; k != n; ++k) { // loop over (the rest of the)
 			// computation of L
 			// A(k,i) := A(k,i) / A(i,i) // check for division by zero
 			col_container colums = (*s)[k];
 			col_iterator ptr_col = colums.find(i);
 			if(ptr_col != colums.end()){
 			a_ki = ptr_col->second / a_ii;
 			// set new entry of ILU matrix
 			A1.insert(k, i, a_ki);

 			col_iterator it=colums.begin();
 			for (jt = ++(((*s)[i]).begin()); jt != ((*s)[i]).end(); ++jt) { // loop over columns of the rest of the matrix
 				// computation of U
 				// A(k,j) := A(k,j) - A(k,i) * A(i,j)
 				double a_ij = jt->second;
 				std::size_t j = jt->first;
 				while(it->first<j && it!=colums.end()){ ++it;}
 				//ptr_col = colums.find(j);
 				//if(ptr_col != colums.end()){
 				if(it->first==j){
 				a_kj = ptr_col->second; // or (*s)[k][j] ?
 				a_kj -= a_ki * a_ij;
 				A1.insert(k, j, a_kj);
 				}else{
 					continue;
 				}
 			}
			}else{
 				continue;
 			}
 		}
 	}

 	return;
 }


 void SparseMatrix::ILU(SparseMatrix & L, SparseMatrix & U) const {

 	std::size_t n = s->size();

 	if(L.size() != n) L.resize(n);
 	if(U.size() != n) U.resize(n);

 	col_iterator jt;
 	col_iterator col;
double a_kj;
 	for (std::size_t i = 0; i < n - 1; ++i) { // n-1 iterations
 		double a_ki;
 		col = ((*s)[i]).find(i);
 		double a_ii = col->second;
		L.insert(i,i,1.);
		U.insert(i,i,a_ii);
 		if (std::fabs(a_ii) <= 1.e-30) {
 			msg::error("Sparse ILU: Division by really small value");
 		}

 		for (std::size_t k = ++i; k != n; ++k) { // loop over (the rest of the)
 			// computation of L
 			// A(k,i) := A(k,i) / A(i,i) // check for division by zero
 			col_container colums = (*s)[k];
 			col_iterator ptr_col = colums.find(i);
 			if(ptr_col != colums.end()){
 			a_ki = ptr_col->second / a_ii;
 			// set new entry of ILU matrix
 			L.insert(k, i, a_ki);

 			col_iterator it=colums.begin();
 			for (jt = ++(((*s)[i]).begin()); jt != ((*s)[i]).end(); ++jt) { // loop over columns of the rest of the matrix
 				// computation of U
 				// A(k,j) := A(k,j) - A(k,i) * A(i,j)
 				double a_ij = jt->second;
 				std::size_t j = jt->first;
 				while(it->first<j && it!=colums.end()){ ++it;}
 				//ptr_col = colums.find(j);
 				//if(ptr_col != colums.end()){
 				if(it->first==j){
 				a_kj = ptr_col->second; // or (*s)[k][j] ?
 				a_kj -= a_ki * a_ij;
 				U.insert(k, j, a_kj);
 				}else{
 					continue;
 				}
 			}
			}else{
 				continue;
 			}
 		}
 	}

 	return;
 }


