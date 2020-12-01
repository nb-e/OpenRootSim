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

#include "Output.hpp"

#include "../../engine/Origin.hpp"

#include "Mapping.hpp"

std::ofstream OutputSoilVTK::pvd_os, OutputSoilVTK::os;
std::string OutputSoilVTK::filename;


//constructor and destructor never called explicitly, unless we do it inside the member code.
OutputSoilVTK::OutputSoilVTK(const Watflow * water_, const std::vector<Solute *> &solute_):
	water(water_), solute(solute_),
	meshi(getFinestMesh()),
	fine(( !solute.empty() && solute[meshi]->getMesh().getNumNP() > water->getMesh().getNumNP()) ? true : false ),
	mesh( fine? solute[meshi]->getMesh(): water->getMesh()),
	x ( mesh.getCordX()),
	y ( mesh.getCordY()),
	z ( mesh.getCordZ()),
	numnp ( mesh.getNumNP()),
	nx ( mesh.getnx()),
	ny ( mesh.getny()),
	nz ( mesh.getnz()){

}
// TODO if there is no nutrient, nothing should get written for solute, isn't it no problem in this class?

tIndex OutputSoilVTK::getFinestMesh() {
	tIndex numberOfSolutes(solute.size());
	int meshi = 0;
	if (numberOfSolutes > 1) {
		for (tIndex i = 0; i + 1 < numberOfSolutes; ++i) {
			if (solute[meshi]->getMesh().getNumNP() < solute[i + 1]->getMesh().getNumNP()) {
				meshi = i + 1;
			}
		}
	}
	return meshi;
}


OutputSoilVTK::~OutputSoilVTK(){
	closePVD();
}

bool OutputSoilVTK::openPVD(){
//open pvd file
	if ( ! pvd_os.is_open() ){
		pvd_os.open( "fem.pvd" );
		if ( pvd_os.is_open() ) {
			//write pvd header
			pvd_os<<"<?xml version=\"1.0\"?>\n	 <VTKFile type=\"Collection\" version=\"0.1\">\n	  <Collection>\n";
		}else{
	    	msg::warning("SoilVTK:: Could not open pvd file.");
	    	return false;
	    }
	}
	return true;
}
void OutputSoilVTK::closePVD(){	//close pvd file
	if(pvd_os.is_open()){
	pvd_os<<"   </Collection>\n  </VTKFile>\n";
	pvd_os.close();
	}
}


void OutputSoilVTK::open(const Time &t){
	//open the file
	filename=std::to_string(t);
	filename.resize(6,0);
	std::string::size_type pos(filename.find(".",0));
	if (pos<3){
		size_t n(3-pos);
		for(std::size_t i=0; i<n; i++) filename.insert(0,"0");
		filename.resize(6,0);
	}
	std::string extension(".vtu");
	//std::string filenameshort=filename+extension;
	filename="fem"+filename+extension;
	//std::cout<<std::endl<<"writing "<<filename<<" at "<<t;
	os.open( filename.c_str() );
	//check whether opening was succesful
	if ( !os.is_open() ) msg::error("SoilVTU: Failed to open "+filename);

    int nex=nx-1;
    int ney=ny-1;
    int nez=nz-1;
    int numel=nex*ney*nez;
    os<<"<?xml version=\"1.0\"?>\n";
    os<<"<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    os<<" <UnstructuredGrid>\n";
    os<<"   <Piece NumberOfPoints=\""<<numnp<<"\" NumberOfCells=\""<<numel<<"\">\n";
    os<<"   <Points>\n";
    os<<"    <DataArray type=\"Float64\" Name=\"Position\" NumberOfComponents=\"3\" format=\"ascii\">\n";
// jouke: VTK uses different coordinate system where -y is down
// also here 0 is bottom of column - not the soil surface
    for(tIndex i=0;i<numnp;++i)
    	os<<x[i]<<" "<<z[i]<<" "<<y[i]<<"\n";
    os<<"    </DataArray>\n";
    os<<"   </Points>\n";
    os<<"   <Cells>\n";
    os<<"    <DataArray type=\"Int64\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\">\n";
    int ie=0;
    for (int iz=0;iz<nez;++iz){
        for (int iy=0;iy<ney;++iy){
            for (int ix=0;ix<nex;++ix){
            	++ie;
                int i1=ix+nx*iy+nx*ny*iz;
				int i2=i1+1;
				int i3=i1+nx;
				int i4=i1+nx+1;
				int i5=i1+nx*ny;
				int i6=i1+nx*ny+1;
				int i7=i1+nx*ny+nx;
				int i8=i1+nx*ny+nx+1;
		        os<<i5<<" "<<i6<<" "<<i7<<" "<<i8<<" "<<i1<<" "<<i2<<" "<<i3<<" "<<i4<<"\n";
            }
        }
    }
    os<<"    </DataArray>\n";
    os<<"    <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n";
    for(int i=8; i<=8*numel ; i+=8){
    	os<<i<<" ";
    }
    os<<"    </DataArray>\n";
    os<<"    <DataArray type=\"Int64\" Name=\"types\" format=\"ascii\">\n";
    for(int i=0;i<numel;++i){
    	os<<11<<" ";
    }
    os<<"    </DataArray>\n";
    os<<"   </Cells>\n";

    os<<"   <PointData>\n";

   	if(openPVD()) pvd_os<<"<DataSet timestep=\""<<t<<"\" group=\"1\" part=\"0\" file=\""<<filename<<"\"/>\n";
}


