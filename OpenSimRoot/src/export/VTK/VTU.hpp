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
/*
 * SHORT DESCRIPTION: Class for VTK output. This will write a triangular mesh. Each
 * triangular mesh forms a cone. Each cone is a numbered cell and has some data attached
 * to it. So each cell corresponds to one datapoint in simroot.
 *
 * Each VTU file visualizes the state of the model at a certain time.
 * The PVD file references the vtu files so we get a number of frames that form an
 * animation. Open the pvd with paraview to visualize the model output.
 *
 * More info - read paraview tutorial and or vtk fileformat reference
 *
 * COMMENTS: Known bug: if you visualize data and the 'shoot' at the same time - the data
 * will not show up in paraview - since there is no data associated to the shoot.
 *
 *
 */
#ifndef VTU_HPP_
#define VTU_HPP_

#include <map>
#include <set>
#include <fstream>
#include "../../export/ExportBaseClass.hpp"

//forward declaration
class SimulaBase;
class circle;



//number of points per circle
#define NRC_VTU 10
class VTU:public ExportBase{
public:
	VTU(const std::string&);
	~VTU();
	virtual void initialize();
	virtual void run(const Time &t);
	virtual void finalize();
protected:
	void storeCircleShoot(const Time &t, SimulaBase* probe, std::vector<circle> &v);
	void storeCircleRoot(const Time &t, SimulaBase* probe, std::vector<circle> &v);
	void addData(const Time &t, circle &c, SimulaBase* probe);
	bool writeRoots,writeShoots,pointData,warning,includeNutrientVTU;
	std::set<std::string> variableList;
	std::set<std::string> nutrients;
	std::ofstream pvd_os;
	std::string filename;
	int plantCount;
	int plantType;
	std::vector<std::string> plantTypes;
	void loadList(const std::string & name, SimulaBase* p);
};



#endif /*VTU_HPP_*/
