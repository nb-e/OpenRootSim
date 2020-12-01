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
#include "InterpolationLibrary.hpp"
#include "MathLibrary.hpp"
#include "VectorMath.hpp"

//interpolation function overloaded for State, staterate and coordinates
//TODO these are only fit for euler. Others will have to be developed when other integration functions are used
void linearInterpolation(const double x1,const  double x2,const  double x3,const double y1,const double y2, double &y3){

	//Check for range, do interpolation
	if  (x2==x1) {
		msg::warning("linearInterpolation: No range for interpolation x1==x2, returning the average of y1 and y2 ");
		y3=(y1+y2)/2;
	} else {
		//this may do extrapolation
		y3=y1+((y2-y1)*(x3-x1)/(x2-x1));
	}

}
void linearInterpolation(const Time &x1, const Time &x2, const Time &x3, const StateRate &y1, const StateRate &y2, StateRate &y3){
	//Euler assumes constant rates
	//y3.rate=y1.rate;
	//now we are using heuns too intepolation is more correct. Currently we have no way to choose another intepolation method with
	//another integration method
	//Check for range, do interpolation
	if  (x2==x1) {
		msg::warning("linearInterpolation: No range for interpolation x1==x2, returning the average of y1 and y2 ");
		y3=(y1+y2)/2;
	} else {
		//this may do extrapolation
		y3=y1+((y2-y1)*(x3-x1)/(x2-x1));
	}
}
/*void linearInterpolation(const Time &x1, const Time &x2, const Time &x3, const Time &y1, const Time &y2, Time &y3){
	//Euler assumes constant rates
	//y3.rate=y1.rate;
	//now we are using heuns too intepolation is more correct. Currently we have no way to choose another intepolation method with
	//another integration method
	//Check for range, do interpolation
	if  (x2==x1) {
		msg::warning("linearInterpolation: No range for interpolation x1==x2, returning the average of y1 and y2 ");
		y3=(y1+y2)/2;
	} else {
		//this may do extrapolation
		y3=y1+((y2-y1)*(x3-x1)/(x2-x1));
	}
}*/
void linearInterpolation(const Time &t1, const Time &t2, const Time &t3, const MovingCoordinate &p1, const MovingCoordinate &p2, MovingCoordinate &p3){
	//euler assumes constant rate.
	StateRate xp1,yp1,zp1,xp2,yp2,zp2,xp3,yp3,zp3;
	p1.split(xp1,yp1,zp1);
	p2.split(xp2,yp2,zp2);
	p3.split(xp3,yp3,zp3);
	//euler assumes linearity during timestep delta t: interpolate linear.
	linearInterpolation(t1,t2,t3,xp1,xp2,xp3);
	linearInterpolation(t1,t2,t3,yp1,yp2,yp3);
	linearInterpolation(t1,t2,t3,zp1,zp2,zp3);
	//join
	p3.join(xp3,yp3,zp3);
	//speed correction for rates
	double l3;
	linearInterpolation(t1,t2,t3,p1.rate.length(),p2.rate.length(),l3);
	p3.rate.setLength(l3);
}
void linearInterpolation(const Time &t1, const Time &t2, const Time &t3, const Coordinate &p1, const Coordinate &p2, Coordinate &p3, const bool & doRates){
	//euler assumes constant rate.
	double xp1,yp1,zp1,xp2,yp2,zp2,xp3,yp3,zp3;
	p1.split(xp1,yp1,zp1);
	p2.split(xp2,yp2,zp2);
	//p3.split(xp3,yp3,zp3);
	//euler assumes linearity during timestep delta t: interpolate linear.
	linearInterpolation(t1,t2,t3,xp1,xp2,xp3);
	linearInterpolation(t1,t2,t3,yp1,yp2,yp3);
	linearInterpolation(t1,t2,t3,zp1,zp2,zp3);
	//join
	p3.join(xp3,yp3,zp3);
	//speed correction for rates
	if(doRates){
		//speed correction for rates
		double l3;
		linearInterpolation(t1,t2,t3,p1.length(),p2.length(),l3);
		p3.setLength(l3);
	}
}
void linearInterpolation( const Coordinate &p1, const Coordinate &p2, const Coordinate &p3, const Time &t1, const Time &t2,  Time &t3){
	//euler assumes constant rate.
	double l3(vectorlength(p1,p3));
	double l2(l3+vectorlength(p2,p3));
	linearInterpolation(0,l2,l3,t1,t2,t3);
}