void OutputSoilVTK::addArray(const std::string& name, const std::valarray<double> &data, const Mesh_base & mesh_coarse ) const{
	if (os.is_open()) {
		os << "   <DataArray type=\"Float64\" Name=\""<<name<<"\" NumberOfComponents=\"1\" format=\"ascii\">\n";

		if(fine && data.size() != numnp) {
			// condition: data is smaller and has to be interpolated to the finer mesh
			bool factorFound = false;
			tIndex fine_nx(nx);
			tIndex fine_ny(ny);
			tIndex fine_nz(nz);
			tIndex factor;
			tIndex data_nx, data_ny, data_nz;
			tIndex factorcount = 0;
			// find the grid factor, it is a two-potency of the coarse grid.
			while ( !factorFound ){
				data_nx = (fine_nx+1)/2 ;
				data_ny = (fine_ny+1)/2 ;
				data_nz = (fine_nz+1)/2 ;
				if ( data_nx*data_ny*data_nz == data.size()  ){
					factor = 2 + factorcount;
					factorFound = true;
				} else{
					fine_nx = data_nx;
					fine_ny = data_ny;
					fine_nz = data_nz;
					factorcount += 2;
				}
			}
			tVector data_interpolated(0.,numnp);
			Mapping::interpolate(mesh_coarse, data, data_interpolated, factor);
			os<<std::scientific;
			for (auto i : data_interpolated) {
				os<<i<<" ";
			}
			os<<std::defaultfloat;
		}else{
			os<<std::scientific;
			for (auto i : data) {
				os<<i<<" ";
			}
			os<<std::defaultfloat;
		}

		os << "</DataArray>\n";
	} else {
		msg::warning("SoilVTU: Failed to write array, file is not open.");
	}
}

void OutputSoilVTK::addArray(const std::string& name, const std::valarray<int> &data) const{
	if(fine && data.size() != numnp) {
		msg::error("addArray: Tried to write integer array on fine mesh. May be size mismatch. Interpolation does not cast integer valarrays to double valarrays.");
	}

	if (os.is_open()) {
		os << "   <DataArray type=\"Float64\" Name=\""<<name<<"\" NumberOfComponents=\"1\" format=\"ascii\">\n";
		for (auto i : data) {
			os<<i<<" ";
		}
		os << "</DataArray>\n";
	} else {
		msg::warning("SoilVTU: Failed to write array, file is not open.");
	}
}


void OutputSoilVTK::close(){
	if(os.is_open()){
    os<<"   </PointData>\n\
  </Piece>\n\
 </UnstructuredGrid>\n\
</VTKFile>";
	      os.close();
	}
}


void OutputSoilVTK::outvtk_new(const double & t){

	std::valarray<double> x_(0., numnp);

	open(t);

	const tVector & hNew = water->gethNew();

	SimulaBase *soilTemperature(ORIGIN->getChild("soil")->existingChild("soilTemperature"));
	if (soilTemperature) {
		double Y;
		for (tIndex i = 0; i < numnp ; ++i) {
			soilTemperature->get(t, Coordinate(x[i], z[i], y[i]), Y);
			x_[i] = Y;
		}
		solute.empty() ? addArray("soilTemperature", x_, water->getMesh() ): addArray("soilTemperature", x_, solute[meshi]->getMesh() );// Ernst: use the water mesh if there is no solute
	}

	//todo, at the minimum time should be checked here, to communicate clearly that the given concentrations are also at that given time point etc.
	addArray("hydraulic_head", hNew, water->getMesh());

	const VanGenuchten & par(water->getPar());

	const tVector & thNew = water->getthNew();
	addArray("theta", thNew, water->getMesh());
	addArray("waterSink", water->getwSink(), water->getMesh());
	addArray("Q", water->getQ(), water->getMesh());
	addArray("SaturatedConductivity", par.getKs(), water->getMesh());

	x_.resize(water->getMesh().getNumNP()); // TODO this is ugly?!
	par.calc_fk(hNew, thNew, x_);
	addArray("Conductivity", x_, water->getMesh());

	addArray("potmask", mesh.getpotmask());

	//add arrays for each solute object
	if(!solute.empty()){  // TODO do I need this or is "it" just not looping.
		for(auto it:solute){
			auto nutrient=it->getNameNutrient();
			addArray("concentration_"+nutrient, it->getConc(), it->getMesh());
			addArray("sinkSource_"+nutrient, it->getcSink(), it->getMesh());
		}
	}//else{
//		msg::warning("OutputSoilVTK: Solute list empty."); // Ernst: This can be deleted but maybe you want to keep it as information to the user?
//	}

	this->close();
}

