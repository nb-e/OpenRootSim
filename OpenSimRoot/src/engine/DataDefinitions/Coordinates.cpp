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

#include "Coordinates.hpp"
#include "../../cli/Messages.hpp"
#include <vector>
#include <math.h>
#define square(X) pow(X,2)

//==================coordinates==================
//constructors
//Coordinate::Coordinate():x(0),y(0),z(0),estimate(false){}
/*Coordinate::Coordinate(const Coordinate& p):
	x(p.x),y(p.y),z(p.z),estimate(p.estimate)
{}*/
/*Coordinate::Coordinate(const MovingCoordinate& p):
		x(p.state.x),y(p.state.y),z(p.state.z),estimate(p.state.estimate)
{}*/
Coordinate::Coordinate(double newX, double newY, double newZ):
		x(newX),y(newY),z(newZ)
{}

//overloading aritmatic operators
Coordinate & Coordinate::operator= (const Coordinate &p) {
		x=p.x;
		y=p.y;
		z=p.z;
		return *this;
}
void Coordinate::operator+= (const Coordinate &p)  {
		x += p.x;
		y += p.y;
		z += p.z;
}
void Coordinate::operator-= (const Coordinate &p)  {
		x -= p.x;
		y -= p.y;
		z -= p.z;
}
void Coordinate::operator*= (double d) {
		x*=d;
		y*=d;
		z*=d;
}
void Coordinate::operator/= (double d) {
		x/=d;
		y/=d;
		z/=d;
}
Coordinate Coordinate::operator+ (const Coordinate &p) const {
		Coordinate r(*this);
		r.x += p.x;
		r.y += p.y;
		r.z += p.z;
		return r;
}
Coordinate Coordinate::operator- (const Coordinate &p) const {
		//name of right side is maintained
		Coordinate r(*this);
		r.x -= p.x;
		r.y -= p.y;
		r.z -= p.z;
		return r;
	}
Coordinate Coordinate::operator+ (double p) const {
		Coordinate r(*this);
		r.x += p;
		r.y += p;
		r.z += p;
		return r;
}
Coordinate Coordinate::operator- (double p) const {
		//name of right side is maintained
		Coordinate r(*this);
		r.x -= p;
		r.y -= p;
		r.z -= p;
		return r;
	}
Coordinate Coordinate::operator* (double d)const  {
		Coordinate r(*this);
		r.x *= d  ;
		r.y *= d ;
		r.z *= d ;
		return r;
	}
Coordinate Coordinate::operator* (const Coordinate &p)const{
	Coordinate r(*this);
	r.x*=p.x;
	r.y*=p.y;
	r.z*=p.z;
	return r;
}
Coordinate Coordinate::operator/ (const Coordinate &p)const{
	Coordinate r(*this);
	r.x/=p.x;
	r.y/=p.y;
	r.z/=p.z;
	return r;
}

double Coordinate::operator^ (const Coordinate &p)const{
	return (square(x-p.x) + square(y-p.y) + square(z-p.z));
}


Coordinate Coordinate::operator/ (double d) const {
		Coordinate r(*this);
		if(d==0) {
			msg::error("Coordinate::operator/ : can't divide through zero");
		}else{
			r.x /=d;
			r.y /=d;
			r.z /=d;
		};
		return r;
	}

bool  Coordinate::operator== (const Coordinate &d)const {
	if(x!=d.x) return false;
	if(y!=d.y) return false;
	if(z!=d.z) return false;
	return true;
}
bool  Coordinate::operator!= (const Coordinate &d)const {
	if(x!=d.x) return true;
	if(y!=d.y) return true;
	if(z!=d.z) return true;
	return false;
}

bool  Coordinate::operator> (const Coordinate &d)const{
	if(y>d.y){
		return true;
	}else if(y<d.y){
		return false;
	}else if(x>d.x){
		return true;
	}else if(x<d.x){
		return false;
	}else if(z>d.z){
		return true;
	}else{
		return false;
	}
}
bool  Coordinate::operator< (const Coordinate &d)const{
	if(y<d.y){
		return true;
	}else if(y>d.y){
		return false;
	}else if(x<d.x){
		return true;
	}else if(x>d.x){
		return false;
	}else if(z<d.z){
		return true;
	}else{
		return false;
	}
}
bool  Coordinate::operator>= (const Coordinate &d)const{
	if(*this<d){
		return false;
	}else{
		return true;
	}
}
bool  Coordinate::operator<= (const Coordinate &d)const{
	if(*this>d){
		return false;
	}else{
		return true;
	}
}


