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

#ifndef SIMULASTOCHASTIC_HPP_
#define SIMULASTOCHASTIC_HPP_

#include <typeinfo>
#include "SimulaBase.hpp"
#include "DataDefinitions/Units.hpp"
#include "../cli/Messages.hpp"
#include "../tools/StringExtensions.hpp"
#include "../math/MathLibrary.hpp"
#include "Origin.hpp"
#include <cfloat>
#include <random>
#include <functional>

//static  boost::minstd_rand generator(seed);
extern std::mt19937 generator;
void setRandomSeed(int seed);

#define PULL(rc)\
	rc=dist(generator);\
	if(n==-1){\
		SimulaBase *p=ORIGIN->getChild("simulationControls")->existingChild("SimulaStochastic::numberOfSamples");\
		if(p) {\
			p->get(n);\
		}else{\
			n=1;\
		}\
	}\
	for(int i=1;i<n;++i){\
		rc+=dist(generator);\
	}\
	rc/=n;\
	if(rc<l1)rc=l1;\
	if(rc>l2)rc=l2;

template <class T, class D >
class SimulaStochastic:public SimulaBase{
public:
	typedef D Dist;
	typedef typename D::result_type Type;
	typedef std::map<int, typename D::result_type> CacheType;
	SimulaStochastic(const std::string &newName, SimulaBase* const newAttributeOf, const double &np1, const double &np2, const Unit &newUnit, const Time &newStartTime, const Time &newEndTime):
		SimulaBase(newName, newAttributeOf, newUnit, newStartTime, newEndTime),
		p1(np1),
		p2(np2),
		l1(-DBL_MAX),
		l2(DBL_MAX),
		dist(np1,np2),
		//vg(generator,dist),
		cache(nullptr),
		step_(0)
	{
	}


	SimulaStochastic(SimulaBase* newAttributeOf, const Time &startTime, const SimulaStochastic<T,D>* copyThis):
		SimulaBase(newAttributeOf, startTime, copyThis),
		p1(copyThis->p1),
		p2(copyThis->p2),
		l1(copyThis->l1),
		l2(copyThis->l2),
		dist(p1,p2),
		//vg(generator,dist),
		cache(nullptr),
		step_(0)
	{
		if(copyThis->cache){
			useCache(copyThis->step_);
			step_=copyThis->step_;//avoid conversion of the step
		}
	}
	~SimulaStochastic(){
		if (cache) delete cache;
	}

	void get(const Time &t, Time &rc)const{
		if (cache){
			if(step_>1e9) msg::error("SimulaStochastic: programmin error: caching results with a timestep of "+std::to_string(step_)+" but no time given in the get() call.");
			int intTime=0;
			typename CacheType::iterator it=cache->find(intTime);
			if (it==cache->end()){
				Type v;
				PULL(v)
				cache->insert(std::pair< int, Type >(intTime,v));
				rc=v;
			}else{
				rc=it->second;
			}
		}else{
			PULL(rc)
		}
	}
	void get(const Time &t, Time &rc){
		if (cache){
			int intTime=(int) trunc((t+TIMEERROR)*step_);
			typename CacheType::iterator it=cache->find(intTime);
			if (it==cache->end()){
				Type v;
				PULL(v)
				cache->insert(std::pair< int, Type >(intTime,v));
				rc=v;
			}else{
				rc=it->second;
			}
		}else{
			PULL(rc)
		}
	}
	void get(Time &rc){
		if (cache){
			if(step_>1e9) msg::error("SimulaStochastic: programmin error: caching results with a timestep of "+std::to_string(step_)+" but no time given in the get() call.");
			int intTime=0;
			typename CacheType::iterator it=cache->find(intTime);
			if (it==cache->end()){
				Type v;
				PULL(v)
				cache->insert(std::pair< int, Type >(intTime,v));
				rc=v;
			}else{
				rc=it->second;
			}
		}else{
			PULL(rc)
		}
	}
	std::string getType()const{
		std::string type="SimulaStochastic<";
		if(typeid(Type)==typeid(double)) {
			type+="double";
		}else if(typeid(Type)==typeid(int)) {
			type+="integer";
		}else if(typeid(Type)==typeid(Coordinate)){
			type+="Coordinate";
		}else{
			type+="unknown";
		}
		type+=">";
		return type;
	}

