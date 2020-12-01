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

#include "VectorMath.hpp"
#include <math.h>
#include "MathLibrary.hpp"
#include "../cli/Messages.hpp"

//return the length of vector/line p1-p2
double vectorlength(const Coordinate &p1,const Coordinate &p2){
	//calculate lenght of the vector with base point p1 and direction p2
	double r(sqrt( square(p1.x-p2.x) + square(p1.y-p2.y) + square(p1.z-p2.z)));
	return r;
}
double vectorlength(const Coordinate &p1){
	//calculate lenght of the vector with base point p1 and direction p2
	double r(sqrt( square(p1.x) + square(p1.y) + square(p1.z)));
	return r;
}
double vectorlength(const double &p1){
	return (std::abs(p1));
}
double vectorlength(const double &p1,const double &p2){
	return (std::abs(p1-p2));
}
//intersect of line l1-l2 with sphere
/*
 *  a = (x2 - x1)^2 + (y2 - y1)^2 + (z2 - z1)^2
 *	b = 2[ (x2 - x1) (x1 - x3) + (y2 - y1) (y1 - y3) + (z2 - z1) (z1 - z3) ]
 *	c = x3^2 + y3^2 + z3^2 + x1^2 + y1^2 + z1^2 - 2[x3 x1 + y3 y1 + z3 z1] - r2
 *
 */
double intersectLineWithSphere(const Coordinate &l1, const Coordinate &l2, const Coordinate &center, const double &radius){
	//coefficients
	Coordinate l21(l2-l1);
	Coordinate l13(l1-center);
	double a = (l21*l21).sum();
	double b = 2*((l21*l13).sum());
	double c = center.squareSum()+l1.squareSum() - (2*(center*l1).sum()) - square(radius);
	//if c<0 than
	//two solutions
	double sol1,sol2;
	if(quadraticFormula(a,b,c,sol1,sol2)) msg::error("intersectLineWithSphere: line does not intersect");
	//one solution should be between l1 and l2
	if(sol1>0 && sol2>0 && sol1!=sol2) msg::error("intersectLineWithSphere: conceptual error - check code");
	if(sol1>0){
		return sol1;
	}else{
		return sol2;
	}
}

//return true when p3 is on line p1-p2. d1 is distance p1-p3. d2 is distance p2-p3
bool onLine(const Coordinate &p1, const Coordinate &p2, const Coordinate &p3, double &d1, double &d2){
	//set distances
	d1=vectorlength(p1,p3);
	d2=vectorlength(p2,p3);
	double d3=vectorlength(p1,p2);

	//if all in strait line with p3 in the middle then d1+d2 equals length of p1->p2
	//TODO ofcourse d1+d2 shoulde equal d3, but small numerical errors seem to cause problems here
	return ((d1+d2)-d3<0.00000001);
}

//distance between a point and a line
double distance(const Coordinate &l1, const Coordinate &l2, const Coordinate &p, double &rl){
	return distance(l2-l1 , p-l1, rl);
}
double distance(const Coordinate &l, Coordinate const &p, double &rl){
	//Check for zero length of l
	if(l.x==0 && l.y==0 && l.z==0 ) {
		rl=0;
		return p.length();
	}
	//relative distance of the projectionpoint.
	rl = (((l.x*p.x)+(l.y*p.y)+(l.z*p.z)) / (pow(l.x,2)+pow(l.y,2)+pow(l.z,2)));
	//projectionpoint
	const Coordinate projectionPoint= l*rl;
	//return distance projectionpoint-p
	return vectorlength(p,projectionPoint);
}

//return the length of a spline (a sorted vector of coordinates)
double splinelength(const Spline &spline){
	//return type
	double r=0;
	//add up all length of lines between coordinates
	Spline::const_iterator it, itnext;
	itnext= ++spline.begin();
	for(it=spline.begin(); itnext!=spline.end() ; ++it){
		r+=vectorlength(*it,*itnext);
		++itnext;
	}
	return r;
}

