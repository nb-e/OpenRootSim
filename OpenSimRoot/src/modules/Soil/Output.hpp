/*
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

#ifndef OUTPUT_H_
#define OUTPUT_H_

// basic file operations
#include <iostream>
#include <fstream>
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../engine/DataDefinitions/Time.hpp"
#include <valarray>
#include <string>
#include "Mesh.hpp"
#include "Solute.hpp"
#include "Watflow.hpp"


class OutputSoilVTK {
public:
	OutputSoilVTK(const Watflow * water, const std::vector<Solute *> &solute);
	~OutputSoilVTK();
	static std::ofstream pvd_os,os;
	static std::string filename;

	void outvtk_new(const double & t);
private:
	void open(const Time &t);
	void addArray(const std::string& name, const std::valarray<double> &data, const Mesh_base & mesh_coarse )const;
	void addArray(const std::string& name, const std::valarray<int> &data )const;
	void close();
	bool openPVD();
	void closePVD();

	tIndex getFinestMesh();

	const Watflow * water;
	const std::vector<Solute *> &solute;
	tIndex meshi;

	const bool fine;

	const Mesh_base & mesh;
	//todo, safer would be std::vector<Coordinate>
	const std::valarray<double> &x;
	const std::valarray<double> &y;
	const std::valarray<double> &z;

	const tIndex numnp;
	const int nx;
	const int ny;
	const int nz;
};

#endif /* OUTPUT_H_ */
