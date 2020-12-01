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
#include "ReadXMLfile.hpp"
#include <fstream>
#include "../../cli/Messages.hpp"
#include "../../engine/Origin.hpp"
#include "../../tools/StringExtensions.hpp"
#include "SimulaReaders.hpp"
#include <cstring>
#include <cfloat>
#include "../../modules/IntegrationMethods/IntegrationLibrary.hpp"

//TODO c++ style this should just go into a class with the read file as methods
static std::string dirname;
static std::string currentFileName;
//TODO this is not nice at all (with static global variable), but currently we do not have a way
//of doing this better
std::string getCurrentInputFileName(){
	return currentFileName;
}
std::string getCurrentInputDirName(){
	return dirname;
}

//for the Fortran code
/*extern "C"{
	void getdirname(long long int * length2, char dn[300], int length){
		SimulaBase* fl(ORIGIN->getChild("simulationControls")->existingChild("SWMSInputFolder"));
		std::string fln("SWMS_3D.IN/");//default
		if(fl){
			fl->get(fln);
			int l=fln.size();
			if(l>0){
				if(fln[0]=='/') fln.erase(fln.begin());
				if(fln[l-1]!='/') fln+='/';
			}

		}
		fln=dirname+fln;
		strcpy(dn,fln.c_str());
		length=strlen(dn);
		*length2=fln.size();
	}
}
*/ //old version

//read file
void readFile(const std::string &filename){
	//open file
	std::ifstream is( filename.c_str() );

#if _WIN32 || _WIN64
	//if this fails, try again with translating the slashes, as not all windows consoles seem to do the translation correctly
	if ( ! is.is_open() ) {
		auto fn=filename;//explicit copy
		std::replace( fn.begin(), fn.end(), '/', '\\');
		std::ifstream is( fn.c_str() );
	}
#endif

	//check for possible errors on stream
	if ( is.is_open() ) {

		currentFileName=filename;

		//see if filename includes another directory, and chance working directory - note this is not standard C++ code
		///@todo: this will not work on windows, slash should be other way, but than in cygwin it's different?
		dirname=filename;
		auto pos=dirname.find_last_of('/');
		if(pos!=std::string::npos){
			dirname.erase(pos+1);
		}else{
#if _WIN32 || _WIN64
			pos=dirname.find_last_of('\\');
			if(pos!=std::string::npos){
				dirname.erase(pos+1);
			}else{
				dirname.clear();
			}
#else
			dirname.clear();
#endif
		}


		//TODO register call back to capture errors with: register_callback method of is - same for include files further down - so we can capture errors and don't have to check al the time

		//read root tag and entity declarations
		Tag tag;tag.closed=true;
		while(tag.closed){
			tag.closed=false;
			getTag(is,tag,currentFileName);
			///@todo this should be checked, and meta data stored
		}

		if(tag.name!="SimulationModel") {
			if(tag.name=="SimulationModelIncludeFile"){
				msg::error("readFile: File "+filename+" is a SimulationModelIncludeFile, not the main file.");
			}else{
				msg::error("readFile: File "+filename+" does not start with a SimulationModel Tag but with "+tag.name+ ".");
			}
		}

		//global variables
		extern double PREFEREDTIMESTEP, SYNCTIMESTEP, MINTIMESTEP, MAXTIMESTEP;
		auto tagIt=tag.attributes.find("defaultStartingTimeStep");
		if (tagIt!=tag.attributes.end()) {
			PREFEREDTIMESTEP=convertFromString<double>(tagIt->second);
			msg::warning("ReadXMLfile: Setting default starting time step to "+tagIt->second);
		}
		tagIt=tag.attributes.find("synchronizationTimeStep");
		if (tagIt!=tag.attributes.end()) {
			SYNCTIMESTEP=convertFromString<double>(tagIt->second);
			msg::warning("ReadXMLfile: Setting default synchronization time step to "+tagIt->second);
		}
		tagIt=tag.attributes.find("defaultMaximumTimeStep");
		if (tagIt!=tag.attributes.end()) {
			MAXTIMESTEP=convertFromString<double>(tagIt->second);
			msg::warning("ReadXMLfile: Setting default maximum time step to "+tagIt->second);
		}
		tagIt=tag.attributes.find("defaultMinimumTimeStep");
		if (tagIt!=tag.attributes.end()) {
			MINTIMESTEP=convertFromString<double>(tagIt->second);
			msg::warning("ReadXMLfile: Setting default minimum time step to "+tagIt->second);
		}
		//default integration method
		//BackwardEuler ForwardEuler RungeKutta4 Fehlberg
		tagIt=tag.attributes.find("defaultIntegrationMethod");
		if (tagIt!=tag.attributes.end()) {
			auto method=tagIt->second;
			bool found(true);
			if (method=="RungeKutta4"){
				BaseClassesMap::getIntegrationClasses()["default"] = newInstantiationRungeKutta4;//
			}else if(method=="ForwardEuler"){
				BaseClassesMap::getIntegrationClasses()["default"] = newInstantiationDefaultIntegration;//
			}else if(method=="BackwardEuler"){
				BaseClassesMap::getIntegrationClasses()["default"] = newInstantiationBackwardEuler;//
			}else{
				BaseClassesMap::getIntegrationClasses()["default"] = newInstantiationRungeKutta4;//
				found=false;
			}
			if (found){
				msg::warning("ReadXMLfile: Setting default integration method to "+method);
			}else{
				msg::error("ReadXMLfile: Unknown default integration method. Use one of BackwardEuler ForwardEuler RungeKutta4 Fehlberg instead of "+method);
			}
		}else{
			//backward compatible default
			BaseClassesMap::getIntegrationClasses()["default"] = newInstantiationRungeKutta4;//
		}


		//read children
		std::string ctag='/'+tag.name;
		readTags(is, ORIGIN, ctag);

		//close file
		is.close();
	}else{
		msg::error("readModelFile: Failed to open "+filename);
	}
}

