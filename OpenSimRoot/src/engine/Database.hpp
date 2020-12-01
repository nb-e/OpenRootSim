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

#ifndef DATABASE_HPP_
#define DATABASE_HPP_

#include <string>
#include <list>
#include <vector>
#include <map>
#include "DataDefinitions/CustomMap.hpp"
#include "../tools/StringExtensions.hpp"
#include "DataDefinitions/Time.hpp"
#include "DataDefinitions/Units.hpp"
#include "DataDefinitions/Coordinates.hpp"
#include "DataDefinitions/ReferenceLists.hpp"


class ObjectGenerator;
class SimulaBase;

class Database{
	friend class SimulaBase; //simulabase as access to private members of Database
private:

	//typedefinitions of both colums
	typedef std::string* Colum1;
	typedef SimulaBase* Colum2;
	//typedef Collection Colum2;
	//typedef std::map<Colum1,Colum2, LessStringPointer  > Table;
	typedef osrMap<Colum1,Colum2, LessStringPointer , std::allocator<std::pair<Colum1, Colum2> >, 1 > Table;
	typedef std::vector<Colum2> List;
	typedef  std::multimap<Coordinate,SimulaBase*> Positions;//todo duplication of typedef in simulabase

	Database(SimulaBase* pSB);
	~Database();

	//stop the updating function
	void stopUpdatefunction();
	void updateRecursively(const Time &);

	std::size_t size()const;//does not update
	std::size_t size(const Time &t);//does update, even when t=-1 in which case is gives back the true container size after updating (constructing generator).

	void getAll(List&, const Time &t);
	void getAll(List&)const;

	//functions for finding positions
	void getPositions(const Time &, const Coordinate&, Positions&);

	void store(SimulaBase*);
	//void updateName(SimulaBase* pSB, const std::string &oldName);

	//TODO following function erases pointer and should be used with care! this is a potential risk for memory leaks
	void erase(SimulaBase*);
	void clear();

	SimulaBase* getFirst(const Time &t);
	SimulaBase* getFirst()const;
	SimulaBase* getLatest()const;

	void setFunctionPointer(const std::string &name="");

	std::string getObjectGeneratorName()const;

	SimulaBase* getParent()const;
	void setParent(SimulaBase* parent);

	static ReferenceList nameList;

	void insertCopy(const Time &startTime, const Database &copyThis);
	SimulaBase* get(const std::string &name, const Time &t=-1);
	SimulaBase* get(const std::string &name, const Unit &unit="na")const;
	SimulaBase* existing(const std::string &name, const Time &t);
	SimulaBase* existing(const std::string &name, const Unit &unit)const;
	SimulaBase* existing(const std::string &name)const;
	SimulaBase* existingPath(const std::string &name, const Time &t=-1);
	SimulaBase* existingPath(const std::string &name, const Unit &unit);
	SimulaBase* getPath(const std::string &name, const Time &t=-1);
	SimulaBase* getPath(const std::string &name, const Unit &unit);

	Table data;
	ObjectGenerator* generator;
	SimulaBase* attributeOf;
	///@todo this is not necessarily an efficient structure for storing and especially finding coordinates, but for now something like octrees is too complicated
	static Positions positions;
	static Time updatedAll;
	static bool updateAllLock;
	static std::vector<SimulaBase*> hooks_;

	friend std::ostream &operator<<(std::ostream &os, Database &d);
	friend std::istream &operator>>(std::istream &is, Database &obj);
};


//generate a unique name for parent.attribute
std::string uniqueName(SimulaBase* parent);



extern"C"{
	//database requests by fortran code
	void existingdatapath(long int *y, char path[300],int l);
	void getdatabypath(double *x, double *y, char path[300],int l);
	void getdataarraybypath(double *x, double *y, double *z, double *a, int* al, char path[300],int l);
	void getspatialdatabypath(double *t, double *x, double *y, double *z, double *a, int* al, char path[300],int l);
	void getdatabypath_coordinate(double *xu,double *yu,double *zu,char path[300],int l);
	void getdatabypath_logical(bool *x,char path[300],int l);
	void getdatabypath_time(double *x,char path[300],int l);
	void getdatabypath_string(char string[300],std::size_t k);
}

#endif /*DATABASE_HPP_*/
