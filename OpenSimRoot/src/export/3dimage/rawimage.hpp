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

/**
 * writes a raster image of the root data
 * can do depzone like depletion zones with overlap.
 *
 */
#ifndef RAWIMAGE_HPP_
#define RAWIMAGE_HPP_

#include <map>
#include <set>
#include <fstream>
#include "../../export/ExportBaseClass.hpp"
#include "../../export/Text/TabledOutput.hpp"

//forward declaration
class SimulaBase;

struct rasterData{
	typedef std::map<std::string,double> Data;
	Coordinate centre;
	Data data;
};


//number of points per circle
#define NRC_VTU 10
class RawImage:public ExportBase{
public:
	RawImage(const std::string &name);
	~RawImage();
	virtual void initialize();
	virtual void run(const Time &t);
	virtual void finalize();
protected:
	void rasterRoot(const Time &t, const std::string& radiusSimulatorName, const double & factor, SimulaBase* probe, std::map<int,float> &v, std::map<int,float> &vtmp, double & errorvol);
	std::ofstream pvd_os;
	std::string filename;
	//void addData(const Time &t, circle &c, SimulaBase* probe);
	/*std::set<std::string> variableList;
	std::set<std::string> nutrients;
	int plantCount;
	int plantType;
	std::vector<std::string> plantTypes;*/
	double resolution,dilation, smoothing,voxelvolume;
	bool includeRAW,includeVTI,additive;
	int substeps;
	Coordinate minCorner, maxCorner;
	int dimx,dimy,dimz;
	std::set<std::string> variableList;
	Table* tabledOutput;

};



#endif /*VTU_HPP_*/