//tag reading function
void readTags(std::istream &is, SimulaBase* const parent, const std::string &pctag){
	while (is && !is.eof()){
		//read tag
		Tag tag;
		getTag(is,tag,currentFileName);

		//Return if this is the closing tag of the parent otherwise continue to read a new nested tag
		if(tag==pctag) break;

		//if this is a closing tag, but the wrong one, give a message, instead of tag2object giving a vague message
		if(tag.name[0]=='/'){
			msg::error("readTags: found closing tag <"+tag.name+"> but expected closing tag <"+pctag+"> first."+filePosition(is,currentFileName));
			return;
		}

		if(tag.name.empty()) {
			while(is && !is.eof()){
				char ctmp=is.get();
				if(ctmp=='<' || ctmp=='!' || ctmp=='&'){
					is.unget();
					break;
				}
			}
		}else{
			//evaluate tag
			if(parent) tag2object(tag, is, parent);
		}

	}
}
#define CHECK_NAME \
	std::string name;\
    tagIt=tag.attributes.find("name");\
	if (tagIt==tag.attributes.end()) {\
		msg::error("CHECK_NAME: missing 'name' attribute. "+filePosition(is,currentFileName));\
	}else{\
		name=tagIt->second;\
	}


//tag evaluation function
void tag2object(const Tag &tag, std::istream &is, SimulaBase* const parent){
	auto tagIt=tag.attributes.find("replacesPreviousDeclaration");
	if (tagIt!=tag.attributes.end()){
		CHECK_NAME
		SimulaBase *p=parent->existingChild(name);
		if(p) delete p;
	}
	if (tag=="SimulaBase"){
		tag2SimulaBase(tag,is, parent);
	}else if (tag=="SimulaConstant"){
		tag2SimulaConstant(tag,is, parent);
	}else if (tag=="SimulaStochastic"){
		tag2SimulaStochastic(tag,is, parent);
	}else if (tag=="SimulaTable"){
		tag2SimulaTable(tag,is, parent);
	}else if (tag=="SimulaGrid"){
		tag2SimulaGrid(tag,is, parent);
	}else if (tag=="SimulaVariable"){
		tag2SimulaVariable(tag,is, parent);
	}else if (tag=="SimulaPoint"){
		tag2SimulaPoint(tag,is, parent);
	}else if (tag=="SimulaDerivative"){
		tag2SimulaDerivative(tag,is, parent);
	}else if (tag=="SimulaLink"){
		tag2SimulaLink(tag,is, parent);
	}else if (tag=="SimulaExternal"){
		tag2SimulaExternal(tag,is, parent);
	}else if (tag=="SimulaIncludeFile"){
		readFileInclude(tag,is,parent);
	}else if (tag=="SimulaDirective"){
		followDirective(tag,is,parent);
	}else{
		msg::error("tag2object: unidentified tag: <"+tag.name+">"+filePosition(is,currentFileName));
	}
}

