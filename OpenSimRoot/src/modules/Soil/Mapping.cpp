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

#include "Mapping.hpp"

#include "../../engine/Origin.hpp"
#include <math.h>


void Mapping::interpolate_factor2(const tVector & source, tVector & target,const tIndex nxw, const tIndex nyw, const tIndex NX, const tIndex NY, const tIndex  numnp_2x) {
	tIndex i = 0;
		tIndex counter = 0;
		tIndex row = 1;
		tIndex layer = 1;
		while( counter < numnp_2x /*&& i < sourceNumNP*/){
			if ( i < row*nxw-1 ) {
				target[counter ] 	= source[i];
				target[counter + 1] = 0.5 * (source[i] + source[i + 1]);
				counter += 2;
			} else {
				target[counter]     = source[i];
				counter += NX+1;
				++row;
				if (i == layer*nxw*nyw-1 ) {
					counter += (NX*(NY-1));
					++layer;
				}
			}
			++i;
		}

		i = 0;
		counter = NX;
		row = 1;
		tIndex counter2 = 0;
		while ( counter < numnp_2x ) {
			if (i < row*nxw-1) {
				target[counter]     = 0.5 * (source[i] + source[i + nxw]);
				target[counter + 1] = 0.25* (source[i] + source[i + 1] + source[i + nxw] + source[i + nxw + 1]);
				counter += 2;
			} else {
				target[counter]     = 0.5 * (source[i] + source[i + nxw]);
				counter += NX+1;
				++row;
				++counter2;
				if( counter2 == nyw -1){
					counter += (NX*NY + NX) ;
					i += nxw; ++row; // then i is at the end of the layer
					counter2 = 0;
				}
			}
			++i;
		}

		i = 0;
		counter = NX*NY;
		row = 1;
		layer = 1;
		while ( counter < numnp_2x ) {
			if (i < row*nxw-1 ) {
				target[counter]      = 0.5  * (source[i] + source[i + nxw * nyw]);
				target[counter + 1]  = 0.25 * (source[i] + source[i + 1] + source[i + nxw * nyw]+ source[i + 1 + nxw * nyw]);
				counter +=2;
			} else {
				target[counter]    = 0.5 * (source[i] + source[i + nxw * nyw]);
				counter +=  NX+1;
				++row;
				if (i == layer*nxw*nyw-1 ) {
					counter += (NX*(NY-1));
					++layer;
				}
			}
			++i;
		}


		i = 0;
		counter = NX*NY+NX;;
		row = 1;
		counter2 = 0;
		while ( counter < numnp_2x ) {
			if (i < row*nxw-1 ) {
				target[counter]    = 0.25 * (source[i + nxw] + source[i] + source[i + nyw * nxw] + source[i + nyw * nxw + nxw]);
				target[counter +1] = 0.125* (source[i + nxw] + source[i]  + source[i + nyw * nxw] + source[i + nyw * nxw + nxw]
							   + source[i + nxw + 1] + source[i + 1]  + source[i + nyw * nxw + 1] + source[i + nyw * nxw + nxw + 1]);
				counter +=2;
			} else {
				target[counter]    = 0.25* (source[i + nxw] + source[i] + source[i + nyw * nxw] + source[i + nyw * nxw + nxw]);
				counter += NX+1;
				++row;
				++counter2;
				if( counter2 == nyw -1){
					counter += (NX*NY + NX) ;
					i += nxw; ++row; // then i is at the end of the layer
					counter2 = 0;
				}
			}
			++i;
		}
	return;
}


void Mapping::interpolate(const Mesh_base & mesh_source, const tVector & source, tVector & target, const int factor) {

	const tIndex nxw(mesh_source.getnx());
	const tIndex nyw(mesh_source.getny());
	const tIndex nzw(mesh_source.getnz());

	const tIndex NumNP(target.size());
	const tIndex NX(2*nxw - 1);
	const tIndex NY(2*nyw - 1);
	const tIndex NZ(2*nzw - 1);
	const tIndex numnp_2x(NX*NY*NZ);
	const tIndex numnp_4x((2*NX-1)*(2*NY-1)*(2*NZ-1));
	const tIndex numnp_8x((2*(2*NX-1)-1)*(2*(2*NY-1)-1)*(2*(2*NZ-1)-1));

	if(factor == 2 || NumNP == numnp_2x){
		interpolate_factor2(source,target, nxw, nyw, NX, NY, numnp_2x);
	}
	else if(factor == 4 || NumNP == numnp_4x){
		tVector temptarget(0., numnp_2x);
		interpolate_factor2(source,temptarget, nxw, nyw, NX, NY, numnp_2x);
		interpolate_factor2(temptarget,target, NX, NY, 2*NX-1, 2*NY-1, numnp_4x);
	}else if(factor == 8 ) {
		tVector temptarget(0., numnp_2x);
		tVector temptarget2(0., numnp_4x);
		interpolate_factor2(source,temptarget, nxw, nyw, NX, NY, numnp_2x);
		interpolate_factor2(temptarget,target, NX, NY, 2*NX-1, 2*NY-1, numnp_4x);
		interpolate_factor2(temptarget,target, 2*NX-1, 2*NY-1, 2*(2*NX-1)-1, 2*(2*NY-1)-1, numnp_8x);
	}else{
		msg::error("interpolate: Fine Mesh is not valid or not a two potency multiple of coarse mesh");
	}

	return;

}