//return true when a p3 is on a spline, d1 and d2 are distances from p3 to beginning and end of the spline respectively, d1n d2n same for points before and after p3
bool onSpline(const Spline &spline, const Coordinate &p3,  double &d1, double &d2, int &nearestSplinePosition,  double &d1n, double &d2n){
	//return type
	bool r=false;

	//reset d1 and d2 to zero
	d1=0;
	d2=0;

	//if spline contains no data
	if(spline.empty()) msg::error("onSpline: Spline contains no data");

	//if there is just one coordinate in spline
	if(spline.size()==1){
		if(p3==spline[0]) {
			nearestSplinePosition=0;
			return true;
		}else{
			return false;
		}
	}

	//dummy variable
	double d1d, d2d;

	//probe all line fragments
	Spline::const_iterator it, itnext;
	itnext= ++spline.begin();
	int pos=0;
	for(it=spline.begin(); itnext!=spline.end() ; ++it){
		if(onLine(*it,*itnext,p3,d1d,d2d)){
			r=true;
			d1+=d1d;
			d2+=d2d;
			d1n=d1d;
			d2n=d2d;
			nearestSplinePosition=pos;
		}else if(r){
			d2+=vectorlength(*it,*itnext);
		}else{
			d1+=vectorlength(*it,*itnext);
		}
		++pos;
		++itnext;
	}
	return r;
}

//return position of a point x cm away from the base of a vector (no limitation on length of vector)
Coordinate positionAllong(const Coordinate &base, const Coordinate &direction, const double &length){
	//return type
	Coordinate r;

	//relative vector length
	if(base==direction) msg::error("positionAllong: base and direction have equal coordinates");
	double relativeLength=length/vectorlength(base,direction);

	//direction as relative to base
	r=direction-base;

	//position new point relative to base
	r=r*relativeLength;

	//return absolute position
	return (r+base);
}

//Return the position of a point x cm down a spline from p3
Coordinate moveDownSpline(const Spline &spline, const Coordinate &p3, const double &x){
	//check if p3 is on spline
	//TODO: do this more sensible, with a warning, a distance from spline and a correction
	double d1,d2,d3,d4;
	int pos(0);
	if(!onSpline(spline,p3,d1,d2,pos,d3,d4)) msg::error("moveDownSpline: starting coordinate does not lay on spline");

	//check if there is space
	///@todo we seem to run sometimes into small accuracy problems here
	if(d2+1E-8 < x) {
		msg::error("moveDownSpline: spline is not long enough to move "+std::to_string(x)+" cm down from point p3. Length p3-tip of the spline is "+std::to_string(d2)+" cm.");
	}

	//set pos to nearest following point listed in spline
	pos++;

	//add more segments till d>=x;
	unsigned int i;
	for(i=pos; i+1<spline.size() && d4<x ; i++){
		d4+=vectorlength(spline[i],spline[1+i]);
	}

	//return exact point finding it in reverse direction
	return positionAllong(spline[i],spline[i-1],d4-x);
}

//Normalize vector
void normalizeVector(const Coordinate &base, Coordinate &direction){
	double l(vectorlength(base,direction));
	if(l==0) {
		msg::error("normalizeVector: Can not normalize vector with zero length");
	}
	direction=direction/l;
}
void normalizeVector(Coordinate &vector){
	double l=vectorlength(vector);
	if(l==0) return;
	vector=vector/l;
}

int randomIntDavis(const int &max){
	//note 'double ceil(double)' is a function in math.h
	return (int)ceil(max*(double)rand()/((double) RAND_MAX));
}

//convert to radians
void convertToRadians(double &alpha, const Unit &unit){
	if(unit.name=="degrees"){
		alpha= PI*alpha/180;
	} else if (unit.name=="radians"){
		//alpha= alpha;
	}else{
		msg::error ("rotateVector: Alpha has an unknown unit: '"+unit.name+"'.");
	}
}

