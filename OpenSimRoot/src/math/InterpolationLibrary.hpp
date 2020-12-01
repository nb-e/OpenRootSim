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
#ifndef INTERPOLATIONLIBRARY_HPP_
#define INTERPOLATIONLIBRARY_HPP_

#include "../cli/Messages.hpp"
#include "../engine/DataDefinitions.hpp"
#include <map>
#include "../tools/StringExtensions.hpp"

// All the interpolation methods supported by SimulaTable
enum class TableInterpolation {
	step,
	linear,
};
// string representations of interpolation methods (eg for XML tags)
static std::map<TableInterpolation, std::string> TableInterpolationNames = {
	{TableInterpolation::step, "step"},
	{TableInterpolation::linear, "linear"}
};

// All the interpolation methods supported by SimulaGrid
enum class GridInterpolation {
	triLinear,
	naturalNeighbour,
	nearestNeighbour,
	kriging3D,
	inverseDistanceWeightedAverage,
	diffuseSpots
};
// string representations of interpolation methods (eg for XML tags)
static std::map<GridInterpolation, std::string> GridInterpolationNames = {
	{GridInterpolation::triLinear, "triLinearInterpolation"},
	{GridInterpolation::naturalNeighbour, "naturalNeighbour"},
	{GridInterpolation::nearestNeighbour, "nearestNeighbourInterpolation"},
	{GridInterpolation::kriging3D, "Kriging3D"},
	{GridInterpolation::inverseDistanceWeightedAverage, "inverseDistanceWeightedAverage"},
	{GridInterpolation::diffuseSpots, "diffuseSpots"}
};

//declaration of linearInterpolation functions for State, StateRate and Coordinate
void linearInterpolation(const double x1,const double x2,const double x3,const double y1,const double y2, double &y3);
void linearInterpolation(const Time &x1, const Time &x2, const Time &x3, const StateRate &y1, const StateRate &y2, StateRate &y3);
//void linearInterpolation(const Time &x1, const Time &x2, const Time &x3, const Time &y1, const Time &y2, Time &y3);
void linearInterpolation(const Time &t1, const Time &t2, const Time &t3, const MovingCoordinate &p1, const MovingCoordinate &p2, MovingCoordinate &p3);
void linearInterpolation(const Time &t1, const Time &t2, const Time &t3, const Coordinate &p1, const Coordinate &p2, Coordinate &p3, const bool & doRates=false);
void linearInterpolation( const Coordinate &p1, const Coordinate &p2, const Coordinate &p3, const Time &t1, const Time &t2,  Time &t3);

//Interpolation of a table
//TODO for now assume linear interpolation. for euler & Heuns that's OK
template <class Colum1, class Colum2, class Table>
Colum2 interpolate(const Colum1 & x, const Table &table){
	//checks for length of table
	if(table.empty())msg::error("interpolate: there is no data in the table.");
	//find upper bound: it_u->first>x
	typename Table::const_iterator it_u(table.lower_bound(x));// it_u->first>=x
	//check that we are not extrapolating at end of table
	if(it_u==table.end()){
		--it_u;
		if((x-it_u->first)<1.1*TIMEERROR){//allow small imprecision in time
			return it_u->second;
		}else{
			msg::error("interpolate: extrapolation requested beyond table");
		}
	}
	//check for exact match
	if((it_u->first-x)<1.1*TIMEERROR) return it_u->second;
	//check that we are not extrapolating before beginning of table
	if(it_u==table.begin()){
		msg::error("interpolate: extrapolation requested before table. requested "+convertToString<Colum1>(x)+" start of table is "+convertToString<Colum1>(table.begin()->first));
	}
	//point below upper bound
	typename Table::const_iterator it_l(it_u);
	--it_l; //it_l->first<x
	//interpolate
	Colum2 r;
	linearInterpolation(it_l->first,it_u->first,x,it_l->second,it_u->second,r);
	return r;
}




#endif /*INTERPOLATIONLIBRARY_HPP_*/