	void setParameters(const Type &np1,const Type &np2 ){
		p1=np1; p2=np2;
		dist = Dist(np1,np2);
	}
	void useCache(const Time & step){
		if(!cache){
			cache=new CacheType();
			if(fabs(step)<TIMEERROR){
				step_=0.;
			}else{
				step_=1./step;
			}
		}else{
			if(!cache->empty()){
				msg::warning("SimulaStochastic: changing step size for cache that is not empty");
			}
			if(fabs(step)<TIMEERROR){
				step_=0.;
			}else{
				step_=1./step;
			}
		}
	}

	void onlyPullOnce(){
		useCache(0);
	}
	void setLimits(const double &nl1,const double &nl2 ){
		l1=nl1; l2=nl2;
	}

	virtual void getXMLtag(Tag &tag){
		//name
		tag.name="SimulaStochastic";
		tag.closed=true;
		//attributes
		tag.attributes.clear();
		tag.attributes["name"]=getName();
		tag.attributes["prettyName"]=getPrettyName();
		tag.attributes["type"]=getType().substr(17,getType().size()-18);
		tag.attributes["unit"]=getUnit().name;
		if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
		if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());

		if(typeid(D)==typeid(std::uniform_int_distribution<>)) {
			tag.attributes["distribution"]="uniform";
		}else if(typeid(D)==typeid(std::uniform_real_distribution<>)) {
			tag.attributes["distribution"]="uniform";
		}else if(typeid(D)==typeid(std::normal_distribution<>)){
			tag.attributes["distribution"]="normal";
			tag.attributes["mean"]=std::to_string(p1);
			tag.attributes["stdev"]=std::to_string(p2);
		}else if(typeid(D)==typeid(std::lognormal_distribution<>)){
			tag.attributes["distribution"]="lognormal";
			tag.attributes["mean_transformed"]=std::to_string(p1);
			tag.attributes["stdev_transformed"]=std::to_string(p2);
		}else if(typeid(D)==typeid(std::weibull_distribution<>)){
			tag.attributes["distribution"]="weibull";
			tag.attributes["shape"]=std::to_string(p1);
			tag.attributes["scale"]=std::to_string(p2);
		}else{
			tag.attributes["distribution"]="unknown";
			tag.attributes["p1"]=std::to_string(p1);
			tag.attributes["p2"]=std::to_string(p2);
		}

		if(l1!=-DBL_MAX)tag.attributes["minimum"]=std::to_string(l1);
		if(l2!=DBL_MAX)tag.attributes["maximum"]=std::to_string(l2);

		if(cache) {
			if(step_<TIMEERROR){
				tag.attributes["sampleOnce"]="true";
			}else{
				tag.attributes["cacheWithTimeStep"]=std::to_string(1/step_);
			}
		}

		std::string on(getObjectGeneratorName());
		if(!on.empty()) tag.attributes["objectGenerator"]=on;
		//nodata
		//children
		if(getNumberOfChildren()) tag.closed=false;

	}

	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
	      return new SimulaStochastic<T,D>(attributeOf,startTime,this);
	};


	template <class TV, class DV>
	friend std::istream &operator>>(std::istream &is, SimulaStochastic<TV,DV> &obj);

protected:
	double p1,p2;//parameters
	double l1,l2;//limits
	D  dist;
	static int n;
	CacheType *cache;
	Time step_;
 };

template <class T , class D > int SimulaStochastic< T , D > ::n = -1;