//rotate vector around axis
//result vector is vector v rotated alpha degrees/radians (set unit) using vector axes as rotational axes
//this function is derived from the original RotAboutArbVector function in the first version of SimRoot.
Coordinate rotateAroundVector(const Coordinate &vL, const Coordinate &axesL, const double &alpha, const Unit &unit){
	//return type
	Coordinate r;

	//normalize vectors
	Coordinate axes(axesL), v(vL);
	normalizeVector(axes);
	normalizeVector(v);

	//alpha rad
	double alphaRad=alpha;
	convertToRadians(alphaRad, unit);

	//calculate the relative changes in xyz direction from the corner over which we rotate (alphaRad)
	double cosA=cos(alphaRad);
	double sinA=sin(alphaRad);
	double invCA=1-cosA;

	//calculate the changes
	Coordinate tmp;
	tmp.x= 	((axes.x * axes.x * invCA) + cosA) ;
	tmp.y=	((axes.x * axes.y * invCA) - (axes.z * sinA)) ;
	tmp.z=	((axes.x * axes.z * invCA) + (axes.y * sinA)) ;

	r.x= (tmp.x*v.x) + (tmp.y*v.y) + (tmp.z*v.z)  ;
	tmp.x= 		((axes.y * axes.x * invCA) + (axes.z * sinA));
	tmp.y=		((axes.y * axes.y * invCA) + cosA);
	tmp.z=		((axes.y * axes.z * invCA) - (axes.x * sinA));
	r.y= (tmp.x*v.x) + (tmp.y*v.y) + (tmp.z*v.z)  ;
	tmp.x= 		((axes.z * axes.x * invCA) - (axes.y * sinA)) ;
	tmp.y=		((axes.z * axes.y * invCA) + (axes.x * sinA)) ;
	tmp.z=		((axes.z * axes.z * invCA) + cosA) ;
	r.z= (tmp.x*v.x) + (tmp.y*v.y) + (tmp.z*v.z)  ;

	//normalize the return vector
	//TODO (I don't think normalizing has any meaning, but the original version does it (jouke))
	normalizeVector(r);

	//return
	return r;
}

//creates perpendicular to plane 0,0,0 ; p1 ; p2
//function based on original SimRoot crossprod()
Coordinate perpendicular(const Coordinate &p1, const Coordinate &p2){
	//return type
	Coordinate r;
	//crossproduct
	r.x = (p1.y*p2.z) - (p2.y*p1.z);
	r.y = (p1.z*p2.x) - (p2.z*p1.x);
	r.z = (p1.x*p2.y) - (p2.x*p1.y);

	//if the result is 0,0,0 either one of the coordinates is the origin and or they are in the same direction
	if(r.x==0 && r.y==0 && r.z==0){
		r.x=std::rand();
		r.z=std::rand();
		if (p1.x==0 && p1.y==0 && p1.z==0){
			return perpendicular(r,p2);
		}else{
			return perpendicular(r,p1);
		}
		//p1 and p2 are not forming a plane, but are in the same direction. take arbitrary point and try again
	}

	//return
	return r;
}


//create perpendicular to vector v in the vertical plane
/*rules:
 * verticale plane so: Rx/Rz=Vx/Vz
 * perpendicular so: Ry=sqrt(Vx^2 + Vz^2)
 * same length so: Rx^2 + Ry^2 + Rz^2 = Vx^2 + Vy^2 + Vz^2
 *
 * simplification:
 * Ry^2 = Vx^2 + Vz^2
 * Rx^2 + Rz^2 = Vy^2
 * Rx = Rz * Vx/Vz
 * Rz^2  = Vy^2/(1+Vx^2/Vz^2)    (since Rz^2 * Vx^2/Vz^2 + Rz^2 = Vy^2 from which follows Rz^2 * (1+Vx^2/Vz^2) = Vy^2    )
 */
/* this function needs to be reviewed, it causes problems when z=0
Coordinate perpendicularInVerticalPlane(const Coordinate &v){
	//check v is not of size 0
	if(v.x==0 && v.y==0 && v.z==0) msg::error("perpendicularVectorInVerticalPlane: vector has length zero");

	//return type
	Coordinate r;

	//Do calculations, see above
	r.y=sqrt(quadratic(v.x) + quadratic(v.z));
	r.z=sqrt(quadratic(v.y) / (1+ quadratic(v.x/v.z)));
	r.x= r.z * v.x/v.z;

	//return
	return r;
}
*/
///@todo: calculate angle between v1 and horizontal plane, idem v2 substract?
double angleInRad(const Coordinate &v1, const Coordinate &v2){
	msg::error("angleRad: not implemented yet");
	return 0;
}