///@todo these are just ugly code multiplications
//these assume tag and tagIt to be present
#define CHECK_TYPE \
	std::string type("double");\
    tagIt=tag.attributes.find("type");\
	if (tagIt!=tag.attributes.end()) {\
		type=tagIt->second;\
		/*simplify type recognition*/\
		if (type=="numeric" || type=="Double"){ type="double";}\
		else if (type=="integer"){ type="int";}\
		else if (type=="text"){ type="string";}\
		else if (type=="coordinate" || type=="Point" || type=="point"){type="Coordinate";}\
		else if (type=="Bool" || type=="bool" || type=="logical" || type=="Logical"){type="bool";}\
		else if (type=="Time" || type=="time" ){type="Time";}\
	}

#define CHECK(ATTRIBUTE,STRING)\
    tagIt=tag.attributes.find(ATTRIBUTE);\
	if (tagIt==tag.attributes.end()) {\
		msg::error("CHECK: missing '"+std::string(ATTRIBUTE)+"' attribute. "+filePosition(is,currentFileName));\
	}else{\
		STRING=tagIt->second;\
	}

#define CHECK_OPTIONAL(ATTRIBUTE,STRING,DEFAULT)\
    tagIt=tag.attributes.find(ATTRIBUTE);\
	if (tagIt==tag.attributes.end()){\
		STRING=DEFAULT;\
	}else{\
		STRING=tagIt->second;\
	}


#define CHECK_UNIT \
	Unit unit;\
    tagIt=tag.attributes.find("unit");\
	if (tagIt==tag.attributes.end()) {\
		unit=UnitRegistry::noUnit();\
	}else{\
		unit=Unit(tagIt->second);\
	}
#define CHECK_LIFETIME \
	Time startTime, endTime;\
    tagIt=tag.attributes.find("startTime");\
	if (tagIt==tag.attributes.end()) {\
		startTime=parent->getStartTime();\
	}else{\
		startTime=convertFromString<double>(tagIt->second);\
	}\
	tagIt=tag.attributes.find("endTime");\
	if (tagIt==tag.attributes.end()) {\
		endTime=parent->getEndTime();\
	}else{\
		endTime=convertFromString<double>(tagIt->second);\
	}
#define CHECK_FUNCTION \
	std::string function;\
    tagIt=tag.attributes.find("function");\
	if (tagIt==tag.attributes.end()) msg::error("CHECK_FUNCTION: missing 'function' attribute. "+filePosition(is,currentFileName));\
	function=tagIt->second;
#define CHECK_OBJECTGENERATOR \
	std::string generatorF;\
    tagIt=tag.attributes.find("objectGenerator");\
	if(tagIt!=tag.attributes.end()) generatorF=tagIt->second;
#define CHECK_RATEF \
	std::string rateF;\
    tagIt=tag.attributes.find("function");\
	if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("rateFunction");\
	if (tagIt==tag.attributes.end()) {\
		msg::error("CHECK_RATEF: missing 'rateFunction' attribute. "+filePosition(is,currentFileName));\
	}else{\
		rateF=tagIt->second;\
	}