//SimulaStochastic<Coordinate> specialization
template <class D>
class SimulaStochastic<Coordinate,D>:public SimulaBase{
public:
	typedef D Dist;
	typedef Coordinate Type;
	SimulaStochastic(const std::string &newName, SimulaBase* const newAttributeOf, const double &np1, const double &np2, const Unit &newUnit, const Time &newStartTime, const Time &newEndTime):
		SimulaBase(newName, newAttributeOf, newUnit, newStartTime, newEndTime),
		p1(np1),
		p2(np2),
		l1(-DBL_MAX),
		l2(DBL_MAX),
		dist(np1,np2),
		//vg(generator,dist),
		once()
	{
	}
	SimulaStochastic(SimulaBase* newAttributeOf, const Time &startTime, const SimulaStochastic<Coordinate,D>* copyThis):
		SimulaBase(newAttributeOf, startTime, copyThis),
		p1(copyThis->p1),
		p2(copyThis->p2),
		l1(-DBL_MAX),
		l2(DBL_MAX),
		dist(p1,p2),
		//vg(generator,dist),
		once()
	{
	}

	void get(const Time &t, Type &rc)const{
		rc.x=dist(generator);
		rc.y=dist(generator);
		rc.z=dist(generator);
		//sample n times and return the average if n>1
		if(n==-1){
			SimulaBase *p=ORIGIN->getChild("simulationControls")->existingChild("SimulaStochastic::numberOfSamples");
			if(p) {
				p->get(n);
			}else{
				n=1;
			}
		}
		for(int i=1;i<n;++i){
			rc.x+=dist(generator);
			rc.y+=dist(generator);
			rc.z+=dist(generator);
		}
		rc.x/=n;
		rc.y/=n;
		rc.z/=n;
		if(rc.x<l1)rc.x=l1;
		if(rc.x>l2)rc.x=l2;
		if(rc.y<l1)rc.y=l1;
		if(rc.y>l2)rc.y=l2;
		if(rc.z<l1)rc.z=l1;
		if(rc.z>l2)rc.z=l2;
	}
	void get(const Time &t, Type &rc){
		rc.x=dist(generator);
		rc.y=dist(generator);
		rc.z=dist(generator);
		//sample n times and return the average if n>1
		if(n==-1){
			SimulaBase *p=ORIGIN->getChild("simulationControls")->existingChild("SimulaStochastic::numberOfSamples");
			if(p) {
				p->get(n);
			}else{
				n=1;
			}
		}
		for(int i=1;i<n;++i){
			rc.x+=dist(generator);
			rc.y+=dist(generator);
			rc.z+=dist(generator);
		}
		rc.x/=n;
		rc.y/=n;
		rc.z/=n;
		if(rc.x<l1)rc.x=l1;
		if(rc.x>l2)rc.x=l2;
		if(rc.y<l1)rc.y=l1;
		if(rc.y>l2)rc.y=l2;
		if(rc.z<l1)rc.z=l1;
		if(rc.z>l2)rc.z=l2;
	}
	void get(Type &rc){
		rc.x=dist(generator);
		rc.y=dist(generator);
		rc.z=dist(generator);
		//sample n times and return the average if n>1
		if(n==-1){
			SimulaBase *p=ORIGIN->getChild("simulationControls")->existingChild("SimulaStochastic::numberOfSamples");
			if(p) {
				p->get(n);
			}else{
				n=1;
			}
		}
		for(int i=1;i<n;++i){
			rc.x+=dist(generator);
			rc.y+=dist(generator);
			rc.z+=dist(generator);
		}
		rc.x/=n;
		rc.y/=n;
		rc.z/=n;
		if(rc.x<l1)rc.x=l1;
		if(rc.x>l2)rc.x=l2;
		if(rc.y<l1)rc.y=l1;
		if(rc.y>l2)rc.y=l2;
		if(rc.z<l1)rc.z=l1;
		if(rc.z>l2)rc.z=l2;

	}
	void getAbsolute(const Time &t, MovingCoordinate &p){
		msg::error("SimulaStochastic<Coordinate>: getAbsolute called");
	}
	void getAbsolute(const Time &t, Coordinate &p){
		msg::error("SimulaStochastic<Coordinate>: getAbsolute called");
	}

