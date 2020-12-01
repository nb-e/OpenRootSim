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

#include "Geometry.hpp"
#include "../../cli/Messages.hpp"
#include "../../math/MathLibrary.hpp"
#include "../../math/VectorMath.hpp"
#include "../PlantType.hpp"

RootClassID::RootClassID(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	int lev=3;
	if(pSD->getName()=="parentRootClassID")
		lev=6;
	std::string name;
	SimulaBase *p=pSD->getParent(lev)->existingChild("rootType");
	if(p) {
		p->get(name);
	}else{
		name="noParent";
		lev=9;
	}

	//append parent class name
	if(pSD->getName()=="combinedRootClassID"){
		lev=9;
		SimulaBase *p=pSD->getParent(6)->existingChild("rootType");
		if(p) {
			std::string pname;
			p->get(pname);
			name=name+"of"+pname;
		}
	}

	std::map<std::string,int>::iterator it(idList.find(name));
	if(it==idList.end()){
		//check parameter section otherwise assign unique id
		//try to look up
		if(lev!=9){
			std::string plantType;
			PLANTTYPE(plantType,pSD);
			SimulaBase *param(GETROOTPARAMETERS(plantType,name));
			param=param->existingChild("rootClassID");
			if(param) {
				param->get(id);
			}else{
				id=-1*((int)idList.size()+2);
			}
		}else{
			id=-1*((int)idList.size()+2);
		}
		idList[name]=id;
	}else{
		id=it->second;
	}
}
std::string RootClassID::getName()const{
	return "rootClassID";
}
std::map<std::string,int>  RootClassID::idList;

void RootClassID::calculate(const Time &t,int &ci){
	ci=id;
}
void RootClassID::calculate(const Time &t,double &ci){
	ci=(double)id;
}
DerivativeBase * newInstantiationRootClassID(SimulaDynamic* const pSD){
   return new RootClassID(pSD);
}


std::string RootCircumference::getName()const{
	return "rootCircumference";
}
std::string RootLength2Base::getName()const{
	return "rootLength2Base";
}

std::string RootSegmentLength::getName()const{
	return "rootSegmentLength";
}
std::string RootSegmentSurfaceArea::getName()const{
	return "rootSegmentSurfaceArea";
}
std::string RootSegmentRootHairSurfaceArea::getName()const{
	return "rootSegmentRootHairSurfaceArea";
}
std::string RootSegmentVolume::getName()const{
	return "rootSegmentVolume";
}
std::string RootSegmentVolumeCortex::getName()const{
	return "rootSegmentVolumeCortex";
}

