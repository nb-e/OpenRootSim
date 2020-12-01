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
#ifndef VECTORMATH_HPP_
#define VECTORMATH_HPP_
#include <cstdlib>
#include <deque>
#include "../engine/DataDefinitions.hpp"


//typedefinition of a spline, repesented by a vector of coordinates
typedef std::deque<Coordinate > Spline;

//return the length of vector/line p1-p2
//overloaded this for other dimensions
double vectorlength(const Coordinate &p1,const Coordinate &p2);
double vectorlength(const Coordinate &p1);
double vectorlength(const double &p1);
double vectorlength(const double &p1,const double &p2);

//return true when p3 is on line p1-p2. d1 is distance p1-p3. d2 is distance p2-p3
bool onLine(const Coordinate &p1, const Coordinate &p2, const Coordinate &p3, double &d1, double &d2);

//distance between a point and a line
double distance(const Coordinate &l1, const Coordinate &l2, const Coordinate &p, double &rl);
double distance(const Coordinate &l, const Coordinate &p, double &rl);

//intersect of line and sphere
double intersectLineWithSphere(const Coordinate &l1, const Coordinate &l2, const Coordinate &center, const double &radius);

//return the length of a spline (a sorted vector of coordinates)
double splinelength(const Spline &spline);

//return true when a p3 is on a spline, d1 and d2 are distances from p3 to beginning and end of the spline respectively
bool onSpline(const Spline &spline, const Coordinate &p3,  double &d1, double &d2, int &nearestSplinePosition,  double &d1n, double &d2n);

//return position of a point x cm away from the base of a vector (no limitation on length of vector)
Coordinate positionAllong(const Coordinate &base, const Coordinate &direction, const double &length);

//Return the position of a point x cm down a spline from p3
Coordinate moveDownSpline(const Spline &spline, const Coordinate &p3, const double &x);

//normalize a vector (making it length 1)
void normalizeVector(const Coordinate &base, Coordinate &direction);
void normalizeVector(Coordinate &vector);

//random vector & integer
//versions used by Robert D Davis in his version 111 of SimRoot.
int randomIntDavis(const int &max);

//convert to radians
void convertToRadians(double &alpha, const Unit &unit);

//rotate vector around axis
//result vector is vector v rotated alpha degrees/radians (set unit) using vector axes as rotational axes
Coordinate rotateAroundVector(const Coordinate &v, const Coordinate &axes, const double &alpha, const Unit &unit);

//creates perpendicular to plane 0,0,0 ; p1 ; p2
//function based on original SimRoot crossprod()
Coordinate perpendicular(const Coordinate &p1, const Coordinate &p2);

//create perpendicular to vector v in the vertical plane
Coordinate perpendicularVectorInVerticalPlane(const Coordinate &v);

//angle between two vectors in rad
double angleInRad(const Coordinate &v1, const Coordinate &v2);

#endif /*VECTORMATH_HPP_*/