	void setParameters(const Type &np1,const Type &np2 ){
		p1=np1; p2=np2;
		dist = Dist(np1,np2);
	}
	void onlyPullOnce(){
		msg::error("SimulaStochastic<Coordinate>::onlyPullOnce: one time sampling not implemented for type Coordinate");
	}
	void useCache(const Time & step){
		msg::error("SimulaStochastic<Coordinate>::useCache: using a cache is not implemented for type Coordinate");
	}

	std::string getType()const{
		std::string type="SimulaStochastic<Coordinate>";
		return type;
	}
	void setLimits(const double &nl1,const double &nl2 ){
		//not implemented, ignore for now
		l1=nl1; l2=nl2;
	}

	virtual void getXMLtag(Tag &tag){
		//name
		tag.name="SimulaStochastic";
		tag.closed=true;
		//attributes
		tag.attributes.clear();
		tag.attributes["name"]=getName();
		tag.attributes["prettyName"]=getPrettyName();
		tag.attributes["type"]="Coordinate";
		tag.attributes["unit"]=getUnit().name;
		if(getStartTime()>0) tag.attributes["startTime"]=std::to_string(getStartTime());
		if(getEndTime()>0) tag.attributes["endTime"]=std::to_string(getEndTime());

		if(typeid(D)==typeid(std::uniform_int_distribution<>)) {
			tag.attributes["distribution"]="uniform";
		}else if(typeid(D)==typeid(std::uniform_real_distribution<>)) {
			tag.attributes["distribution"]="uniform";
		}else if(typeid(D)==typeid(std::normal_distribution<>)){
			tag.attributes["distribution"]="normal";
			tag.attributes["mean"]=std::to_string(p1);
			tag.attributes["stdev"]=std::to_string(p2);
		}else if(typeid(D)==typeid(std::lognormal_distribution<>)){
			tag.attributes["distribution"]="lognormal";
			tag.attributes["mean_transformed"]=std::to_string(p1);
			tag.attributes["stdev_transformed"]=std::to_string(p2);
		}else if(typeid(D)==typeid(std::weibull_distribution<>)){
			tag.attributes["distribution"]="weibull";
			tag.attributes["shape"]=std::to_string(p1);
			tag.attributes["scale"]=std::to_string(p2);
		}else{
			tag.attributes["distribution"]="unknown";
			tag.attributes["p1"]=std::to_string(p1);
			tag.attributes["p2"]=std::to_string(p2);
		}

		if(l1!=-DBL_MAX)tag.attributes["minimum"]=std::to_string(l1);
		if(l2!=DBL_MAX)tag.attributes["maximum"]=std::to_string(l2);

		std::string on(getObjectGeneratorName());
		if(!on.empty()) tag.attributes["objectGenerator"]=on;
		//nodata
		//children
		if(getNumberOfChildren()) tag.closed=false;

	}


	virtual SimulaBase* createAcopy(SimulaBase * attributeOf, const Time & startTime)const{
	      return new SimulaStochastic<Coordinate,D>(attributeOf,startTime,this);
	};

	template <class TV, class DV>
	friend std::istream &operator>>(std::istream &is, SimulaStochastic<TV,DV> &obj);

protected:
	double p1,p2,l1,l2;
	D  dist;
	//std::variate_generator< std::minstd_rand &  , Dist > vg;
	static int n;
	Coordinate once;
};
template < class D > int SimulaStochastic< Coordinate , D > ::n = -1;

#endif /*SIMULASTOCHASTIC_HPP_*/