RootLength2Base::RootLength2Base(SimulaDynamic* pSD):DerivativeBase(pSD)//,estimated(false)
{
	lg=pSD->getParent(3)->getChild("growthpoint")->getChild("rootLongitudinalGrowth","cm");
	std::size_t nmbpred=pSD->getPredictorSize();
	lg->get(pSD->getParent()->getStartTime(),lf);
	if(nmbpred<pSD->getPredictorSize()) {
		msg::warning("RootLength2Base: the length to base seems to be based on estimated data, but nodes should not be created based on estimated data?");
	}
}
void RootLength2Base::calculate(const Time &t,double &cf){
	cf=lf;
}
RootCircumference::RootCircumference(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	//diameter of the root
    rootDiameterSimulator=pSD->getSibling("rootDiameter");
}
void RootCircumference::calculate(const Time &t,double &cf){
	//diameter of the root -- note, currently this is not simulated but a constant. This will change in the future.
    rootDiameterSimulator->get(t,cf);
	//circumference
	cf*=PI;
}
RootSegmentLength::RootSegmentLength(SimulaDynamic* pSD):TotalBase(pSD)//,estimatedlf(false)//,estimatedff(false)
{
	SimulaBase *current(pSD->getParent());
	//length of the root
	lg=current->getParent(2)->getChild("growthpoint")->getChild("rootLongitudinalGrowth","cm");
	//set length of the root when this rootnode was created
	std::size_t nmbpred=pSD->getPredictorSize();
	lg->get(pSD->getParent()->getStartTime(),lf);
	if(nmbpred<pSD->getPredictorSize()) {
		msg::warning("RootSegmentLength: length based on estimated code, but this should not be the case as nodes are not created based on estimates.");
	}
	ff=-1;ft=1e9;
}
void RootSegmentLength::calculate(const Time &t,double &length){
	//this is purposely not based on the distance between coordinates. That distance may be shorter
	//than what is calculated here, since we may use less root nodes than timesteps i.e.
	//roots may wind between nodes.
	//Determine length at next point, this can not be done in the constructor, as the next point probably does not exist then.
	if(ff<0 ){
		SimulaBase * next;
		next=getNext(t);
		ft=next->getStartTime();
		if(ft>pSD->getStartTime()){ //this next point, otherwise it is the growth point which has an earlier or equal start time.
			std::size_t nmbpred=pSD->getPredictorSize();
			lg->get(ft,ff);
			if(nmbpred<pSD->getPredictorSize()) {
				msg::warning("RootSegmentLength: length based on estimated code, but this should not be the case as nodes are not created based on estimates. (2)");
			}
			ff-=lf;
		}else{
		    ft=1e9;
		}
	}

	//calculate length
	if(t>ft){
		length=ff;
	}else{
		lg->get(t,length);
		length-=lf;
		if(length<0.) length=0.;//small round of errors can occur when t=start time;
	}
}
RootSegmentSurfaceArea::RootSegmentSurfaceArea(SimulaDynamic* pSD):TotalBase(pSD)
{
	length=pSD->getSibling("rootSegmentLength");
	current=pSD->getSibling("rootDiameter");
}
void RootSegmentSurfaceArea::calculate(const Time &t,double &area){
	//check based on time what the next point is: growthpoint of datapoint
	SimulaBase * next=current->followChain(t);

	//get the diameter at these datapoints
	double r0(0);
	current->get(t,r0);r0/=2;
	double r1(0);
	next->get(t,r1);r1/=2;


	//double distance between these datapoints
	double l;
	length->get(t,l);

	//cylinder or cone geometry
	if(fabs(r0-r1)<1e-3){
			//cylinder
			area=l*r0*2*PI;
	}else{
		//we can calculate this by substracting the surface area of two cones
		//surface area cone (without the base) pi*r*s where s = square root (r^2+h^2)
		//for h of the two cylinders see volume calculation below
		//large cone h (assuming r0>r1:  (r0-r1) / l = (r0)/h so h= r0 / ((r0-r1) / l)=r0*l/(r0-r1)
		//small cone is large cone h-l
		if(r0<r1) {
			//swap
			std::swap(r0,r1);
		}
		double h0(r0*l/(r0-r1)), h1(h0-l);

		//convert h to s
		h0=sqrt(square(r0)+square(h0));
		h1=sqrt(square(r1)+square(h1));

		//area
		area =  ((r0*h0) - (r1*h1)) * PI;
	}
}

RootSegmentRootHairSurfaceArea::RootSegmentRootHairSurfaceArea(SimulaDynamic* pSD):DerivativeBase(pSD)
{
	SimulaBase *p(pSD->getParent());
	rhdensity=p->getChild("rootHairDensity");
	rhlength=p->getChild("rootHairLength");
	rhdiameter=p->getChild("rootHairDiameter");
	if(pSD->getUnit()=="cm/cm"){
		length=p->getChild("rootSegmentLength");
	}else{
		length=nullptr;
	}
}
void RootSegmentRootHairSurfaceArea::calculate(const Time &t,double &area){
	rhdensity->get(t,area);
	double d;
	rhlength->get(t,d);
	area*=d;
	rhdiameter->get(t,d);
	area*=d*PI;
	if(length){
		length->get(t,d);
		area*=d;
	}
}

