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
#ifndef UNITS_HPP_
#define UNITS_HPP_

#include <iostream>
#include <string>
#include <map>

//TODO this should be just taken from the unit list. however, there is no guaranty that the unit list is already in memory when the Origin is created -> this resulted in an segmentation fault.
#define UNITCOORDINATES Unit("length", "cm", 0.01)

//definition of a unit
class Unit{
private:
public:
	Unit();
	Unit(const std::string &newName);
	Unit(const char *newName);
	Unit(const std::string &nOrder, const std::string &newName, const double &newFactor);
	Unit(const Unit &copyThis);
	//unit conversion
	double getUnitConversionFactor(const Unit &target)const;
	void check(const Unit &check,const std::string &parameterName,const std::string &moduleName="")const;
	std::string name;
	double factor;
	std::string order;
	bool operator== (const Unit &u)const;
	bool operator!= (const Unit &u)const;
	bool operator< (const Unit &u)const;
	bool operator> (const Unit &u)const;
	void operator= (const Unit &u);
	Unit operator/ (const Unit &u)const;
	Unit operator* (const Unit &u)const;
	void operator/= (const Unit &u);
	void operator*= (const Unit &u);
	std::string element(unsigned int i)const;
	std::string formattedName()const;
};

class UnitRegistry{
private:
	static std::map<int,Unit> unitsById_;
	static std::map<std::string,int> idByUnits_;
	static void loadStandardUnits();
	UnitRegistry(); //private constructor so this is a singleton class of only one instance which is instantiation the first time a static function is called.
public:
	static Unit & getUnit(const int & id);
	static Unit & getUnit(const std::string & s);
	static int  getId(const Unit & unit);
	static Unit & noUnit();
};

//reading function for units
std::istream &operator>>(std::istream &is, Unit &obj);

//writing function for units
std::ostream &operator<<(std::ostream &os, const Unit &obj);

// when adding units, also include them in the string2unit function
/*namespace units {
	const Unit noUnit("NA","noUnit",1);
	const Unit kg("weight","kg",1E3);
	const Unit g("weight","g",1);
	const Unit mg("weight","mg",1E-3);
	const Unit ug("weight","ug",1E-6);

	const Unit m("length","m",1);
	const Unit cm("length","cm",1E-2);
	const Unit km("length","km",1E3);
	const Unit mm("length","mm",1E-3);
	const Unit um("length","um",1E-6);

	const Unit m2("area","m2",1);
	const Unit cm2("area","cm2",1E-4);
	const Unit km2("area","km2",1E6);
	const Unit mm2("area","mm2",1E-6);

	const Unit m3("volume","m3",1);
	const Unit cm3("volume","cm3",1E-6);
	const Unit km3("volume","km3",1E9);
	const Unit mm3("volume","mm3",1E-9);

	const Unit l("volume","l",1);
	const Unit ml("volume","ml",1E-3);
	const Unit ul("volume","ul",1E-6);

	const Unit count("count","#",1);

	const Unit M("concentration","M",1);
	const Unit mM("concentration","mM",1E-3);
	const Unit uM("concentration","uM",1E-6);

	const Unit second("time","second",1);
	const Unit minute("time","minute",60);
	const Unit hour("time","hour",3600);
	const Unit day("time","day",86400);
	const Unit week("time","week",604800);
	const Unit year("time","year",31536000); //based on 365 days a year

	const Unit radians("angle","radians",1);
	const Unit degrees("angle","degrees",57.295779513);

	const Unit uMol("amount","uMol",1E-6);

	const Unit J("energy","J",1);
	const Unit kJ("energy","kJ",1E3);
	const Unit MJ("energy","MJ",1E6);

	const Unit percent("fraction","%",1);
	const Unit percent100("fraction","100%",1E-2);
//feel free to add many more
}//end of namespace units
*/



#endif /*UNITS_HPP_*/