void Coordinate::split(double &xs, double &ys, double &zs)const{
	xs=x;
	ys=y;
	zs=z;
}
void Coordinate::join(double xs, double ys, double zs){
	x=xs;
	y=ys;
	z=zs;
}
void Coordinate::normalize(){
	double l=this->length();
	if(l!=0) *this/=l;
}
double Coordinate::length()const{
	double r(sqrt( square(x) + square(y) + square(z)));
	return r;
}

////todo
void Coordinate::setLength(double l){
	this->normalize();
	*this*=l;
}
double Coordinate::sum()const{
	double r(x + y + z);
	return r;
}
double Coordinate::squareSum()const{
	double r(square(x) + square(y) + square(z));
	return r;
}


std::ostream &operator<<(std::ostream &os, const Coordinate &obj){
	os	<<obj.x<<'\t'<<obj.y<<'\t'<<obj.z;
	return os;
}



MovingCoordinate::MovingCoordinate():
	state(0,0,0),rate(0,0,0){}
/*MovingCoordinate::MovingCoordinate(const MovingCoordinate& p):
	state(p.state),rate(p.rate){}*/
MovingCoordinate::MovingCoordinate(const Coordinate& p, const Coordinate& r):
	state(p),rate(r){}
MovingCoordinate::MovingCoordinate(const double &newX, const double &newY, const double &newZ):
	state(newX,newY,newZ),rate(0,0,0){}


bool  MovingCoordinate::operator== (const MovingCoordinate &d)const{
	if(state!=d.state) return false;
	if(rate!=d.rate) return false;
	return true;
}
bool  MovingCoordinate::operator!= (const MovingCoordinate &d)const{
	if(state==d.state) return false;
	if(rate==d.rate) return false;
	return true;
}

	//overloading aritmatic operators
/*MovingCoordinate & MovingCoordinate::operator= (const MovingCoordinate &p) {
	state=p.state;
	rate=p.rate;
//	estimate=p.estimate;
//	rate.estimate=p.rate.estimate;
	return *this;
}*/
MovingCoordinate MovingCoordinate::operator+ (const MovingCoordinate &p) const {
	MovingCoordinate r(*this);
	r.state+=p.state;
	r.rate+=p.rate;
//	r.estimate+=p.estimate;
//	rate.estimate+=p.rate.estimate;
	return r;
}
MovingCoordinate MovingCoordinate::operator- (const MovingCoordinate &p) const {
	MovingCoordinate r(*this);
	r.state-=p.state;
	r.rate-=p.rate;
//	r.estimate+=p.estimate;
//	rate.estimate+=p.rate.estimate;
	return r;
}
void MovingCoordinate::operator+= (const MovingCoordinate &p){
	state+=p.state;
	rate+=p.rate;
//	estimate+=p.estimate;
//	rate.estimate+=p.rate.estimate;
}
void MovingCoordinate::operator-= (const MovingCoordinate &p){
	state-=p.state;
	rate-=p.rate;
//	estimate+=p.estimate;
//	rate.estimate+=p.rate.estimate;
}
////todo
MovingCoordinate MovingCoordinate::operator* (const double &d)const{
	MovingCoordinate r(*this);
	r.state*=d;
	r.rate*=d;
	return r;
}
////todo
MovingCoordinate MovingCoordinate::operator/ (const double &d) const{
	MovingCoordinate r(*this);
	r.state/=d;
	r.rate/=d;
	return r;
}

void MovingCoordinate::split(StateRate & xs, StateRate & ys, StateRate & zs)const{
	xs.state=state.x;
	xs.rate=rate.x;
	ys.state=state.y;
	ys.rate=rate.y;
	zs.state=state.z;
	zs.rate=rate.z;
}
void MovingCoordinate::join(const StateRate & xs, const StateRate & ys, const StateRate & zs){
	state.x=xs.state;
	state.y=ys.state;
	state.z=zs.state;
	rate.x=xs.rate;
	rate.y=ys.rate;
	rate.z=zs.rate;
}
double MovingCoordinate::length()const{
	double r(sqrt( square(state.x) + square(state.y) + square(state.z)));
	return r;
}


std::ostream &operator<<(std::ostream &os, const MovingCoordinate &obj){
	os	<<obj.state<<"\t"<<obj.rate<<"\t";
	return os;
}