RootSegmentVolume::RootSegmentVolume(SimulaDynamic* pSD):TotalBase(pSD)
{
	//current and next datapoint
	if(pSD->getName()=="rootSegmentVolume"){
		diameter=pSD->getSibling("rootDiameter");
	}else if (pSD->getName()=="rootSegmentVolumeSteel"){
		diameter=pSD->getSibling("rootDiameterSteel");
	}else if(pSD->getName()=="rootSegmentVolumeNoRCS"){
			diameter=pSD->getSibling("rootDiameterNoRCS");
	}else{
		msg::error("RootSegmentVolume: no method implemented for "+pSD->getName());
	}
	length=pSD->getSibling("rootSegmentLength");
}
void RootSegmentVolume::calculate(const Time &t,double &volume){
	//get the diameter at these datapoints
	double r0(0);
	diameter->get(t,r0);
	r0/=2;
	//check based on time what the next point is: growthpoint of datapoint
	double r1(0);
	diameter->followChain(t)->get(t,r1);
	r1/=2;

	//double distance between these datapoints
	double l;
	length->get(t,l);

	//cylinder or cone
	if(fabs(r0-r1)<1e-3){
		//cylinder
		volume=surfaceAreaCircle(r0)*l;
	}else{
		//we can do this by subtracting the volume of two cones
		//volume of cone pi*r^2*h/3
		//large cone h (assuming r0>r1:  (r0-r1) / l = (r0)/h so h= r0 / ((r0-r1) / l)=r0*l/(r0-r1)
		//small cone is large cone h-l
		if(r0<r1) {
			//swap
			double rt(r0);
			r0=r1;
			r1=rt;
		}
		//cone without tip
		double h0(r0*l/(r0-r1)),h1(h0-l);

		//volumes
		volume=  ( square(r0) * h0 - square(r1)*h1)*PI /3;
	}
}

RootSegmentVolumeCortex::RootSegmentVolumeCortex(SimulaDynamic* pSD):TotalBase(pSD)
{
	steelVolume=pSD->getSibling("rootSegmentVolumeSteel",pSD->getUnit());
	if(pSD->getName()=="rootSegmentVolumeCortexNoRCS"){
		rootVolume=pSD->getSibling("rootSegmentVolumeNoRCS",pSD->getUnit());
	}else{
		rootVolume=pSD->getSibling("rootSegmentVolume",pSD->getUnit());
	}
}
void RootSegmentVolumeCortex::calculate(const Time &t,double &volume){
	double sv;
	steelVolume->get(t,sv);
	rootVolume->get(t,volume);
	volume-=sv;
}


DerivativeBase * newInstantiationRootCircumference(SimulaDynamic* const pSD){
   return new RootCircumference(pSD);
}
DerivativeBase * newInstantiationRootLength2Base(SimulaDynamic* const pSD){
   return new RootLength2Base(pSD);
}
DerivativeBase * newInstantiationRootSegmentLength(SimulaDynamic* const pSD){
   return new RootSegmentLength(pSD);
}
DerivativeBase * newInstantiationRootSegmentSurfaceArea(SimulaDynamic* const pSD){
   return new RootSegmentSurfaceArea(pSD);
}
DerivativeBase * newInstantiationRootSegmentRootHairSurfaceArea(SimulaDynamic* const pSD){
   return new RootSegmentRootHairSurfaceArea(pSD);
}
DerivativeBase * newInstantiationRootSegmentVolume(SimulaDynamic* const pSD){
   return new RootSegmentVolume(pSD);
}
DerivativeBase * newInstantiationRootSegmentVolumeCortex(SimulaDynamic* const pSD){
   return new RootSegmentVolumeCortex(pSD);
}

class AutoRegisterGeometryInstantiationFunctions {
public:
   AutoRegisterGeometryInstantiationFunctions() {
      // register the maker with the factory
		BaseClassesMap::getDerivativeBaseClasses()["rootClassID"] = newInstantiationRootClassID;
		BaseClassesMap::getDerivativeBaseClasses()["rootCircumference"] = newInstantiationRootCircumference;
		BaseClassesMap::getDerivativeBaseClasses()["rootLength2Base"] = newInstantiationRootLength2Base;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentLength"] = newInstantiationRootSegmentLength;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentSurfaceArea"] = newInstantiationRootSegmentSurfaceArea;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentRootHairSurfaceArea"] = newInstantiationRootSegmentRootHairSurfaceArea;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentVolume"] = newInstantiationRootSegmentVolume;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentVolumeSteel"] = newInstantiationRootSegmentVolume;
		BaseClassesMap::getDerivativeBaseClasses()["rootSegmentVolumeCortex"] = newInstantiationRootSegmentVolumeCortex;
   }
};

static AutoRegisterGeometryInstantiationFunctions p;