#define CHECK_INTEGRATIONF \
	std::string integrationF;\
    tagIt=tag.attributes.find("integrationFunction");\
	if (tagIt==tag.attributes.end()) {\
		integrationF="default";\
	}else{\
		integrationF=tagIt->second;\
	}
#define CHECK_INTEGRATIONPARAMETERS(POINTER) \
    tagIt=tag.attributes.find("minTimeStep");\
	if (tagIt!=tag.attributes.end()) POINTER->minTimeStep()=convertFromString<double>(tagIt->second);\
    tagIt=tag.attributes.find("maxTimeStep");\
	if (tagIt!=tag.attributes.end()) POINTER->maxTimeStep()=convertFromString<double>(tagIt->second);\
    tagIt=tag.attributes.find("preferedTimeStep");\
	if (tagIt!=tag.attributes.end()) POINTER->preferedTimeStep()=convertFromString<double>(tagIt->second);\
	tagIt=tag.attributes.find("errorTolerance");\
	if (tagIt!=tag.attributes.end()) POINTER->setIntegrationFunctionTolerance(convertFromString<double>(tagIt->second));

#define CHECK_GARBAGECOLLECTIONPARAMETERS(POINTER) \
    tagIt=tag.attributes.find("garbageCollectionOff");\
	if (tagIt!=tag.attributes.end() && string2bool(tagIt->second)) POINTER->garbageCollectionOff();\


#define READDATA(CTAG) \
	if(!tag.closed){\
		skipComments(is);\
		is>>p;\
		skipComments(is);\
		readTags(is, p, CTAG);\
	}


void tag2SimulaBase(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_UNIT
	CHECK_NAME
	CHECK_LIFETIME
	CHECK_OBJECTGENERATOR

	//create object and load data into it
	SimulaBase* p=new SimulaBase(name,parent,unit,startTime,endTime);
	if(!generatorF.empty()) p->setFunctionPointer(generatorF);

	//reads the value and possible nested tags
	if(!tag.closed) is>>p;
}

void tag2SimulaLink(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_NAME
	CHECK_LIFETIME
	CHECK_OBJECTGENERATOR

	//create object and load data into it
	SimulaLink* p=new SimulaLink(name,parent,startTime);
	p->setEndTime(endTime);
	if(!generatorF.empty()) p->setFunctionPointer(generatorF);

	//check for the name of the link
    tagIt=tag.attributes.find("linksName");
    //if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("path");
	if (tagIt!=tag.attributes.end()) {
		p->setLinkName(tagIt->second);
	}

    //reads the value and possible nested tags
	if(!tag.closed) is>>p;
}
void tag2SimulaConstant(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_TYPE
	CHECK_NAME
	CHECK_UNIT
	CHECK_LIFETIME
	CHECK_OBJECTGENERATOR

	if(parent==ORIGIN && name=="origin"){
		//don't insert a double origin, ignore it.
		SimulaConstant<Coordinate>* p=dynamic_cast<SimulaConstant<Coordinate>*> (parent);
		READDATA("/SimulaConstant")
		return;
	}

	//create object and load data into it
	if (type=="double"){
		SimulaConstant<double>* p(new SimulaConstant<double>(name,parent,unit,startTime,endTime));
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		READDATA("/SimulaConstant")
	}else if (type=="Time"){
		SimulaConstant<Time>* p(new SimulaConstant<Time>(name,parent,unit,startTime,endTime));
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		READDATA("/SimulaConstant")
	}else if (type=="int"){
		SimulaConstant<int>* p(new SimulaConstant<int>(name,parent,unit,startTime,endTime));
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		READDATA("/SimulaConstant")
	}else if (type=="string"){
		SimulaConstant<std::string>* p(new SimulaConstant<std::string>(name,parent,unit,startTime,endTime));
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		READDATA("/SimulaConstant")
	}else if (type=="Coordinate"){
		SimulaConstant<Coordinate>* p(new SimulaConstant<Coordinate>(name,parent,startTime,endTime));
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		READDATA("/SimulaConstant")
	}else if (type=="bool" ){
		SimulaConstant<bool>* p(new SimulaConstant<bool>(name,parent,unit,startTime,endTime));
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		READDATA("/SimulaConstant")
	}else{
		msg::error("tag2Constant: unknown type attribute."+filePosition(is,currentFileName));
	}
}

#define STOCH_INSTANTIATION(TYPE,DISTR)\
		SimulaStochastic<TYPE,DISTR  >* p(new SimulaStochastic<TYPE,DISTR  >(name,parent,par1d,par2d,unit,startTime,endTime));\
		p->setLimits(convertFromString<double>(l1),convertFromString<double>(l2));\
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);\
		READDATA("/SimulaStochastic");\
		if(once=="true" || once=="True" || once=="1") p->onlyPullOnce();\
		if(step!="false")	p->useCache(convertFromString<double>(step));

//#include <boost/random/lognormal_distribution.hpp>
//#include <boost/math/distributions/lognormal.hpp>
void tag2SimulaStochastic(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_TYPE
	CHECK_NAME
	CHECK_UNIT
	CHECK_LIFETIME
	CHECK_OBJECTGENERATOR
	//check distribution
	std::string dist;
	CHECK("distribution",dist)

	//check if we shoot sample once, or many times
	std::string once;
	CHECK_OPTIONAL("sampleOnce",once,"false")

	//check if we should sample once, or many times
	std::string step;
	CHECK_OPTIONAL("cacheWithTimeStep",step,"false")

	if(once!="false" && step!="false"){
		msg::error("tag2SimulaStochastic: both sampleOnce and cacheWithTimeStep given. Only supply one of the two."+filePosition(is,currentFileName));
	}

	//check for parameters
	std::string par1,par2,l1,l2,par1_t,par2_t,maxs(std::to_string(DBL_MAX));
	if(dist=="normal"){
		CHECK("mean",par1)
		CHECK("stdev",par2)
		CHECK_OPTIONAL("minimum",l1,std::to_string(-DBL_MAX))
		CHECK_OPTIONAL("maximum",l2,std::to_string(DBL_MAX))
	}else if (dist=="lognormal"){

		CHECK_OPTIONAL("mean",par1,maxs)
		CHECK_OPTIONAL("stdev",par2,maxs)
		CHECK_OPTIONAL("minimum",l1,std::to_string(-DBL_MAX))
		CHECK_OPTIONAL("maximum",l2,maxs)
		CHECK_OPTIONAL("mean_transformed",par1_t,"NA")
		CHECK_OPTIONAL("stdev_transformed",par2_t,"NA")
		if(par1==maxs && par1_t=="NA") msg::error("tag2SimulaStochastic: need mean and stdev for simulstochastic lognormal"+filePosition(is,currentFileName));
		if(par2==maxs && par2_t=="NA") msg::error("tag2SimulaStochastic: need mean and stdev for simulstochastic lognormal"+filePosition(is,currentFileName));
	}else if (dist=="uniform"){
		CHECK("minimum",par1)
		CHECK("maximum",par2)
		l1=par1;
		l2=par2;
	}else if (dist=="weibull"){
		CHECK("shape",par1)
		CHECK("scale",par2)
		CHECK_OPTIONAL("minimum",l1,std::to_string(-DBL_MAX))
		CHECK_OPTIONAL("maximum",l2,std::to_string(DBL_MAX))
	}else{
		msg::error("tag2SimulaStochastic: unsupported distribution type."+filePosition(is,currentFileName));
	}
	double par1d(convertFromString<double>(par1)),par2d(convertFromString<double>(par2));
	if (dist=="uniform" && par1d==par2d){
		par2d+=1;//this is a hack because of the newer boost libraries not being able to handle no range. the number will still be the same for the minimum and maximum are the same
	}
	if (dist=="lognormal"){
		if(par1_t!="NA" && par2_t!="NA"){
			par1d=convertFromString<double>(par1_t);
			par2d=convertFromString<double>(par2_t);
		}else{
			if(par1d<1e-8) msg::error("tag2SimulaStochastic: lognormal distribution needs to have non zero, positive parameters."+filePosition(is,currentFileName));
			par2d=std::log(1+(pow(par2d,2.)/pow(par1d,2.)));
			par1d=std::log(par1d)-par2d/2.;
			par2d=sqrt(par2d);
		}
	}
	if(dist=="weibull" && (par1d <= 0. || par2d<=0.) ){
		msg::error("tag2SimulaStochastic: weibull distribution needs to have parameters greater than 0."+filePosition(is,currentFileName));
	}

	double l1d(convertFromString<double>(l1)),l2d(convertFromString<double>(l2));
	if(l1d>l2d){
		msg::error("tag2SimulaStochastic: minimum is greater than maximum "+filePosition(is,currentFileName));
	}

	//create object and load data into it
	if (dist=="normal" && (type=="double" ) ){
		STOCH_INSTANTIATION(double, std::normal_distribution<> )
	}else if (dist=="normal" && type=="Coordinate"){
		STOCH_INSTANTIATION(Coordinate, std::normal_distribution<>)
	}else if (dist=="lognormal" && (type=="double" )){
		//STOCH_INSTANTIATION(double,std::lognormal_distribution<>)
		STOCH_INSTANTIATION(double,std::lognormal_distribution<>)
	}else if (dist=="lognormal" && type=="Coordinate"){
		STOCH_INSTANTIATION(Coordinate,std::lognormal_distribution<>)
	}else if (dist=="uniform" && (type=="double" )){
		STOCH_INSTANTIATION(double,std::uniform_real_distribution<>)
	}else if (dist=="uniform" && type=="int"){
		STOCH_INSTANTIATION(int,std::uniform_int_distribution<> )
	}else if (dist=="uniform" && type=="Coordinate"){
		STOCH_INSTANTIATION(Coordinate,std::uniform_real_distribution<>)
	}else if (dist=="weibull" && (type=="double" )){
		STOCH_INSTANTIATION(double,std::weibull_distribution<> )
	}else{
		msg::error("tag2SimulaStochastic: unsupported type and distribution combination."+filePosition(is,currentFileName));
	}

}


void tag2SimulaGrid(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_NAME
	CHECK_UNIT
	CHECK_LIFETIME
	CHECK_OBJECTGENERATOR

	//create object and load data into it
	SimulaGrid* p=new SimulaGrid(name,parent,unit,startTime,endTime);
	if(!generatorF.empty()) p->setFunctionPointer(generatorF);
	//reads the value and possible nested tags
	is>>p;
	//interpolation method
	tagIt=tag.attributes.find("interpolationMethod");
	if (tagIt!=tag.attributes.end()){
		p->setInterpolationMethod(tagIt->second);
	}
}

void tag2SimulaTable(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_LIFETIME
	CHECK_OBJECTGENERATOR

	//check for names
	std::string nameX;
    tagIt=tag.attributes.find("name_column1");
    if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("name_colum1");
	if (tagIt==tag.attributes.end()) {
		msg::error("CHECK_NAME: missing 'name_colum1' attribute. "+filePosition(is,currentFileName));\
	}else{\
		nameX=tagIt->second;\
	}

	std::string nameY;
    tagIt=tag.attributes.find("name");
    if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("name_column2");
    if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("name_colum2");
	if (tagIt==tag.attributes.end()) msg::error("CHECK_NAME: missing 'name_colum2' attribute. "+filePosition(is,currentFileName));
	nameY=tagIt->second;


	//check for unit
	Unit unitX;
    tagIt=tag.attributes.find("unit_colum1");
    if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("unit_column1");
	if (tagIt==tag.attributes.end()) {
		unitX=UnitRegistry::noUnit();
	}else{
		unitX=Unit(tagIt->second);
	}
	Unit unitY;
    tagIt=tag.attributes.find("unit_colum2");
    if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("unit_column2");
    if (tagIt==tag.attributes.end()) tagIt=tag.attributes.find("unit");
	if (tagIt==tag.attributes.end()) {
		unitY=UnitRegistry::noUnit();
	}else{
		unitY=Unit(tagIt->second);
	}

	//create object and load data into it
	if(unitX=="day"){//time table
		SimulaTable<Time>* p=new SimulaTable<Time>(nameX,parent,unitX,nameY,unitY,startTime,endTime);
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		//reads the value and possible nested tags
		if(!tag.closed) is>>p;
		//check for function (allow function initialization to override read data
	    tagIt=tag.attributes.find("function");
		if (tagIt!=tag.attributes.end()){
			p->setRateFunction(tagIt->second);

		}
		//interpolation method
	    tagIt=tag.attributes.find("interpolationMethod");
		if (tagIt!=tag.attributes.end()){
			p->setInterpolationMethod(tagIt->second);
		}
	}else{
		SimulaTable<double>* p=new SimulaTable<double>(nameX,parent,unitX,nameY,unitY,startTime,endTime);
		if(!generatorF.empty()) p->setFunctionPointer(generatorF);
		//reads the value and possible nested tags
		is>>p;
		//interpolation method
	    tagIt=tag.attributes.find("interpolationMethod");
		if (tagIt!=tag.attributes.end()){
			p->setInterpolationMethod(tagIt->second);
		}
	}


}

void tag2SimulaVariable(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_NAME
	CHECK_UNIT
	CHECK_LIFETIME
	CHECK_RATEF
	CHECK_INTEGRATIONF
	CHECK_OBJECTGENERATOR

	//create object and load data into it
	SimulaVariable* p(new SimulaVariable(name,parent,rateF,integrationF,unit,startTime,endTime));
	//set optional parameters
	CHECK_INTEGRATIONPARAMETERS(p)
	CHECK_GARBAGECOLLECTIONPARAMETERS(p)
	//reads the value and possible nested tags
	if(!tag.closed)	is>>p;
}
void tag2SimulaPoint(const Tag &tag, std::istream &is, SimulaBase* const parent){
		//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_NAME
	CHECK_LIFETIME
	CHECK_RATEF
	CHECK_INTEGRATIONF
	CHECK_OBJECTGENERATOR

	//create object and load data into it
	SimulaPoint* p(new SimulaPoint(name,parent,rateF,integrationF,startTime,endTime));
	if(!generatorF.empty()) p->setFunctionPointer(generatorF);
	//set optional parameters
	CHECK_INTEGRATIONPARAMETERS(p)
	CHECK_GARBAGECOLLECTIONPARAMETERS(p)
	//reads the value and possible nested tags
	is>>p;

}
void tag2SimulaDerivative(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_TYPE
	CHECK_NAME
	CHECK_UNIT
	CHECK_LIFETIME
	CHECK_FUNCTION
	CHECK_OBJECTGENERATOR

	//create object

	if (type=="double"){
		SimulaDerivative<double>* p(new SimulaDerivative<double>(name,parent,function ,unit,startTime,endTime));
		if(!generatorF.empty())	p->setFunctionPointer(generatorF);
		READDATA("/SimulaDerivative")
	}else if (type=="int"){
		SimulaDerivative<int>* p(new SimulaDerivative<int>(name,parent,function ,unit,startTime,endTime));
		if(!generatorF.empty())	p->setFunctionPointer(generatorF);
		READDATA("/SimulaDerivative")
	}else if (type=="string"){
		SimulaDerivative<std::string>* p(new SimulaDerivative<std::string>(name,parent,function ,unit,startTime,endTime));
		if(!generatorF.empty())	p->setFunctionPointer(generatorF);
		READDATA("/SimulaDerivative")
	}else if (type=="Coordinate"){
		SimulaDerivative<Coordinate>* p(new SimulaDerivative<Coordinate>(name,parent,function ,unit,startTime,endTime));
		if(!generatorF.empty())	p->setFunctionPointer(generatorF);
		READDATA("/SimulaDerivative")
	}else if (type=="bool" ){
		SimulaDerivative<bool>* p(new SimulaDerivative<bool>(name,parent,function ,unit,startTime,endTime));
		if(!generatorF.empty())	p->setFunctionPointer(generatorF);
		READDATA("/SimulaDerivative")
	}else{
		msg::error("tag2SimulaDerivative: unknown type attribute."+filePosition(is,currentFileName));
	}
}
void tag2SimulaExternal(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//declare iterator
	Tag::Attributes::const_iterator tagIt;

	//checks
	CHECK_NAME
	CHECK_UNIT
	CHECK_LIFETIME
	CHECK_FUNCTION
	CHECK_OBJECTGENERATOR

	//create object
	SimulaExternal* p(new SimulaExternal(name,parent,function ,unit,startTime,endTime));
	CHECK_INTEGRATIONPARAMETERS(p)
	p->registerSignalingHook();///TODO backwards compatible default for simulaexternal at option to turn it off
	if(!generatorF.empty())	p->setFunctionPointer(generatorF);

	//move on unless there are nested objects
	if(!tag.closed) READDATA("/SimulaExternal")
}
void readFileInclude(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//get the filename from the fileName attribute in the tag
	Tag::Attributes::const_iterator tagIt(tag.attributes.find("fileName"));
	if (tagIt==tag.attributes.end()) {
		msg::error("tag2SimulaFileInclude: missing 'fileName' attribute. "+filePosition(is,currentFileName));
		return;
	}
	std::string filename(dirname+tagIt->second);

	//open new file
	std::ifstream nis(filename.c_str() );

	//check for possible errors on stream
	if ( !nis.is_open() ) msg::error("readFileInclude: Failed to open "+filename+" which is referenced in "+currentFileName);
	std::string prevFileName(currentFileName);
	currentFileName=filename;
	//msg::warning("reading file: "+filename);

	//read modeling tag
	Tag ntag;
	getTag(nis,ntag,currentFileName);
	///@todo this should be checked, and meta data stored

	if(ntag.name!="SimulationModelIncludeFile"){
		msg::error("readFile: File "+filename+" does not start with a SimulationModelIncludeFile Tag but with "+ntag.name+ ".");
	}

	//read children
	std::string ctag='/'+ntag.name;
	readTags(nis, parent, ctag);

	//close file
	nis.close();
	currentFileName=prevFileName;

	//see if this tag has a closing "/>" otherwise take closing tag of the stream
	if(tag.closed) {
		return;
	}else{
		getTag(is,ntag,currentFileName);
		if(tag!="/SimulaIncludeFile") msg::error("readFileInclude: no closing tag found."+filePosition(is,currentFileName));
	}

	//continue with the previous file
}

void followDirective(const Tag &tag, std::istream &is, SimulaBase* const parent){
	//get the direction  from the path attribute in the tag
	Tag::Attributes::const_iterator tagIt(tag.attributes.find("path"));
	if (tagIt==tag.attributes.end()) msg::error("tag2SimulaFileInclude: missing 'path' attribute. "+filePosition(is,currentFileName));
	std::string fullPath(tagIt->second);

	//see if this tag has a closing "/>" otherwise take closing tag of the stream
	if(tag.closed) {
		msg::error("followDirective: a directive that is closed right away has no meaning"+filePosition(is,currentFileName));
	}else{
		//set parent to the new parent and continue
		SimulaBase* newParent=parent->existingPath(fullPath);
		if(!newParent){
			Tag::Attributes::const_iterator tagIt(tag.attributes.find("optional"));
			if (tagIt==tag.attributes.end()) {
				msg::error("followDirective: '"+parent->getPath()+"/"+fullPath+"' is not found and not optional."+filePosition(is,currentFileName));
			}
		}
		//call to read any children
		readTags(is, newParent, "/SimulaDirective");

	}
}

