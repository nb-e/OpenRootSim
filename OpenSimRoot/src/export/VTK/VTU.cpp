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
#include "../../export/VTK/VTU.hpp"
#include "../circle.hpp"

#include <iomanip>
#include "../../cli/ANSImanipulators.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../engine/Origin.hpp"
#include "../../math/MathLibrary.hpp"
#include "../../math/VectorMath.hpp"
//#include "../../FunctionLibrary/SoilModule/Kuppe/swms3d.hpp"
#include "../../import/xmlReader/Indenting.hpp"

//write a VTU (.wrl) file using triangular mesh

//write wrl output
VTU::VTU(const std::string& nameMod):ExportBase(nameMod),writeRoots(true),writeShoots(true),pointData(true),warning(false),includeNutrientVTU(false), filename("visualisationRoots.pvd"),
plantCount(0),plantType(0)
{}
VTU::~VTU(){
	this->finalize();
	//if(runModule) //unclear why this is here, or what it means
		//msg::warning("VTU::addData: Data is not simulated entering 0 in the list");
}
void VTU::loadList(const std::string  & base, SimulaBase* p){

	SimulaBase::List list;
	p->getAllChildren(list);
	for (auto &it:list){
		std::string name=base+(it)->getName();
		if((it)->hasChildren()){
			name+="/";
			loadList(name,(it));
		}else{
			variableList.insert(name);
		}
	};

}


void VTU::initialize(){
	//check parameters
	if(moduleName=="wireframe.vtu") {
		filename="visualisationRoots.wirefame.pvd";
		SimulaBase *sbp;
		sbp=controls->existingChild("includePointData");
		if(sbp)sbp->get(pointData);
		writeShoots=false;
	}else if(moduleName=="vtp") {
		filename="visualisationRoots.polylines.pvd";
		SimulaBase *sbp;
		sbp=controls->existingChild("includePointData");
		if(sbp)sbp->get(pointData);
		writeShoots=false;
	}else{
		SimulaBase *sbp;
		sbp=controls->existingChild("includeShoots");
		if(sbp) sbp->get(writeShoots);
		sbp=controls->existingChild("includeRoots");
		if(sbp)sbp->get(writeRoots);
		sbp=controls->existingChild("includePointData");
		if(sbp)sbp->get(pointData);
		sbp=controls->existingChild("includeVTUForDepletionZones");
		if(sbp)sbp->get(includeNutrientVTU);
	}
	if(pointData){
		std::string param;
		std::string::size_type pos(0);
		SimulaBase *pd=controls->existingChild("pointDataVariables");
		SimulaBase* p(ORIGIN->getChild("dataPointTemplate"));
		if(pd) pd->get(param);
		if(param.size()>0){
			while (pos<param.size()) {
				std::string ps(nextWord(param,pos,','));
				if(p->existingPath(ps)){
					variableList.insert(ps);
				}else if(ps.substr(ps.size()-4)=="Rate"){
					SimulaBase *pr(p->existingPath(ps.substr(0,ps.size()-4)));
					if(pr && pr->getType()=="SimulaVariable") variableList.insert(ps);
				}
			}
		}else{
			//insert all that are in the template
			std::string name;
			loadList(name,p);
		}
		variableList.insert("plantID");
		variableList.insert("plantTypeID");
	}


	//collect list of nutrients for which we will try to write the depletionzone output
	for(std::set<std::string>::iterator nit(variableList.begin()); nit!=variableList.end();++nit){
		std::string nn(*nit);
		std::size_t pos(nn.find("/radiusDepletionZone"));
		if(pos!=std::string::npos){
			nn.erase(pos,std::string::npos);
			nutrients.insert(nn);
		}
	}

	//write output for roots - shoots and seed seperately

	//open pvd file
	pvd_os.open( filename.c_str() );
	//check wether opening was succesfull
	if ( !pvd_os.is_open() ) msg::error("VTU::run: Failed to open visualisationRoots.pvd");
	//write pvd header
	pvd_os<<"\
<?xml version=\"1.0\"?>\n\
 <VTKFile type=\"Collection\" version=\"0.1\">\n\
  <Collection>\n";
}

void VTU::finalize(){
	//write pvd footer and close pvd file
	if(pvd_os.is_open()){
	pvd_os<<"\
   </Collection>\n\
  </VTKFile>\n";
	pvd_os.close();

	//hack to close the pvd file of simunek
	//Swms3d::terminate();

	//copy the pvd file for each depletion zone set
	//TODO remove dependence on system call and sed
	if(includeNutrientVTU){
		for(std::set<std::string>::iterator nutrient(nutrients.begin()); nutrient!=nutrients.end() ; ++nutrient){
			std::string nfilename("visualisation_"+*nutrient+"_DepletionZone.pvd");
			std::string call ("rm -f "+nfilename);
			if(system(call.c_str())==0){
				call=("sed  's/roots/"+*nutrient+"/g' "+filename+" > "+nfilename);
				if(system(call.c_str())!=0) msg::warning("VTU: Failed to execute system command for creating depletion zone output. Please execute \n"+call);
			}else{
				call=("sed  's/roots/"+*nutrient+"/g' "+filename+" > "+nfilename);
				msg::warning("VTU: Failed to execute system command for creating depletion zone output. Please execute \n"+call);
			}
		}
	}
	}
}


void VTU::run(const Time &dt){
		//container for circles
		std::vector<circle> sh;
		//open the file
		std::string filename(std::to_string(dt)), filenameshort;
		filename.resize(6,0);
		std::string::size_type pos(filename.find(".",0));
		if (pos<3){
			size_t n(3-pos);
			for(std::size_t i=0; i<n; i++) filename.insert(0,"0");
			filename.resize(6,0);
		}
		std::string extension(".vtu");
		if(moduleName=="wireframe.vtu" || moduleName=="vtp") extension="."+moduleName;
		filenameshort=filename+extension;
		filename="roots"+filename+extension;
		std::ofstream os( filename.c_str() );
		//check whether opening was succesful
		if ( !os.is_open() ) msg::error("VTU: Failed to open "+filename);
		//tell os to write fixed numbers instead of scientific notation
		os<<std::fixed;
		os.precision(3);
		//dump the model into the file
		indent()=0;
		//write header
		std::string type("UnstructuredGrid");
		if(moduleName=="vtp") type="PolyData";
		os	<<"\
<?xml version=\"1.0\"?>\n\
<VTKFile type=\""<<type<<"\" version=\"0.1\" byte_order=\"LittleEndian\">\n\
   <"<<type<<">\n";
   		//write the shoot
		if (writeShoots){
			//populate vector with circles
			sh.clear();
			storeCircleShoot(dt,ORIGIN,sh);
			//number of points and cells
			std::size_t nop(sh.size()*(NRC_VTU+1)), noc(sh.size());
			//write the coordinates
			os<<"\
	      <Piece NumberOfPoints=\""<<nop<<"\" NumberOfCells=\""<<noc<<"\">\n\
	         <Points>\n\
	            <DataArray type=\"Float32\" Name=\"Position\" NumberOfComponents=\"3\" format=\"ascii\">\n";
			//write here the coordinates
			for (std::vector<circle>::iterator lit(sh.begin()); lit!=sh.end(); ++lit){
				//write centre point
				os << lit->centre << std::endl;
				//write circle
				for (int i=0 ; i<NRC_VTU ; i++)	os << lit->c[i] << std::endl;
			}
if(sh.empty()){
	os<< 0;
}
			os<<"\
			    </DataArray>\n\
	         </Points>\n\
	         <Cells>\n\
	            <DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\">\n";
			//write here the connectivity of the cells
			//this is the leaf area disk. connectivity should be 2 3 1 4 5 1 6 7 etc for each cell
			for (unsigned int j(0) ; j<noc ; j++){
				//write cell
				for (int i=1 ; i<NRC_VTU ; i+=2){
					os <<" "<< j*NRC_VTU+i <<" "<< (j*NRC_VTU)+i+1 <<" "<< j*NRC_VTU ;
				}
				//connect last triangle with first one
				os << " "<<j*NRC_VTU+1<< std::endl;
			}
			os<<"\
	            </DataArray>\n\
	            <DataArray type=\"Int32\" Name=\"offsets\" NumberOfComponents=\"1\" format=\"ascii\">\n";
			//write here the offsets for the cells (cumulative number of connectivity integers that belong to one cell)
				for (unsigned int i(1) ; i<=noc ; i++){
					os <<" "<< (i*3*NRC_VTU/2)+1  ;
				}
				os << std::endl;
			os<<"\
	            </DataArray>\n\
	            <DataArray type=\"UInt8\"  Name=\"types\" NumberOfComponents=\"1\" format=\"ascii\">\n";
			//write here the data types for the cells see vtk file type document for types - 10 is tetrahedron and 5 is triangle and  6 triangle strip
				for (unsigned int i=0 ; i<noc ; i++){
					os <<" 6";
				}
				os << std::endl;
			os<<"\
	            </DataArray>\n\
	         </Cells>\n";
			if(pointData){
				//set scientific notation
				os<<std::scientific;
				os<<"         <CellData>\n";
				for (std::set<std::string>::iterator nit(variableList.begin()); nit!=variableList.end() ; ++nit){
					os<<"           <DataArray type=\"Float32\" Name=\""<<*nit<<"\" NumberOfComponents=\"1\" format=\"ascii\">\n";
					//write here the values

						for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
							if(it->next!=-1) os<<" "<< it->data[*nit];
						}
						if(sh.empty()){
							os<< 0;
						}

					os<<"\n           </DataArray>\n";
				}
				os<<"         </CellData>\n";
			}
			os<<"\
	      </Piece>\n";
		}//end of write shoot

		if(writeRoots){
		//populate vector with circles
		sh.clear();
		storeCircleRoot(dt,ORIGIN,sh);
		//number of points and cells
		std::size_t noc=sh.size();
		// Ernst: As far as I understand the purpose of this block is to make sure there is at least 1 cell in the output.
		// To have 1 cell there should be 2 circles, but it was actually making three and this leads to an error when you open the output with paraview because
		// there are 2 cells in the phosphorous_depletion_zone file and the connectivity part of the second cell contains values that don't exist.
		// Changing the two 3's in the two lines below this to 2's fixed this bug for me and Nathan while otherwise not influencing output.
		if(noc<2){
			while(sh.size()<2){
				//insert circles
				circle c;
				c.centre=Coordinate(0,sh.size()/100,0);
				Coordinate c0(0,1,0), np(0,0,1);
				for (int i(0) ; i<NRC_VTU ; i++){
					double angle((double)i*360.0/(double)NRC_VTU);
					c.c.push_back(c.centre+rotateAroundVector(c0,np,angle,"degrees")*0.01);
				}
				for (std::set<std::string>::iterator it(variableList.begin()); it!=variableList.end() ; ++it){
					c.data[*it]=0;
				}
				sh.push_back(c);
			}
			noc=2;
			sh[0].next=1;
			sh[1].next=-1;
		}
		std::size_t nop=noc*NRC_VTU;
		//correct the number of cells for the number of root points.
		unsigned int norp(0);
		for (std::vector<circle>::iterator lit(sh.begin()); lit!=sh.end(); ++lit){
			if(lit->next==-1){
				norp++;
			}
		}
		if(moduleName=="wireframe.vtu" || moduleName=="vtp") {
			nop=noc;
			noc=norp;
		}else{
			noc-=norp;
		}

		//tell os to write fixed numbers instead of scientific notation
		os<<std::fixed;
		os.precision(3);
		//write the coordinates
		std::string celltype("Cells");
		if(moduleName=="vtp") {
			//todo roots might be polys or the like
			celltype="Lines";
			 os<<"\
 <Piece NumberOfPoints=\""<<nop<<"\" NumberOfLines=\""<<noc<<"\" NumberOfVerts=\"0\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">\n\
    <Points>\n\
       <DataArray type=\"Float32\" Name=\"Position\" NumberOfComponents=\"3\" format=\"ascii\">\n";

		}else{
				 os<<"\
      <Piece NumberOfPoints=\""<<nop<<"\" NumberOfCells=\""<<noc<<"\">\n\
         <Points>\n\
            <DataArray type=\"Float32\" Name=\"Position\" NumberOfComponents=\"3\" format=\"ascii\">\n";
		}
		//write here the coordinates
		if(moduleName=="wireframe.vtu" || moduleName=="vtp") {
			for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
				//write center point
				os << it->centre << std::endl;
			}
		}else{
			for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
				//write center point
				//! not for roots for now - os << it->centre << std::endl;
				//write circle
				for (int i(0) ; i<NRC_VTU ; ++i){
					os << it->c[i] << std::endl;
				}
			}
		}

		os<<"\
		    </DataArray>\n\
         </Points>\n\
         <"<<celltype<<">\n\
            <DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\">\n";
		//write here the connectivity of the cells
		//this is the root surface area.
		unsigned int celloffset(0);
		if(moduleName=="wireframe.vtu" || moduleName=="vtp"){
			for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
				//if this is not the end of a root
				//if(it->next != -1){
					os <<" "<< celloffset;
				//}
				celloffset+= 1;
			}
		}else{
			for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
				//if this is not the end of a root
				if(it->next != -1){
					//write cell
					for (int i(0) ; i<NRC_VTU ; i++){
						os <<" "<< celloffset+i <<" "<< celloffset+i+NRC_VTU ;
					}
					//connect last points with first ones
					os <<" "<< celloffset <<" "<< celloffset+NRC_VTU;
					os << std::endl;
				}
				celloffset+= NRC_VTU;
			}
		}
		os << std::endl;
		os<<"\
            </DataArray>\n";
		os<<"\
            <DataArray type=\"Int32\" Name=\"offsets\" NumberOfComponents=\"1\" format=\"ascii\">\n";
		//write here the offsets for the cells (cumulative number of connectivity integers that belong to one cell)
		//each root cylinder is a cell.
			celloffset=0;
			int incroffset=(2*NRC_VTU)+2;
			if(moduleName=="wireframe.vtu" || moduleName=="vtp"){
				incroffset=1;
				for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
					celloffset+= incroffset;
					if(it->next==-1){
						os <<" "<< celloffset  ;
					}
				}
			}else{
				for (unsigned int i(0) ; i<noc ; i++){
					celloffset+= incroffset;
					os <<" "<< celloffset  ;
				}
			}
			os << std::endl;
		os<<"\
            </DataArray>\n";


		if(moduleName!="pvd"){
		os<<"\
            <DataArray type=\"UInt8\"  Name=\"types\" NumberOfComponents=\"1\" format=\"ascii\">\n";
		//write here the data types for the cells see vtk file type document for types - 10 is tetrahedron and 5 is triangle and  6 triangle strip
		    int type(moduleName=="wireframe.vtu"? 4 : 6 );
			os<< type;
			for (unsigned int i=1 ; i<noc ; i++){
				os <<" "<<type;
			}
			os << std::endl;
		os<<"\
            </DataArray>\n";
		}
		os<<"\
         </"<<celltype<<">\n";
		// point data for each root node
		if(pointData){
			//set scientific notation
			os<<std::scientific;
			os.precision(3);
			std::string dataType(moduleName=="VTU"?   "CellData" : "PointData" );
			os<<"         <"<<dataType<<">\n";
				//write here the values
				if(dataType=="CellData"){
					for (std::set<std::string>::iterator nit(variableList.begin()); nit!=variableList.end() ; ++nit){
						os<<"           <DataArray type=\"Float32\" Name=\""<<*nit<<"\" NumberOfComponents=\"1\" format=\"ascii\">\n";
						for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end(); ++it){
							if(it->next!=-1) {
								os<<" "<< it->data[*nit];
							}
						}
						os<<"\n           </DataArray>\n";
					}
				}else{
					for (std::set<std::string>::iterator nit(variableList.begin()); nit!=variableList.end() ; ++nit){
						double scale=1;
						std::string n=*nit;
						if(n=="rootDiameter"){
							n="rootRadius";
							scale=0.5;
						}
						os<<"           <DataArray type=\"Float32\" Name=\""<<n<<"\" NumberOfComponents=\"1\" format=\"ascii\">\n";
						for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end(); ++it){
							os<<" "<< it->data[*nit]*scale;
						}
						os<<"\n           </DataArray>\n";
					}
					os<<"           <DataArray type=\"Float32\" Name=\"lineSegmentLength\" NumberOfComponents=\"1\" format=\"ascii\">\n";
					for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end(); ++it){
							int n=it->next;
							if(n!=-1) {
								os<<" "<<vectorlength(sh[n].centre,it->centre);
							}else{
								os<<" "<<0;
							}
					}
					if(sh.empty()){
						os<< 0;
					}
					os<<"\n           </DataArray>\n";
					os<<std::fixed;
					os.precision(3);
					os<<"           <DataArray type=\"Float32\" Name=\"nextxyz\" NumberOfComponents=\"3\" format=\"ascii\">\n";
					for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end(); ++it){
							int n=it->next;
							if(n!=-1) {
								os<<sh[n].centre<<std::endl;
							}else{
								os<<it->centre<<std::endl;
							}
					}
					os<<"\n           </DataArray>\n";
					os<<"           <DataArray type=\"Float32\" Name=\"growthvector\" NumberOfComponents=\"3\" format=\"ascii\">\n";
					for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end(); ++it){
							int n=it->next;
							if(n!=-1) {
								os<<sh[n].centre-it->centre<<std::endl;
							}else{
								os<<Coordinate()<<std::endl;
							}
					}
					os<<"\n           </DataArray>\n";
					os<<"           <DataArray type=\"Float32\" Name=\"next\" NumberOfComponents=\"1\" format=\"ascii\">\n";
					for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end()--; ++it){
							os<<" "<<it->next;
					}
					os<<"\n           </DataArray>\n";
					os<<"           <DataArray type=\"Float32\" Name=\"previous\" NumberOfComponents=\"1\" format=\"ascii\">\n";
					int cnt=-1;
					os<<" "<<cnt;
					++cnt;
					for (std::vector<circle>::iterator it(sh.begin()); it!=sh.end(); ++it){
						if(it->next==-1){
							os<<" "<<-1;
						}else{
							os<<" "<<cnt;
						}
					    ++cnt;
					    if(cnt+1==(int)sh.size()) break;
					}
					os<<"\n           </DataArray>\n";

				}


			os<<"         </"<<dataType<<">\n";
		}
		os<<"\
      </Piece>\n";
	}//end off including root peace
		//write the footer
		os<<"\
   </"<<type<<">\n\
</VTKFile>\n";

		//close the file
		os.close();

		//write pvd line
		pvd_os<<"   <DataSet timestep=\""<<dt<<"\" group=\"\" part=\"0\" file=\""<<filename<<"\"/>\n";

		//write depletion zones
	if(includeNutrientVTU){
		for(std::set<std::string>::iterator nutrient(nutrients.begin()); nutrient!=nutrients.end() ; ++nutrient){
			//open file
			filename=*nutrient+filenameshort;
			std::ofstream os( filename.c_str() );
			if ( !os.is_open() ) msg::error("writeWRL: Failed to open "+filename);
			//write the header
			os	<<"\
<?xml version=\"1.0\"?>\n\
<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n\
   <UnstructuredGrid>\n";
			//tell os to write fixed numbers instead of scientific notation
			os<<std::fixed;
			//number of points and cells
			std::size_t noc=sh.size();  	    								///was unsigned int before, but a size is always unsigned
			std::size_t nop=noc*NRC_VTU;  										///was unsigned int before
			//correct the number of cells for the number of root points.
			std::size_t norp(0);  												///was unsigned int before
			for (std::vector<circle>::iterator lit(sh.begin()); lit!=sh.end(); ++lit){
				if(lit->next==-1){
					norp++;
				}
			}
			noc-=norp;

			//write the coordinates
			os<<"\
	      <Piece NumberOfPoints=\""<<nop<<"\" NumberOfCells=\""<<noc<<"\">\n\
	         <Points>\n\
	            <DataArray type=\"Float32\" Name=\"Position\" NumberOfComponents=\"3\" format=\"ascii\">\n";
			//write here the coordinates
			for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
				//write centre point
				//! not for roots for now - os << it->centre << std::endl;
				//use transformation to get depletion
				circle::Data::iterator dit( it->data.find(*nutrient+"/radiusDepletionZone") );
				if(dit==it->data.end())	msg::error("VTU: No data found for "+*nutrient+"/radiusDepletionZone. Make sure it's included in the list");
				double dd(dit->second);
				dit=it->data.find("rootDiameter");
				if(dit==it->data.end())	msg::error("VTU: No data found for \"rootDiameter\". Make sure it's included in the list");
				double d(dit->second/2);
				if(d==0.){
					msg::warning("VTU: root segments with 0 diameter replaced with small diameter",2);
					d=0.001;
				}
				//write
				for (int i(0) ; i<NRC_VTU ; ++i){
					Coordinate p( it->centre+((it->c[i]-it->centre)*dd/d) );
					os << p << std::endl;
				}
			}
			//if there were not roots at all - we need write some coordinates just to make sure that paraview works
			if(sh.empty()){
				for(int i(0);i<2*NRC_VTU ; ++i) os<< Coordinate()<<std::endl;
			}

			os<<"\
			    </DataArray>\n\
	         </Points>\n\
	         <Cells>\n\
	            <DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\">\n";
			//write here the connectivity of the cells
			//this is the root surface area.
			unsigned int celloffset(0);
			for (std::vector<circle>::iterator it=sh.begin(); it!=sh.end(); ++it){
				//if this is not the end of a root
				if(it->next != -1){
					//write cell
					for (int i(0) ; i<NRC_VTU ; i++){
						os <<" "<< celloffset+i <<" "<< celloffset+i+NRC_VTU ;
					}
					//connect last points with first ones
					os <<" "<< celloffset <<" "<< celloffset+NRC_VTU;
					os << std::endl;
				}
				celloffset+= NRC_VTU;
			}
			//if there were no roots at all - we need write some coordinates just to make sure that paraview works
			if(sh.empty()){
				os<< "0 1";
			}

			os<<"\
	            </DataArray>\n\
	            <DataArray type=\"Int32\" Name=\"offsets\" NumberOfComponents=\"1\" format=\"ascii\">\n";
			//write here the offsets for the cells (cumulative number of connectivity integers that belong to one cell)
			//each root cylinder is a cell.
				celloffset=0;
				for (unsigned int i(1) ; i<=noc ; i++){
					celloffset+=(2*NRC_VTU)+2;
					os <<" "<< celloffset  ;
				}
				if(sh.empty()){
					os<< " 2 ";
				}
				os << std::endl;
			os<<"\
	            </DataArray>\n\
	            <DataArray type=\"UInt8\"  Name=\"types\" NumberOfComponents=\"1\" format=\"ascii\">\n";
			//write here the data types for the cells see vtk file type document for types - 10 is tetrahedron and 5 is triangle and  6 triangle strip
				for (unsigned int i=0 ; i<noc ; i++){
					os <<" 6";
				}
				if(sh.empty()){
					os<< 6;
				}
				os << std::endl;
			os<<"\
	            </DataArray>\n\
	         </Cells>\n";
			//TODO cell and point data goes here in case we have something in the future
			os<<"\
	      </Piece>\n";
			//write the footer
			os<<"\
	   </UnstructuredGrid>\n\
	</VTKFile>\n";

			//close the file
			os.close();
		}
	}


}



void VTU::storeCircleShoot(const Time &t, SimulaBase* probe, std::vector<circle> &v){
	//skip if this is a template
	///@todo, there maybe an case insensitive flag that we can use here?
	if (probe->getName().find( "Template" , 0 ) != std::string::npos) return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "parameters" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "Parameters" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "primaryRoot" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "hypocotyl" , 0 ) != std::string::npos) return;

	//probe this object
	if(probe->getName()=="shoot"){
		// plant position as centre point of the circle
		Coordinate cp;
		probe->getParent()->getSibling("plantPosition")->getAbsolute (t,cp);
		cp.y=2;

		// radius based on the current leaf area
		double r(0);
		SimulaBase *la(probe->existingChild("leafArea",t));
		if(la) la->get(t,r);
		if (r>0){
			r=sqrt(r)/PI;

			//perpendicular
			Coordinate pp(0,1,0);

			//first point relative to origin
			Coordinate c0(r,0,0);

			//circle
			circle c;
			c.centre=cp;
			for (int i(0) ; i<NRC_VTU ; i++){
				c.c.push_back(cp+(rotateAroundVector(c0,pp,i*(360/NRC_VTU),"degrees")*r));
			}

			//add Data (Note this results probably in 0 values for the shoot, but without data vtk will either crash or not show the data
			if(pointData) addData(t,c,la);

			//insert circle
			v.push_back(c);
		}
	} else {
		//probe the attributes of this object
		SimulaBase::List allattributes;
			probe->getAllChildren(allattributes,t);
			for (auto & it:allattributes){
				storeCircleShoot(t, it, v);
			}

	}
}


void VTU::storeCircleRoot(const Time &t, SimulaBase* probe, std::vector<circle> &v){
	//skip if this is a template
	///@todo, there maybe an case insensitive flag that we can use here?
	if (probe->getName().find( "Template" , 0 ) != std::string::npos) return;
	//if (probe->getName().find( "template" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "parameters" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "Parameters" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "growthpoint" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "soil" , 0 ) != std::string::npos) return;
	if (probe->getName().find( "atmosphere" , 0 ) != std::string::npos) return;

	//probe this object
	if(probe->getName()=="dataPoints"){
		//for each datapoint insert a circle
		SimulaBase::List l;
		probe->getAllChildren(l,t);
		//add growthpoint to bottom of the list
		l.push_back(probe->getSibling("growthpoint"));
		for (auto it(l.begin());it!=l.end();++it){
			//check if size of l is at least 2
			if(l.size()<2) break;

			//structure for storing coordinates and data at the root node
			circle c;

			// next point
			Coordinate np;
			auto nit(it);
			++nit;
			if(nit==l.end()){
				----nit;
				c.next=-1;
			}else{
				c.next=(int)v.size()+1;
			}

			// center point relative
			Coordinate cp;
			(*it)->get(t,cp);
			//next center point
			(*nit)->get(t,np);
			np-=cp;

			//distance between points
			double cp_np_length(vectorlength(np));
			if(cp_np_length<0.0002){
				//remove nit form list
				--it;
				--nit;
				l.erase(nit);
				continue;
			}

			// diameter
			double d;
			(*it)->getChild("rootDiameter")->get(t,d);
			d/=2;
			if(d<0.0001)d=0.0001;//make sure the root as not a 0 diameter
			//if(next==-1) d/-2; //make the roottip a bit thinner.

			//correct for curvature of the root
			double k(1);
			if(it!=l.begin()){
				auto pit(it);
				--pit;
				//get position of prev coordinate
				Coordinate pp;
				(*pit)->get(t,pp);
				pp-=cp;
				//flip this
				pp*=-1;
				normalizeVector(pp);
				pp*=cp_np_length;
				//use midpoint between pp and np
				np=(pp+np)/2;
				if(np.length()==0) np=pp;
				//diameter correction
				k=sqrt(square(cp_np_length)-(np^pp));
				k!=0 ? k=cp_np_length/k : k=1;
			}

			//first point perpendicular to surface pp, cp, and an arbritary point that is outside pp->cp
			Coordinate a(1,1,1);
			Coordinate c0(perpendicular(a,np));

			//fill the circles coordinates
			(*it)->getAbsolute(t,cp);
			c.centre=cp;
			for (int i(0) ; i<NRC_VTU ; i++){
				double angle((double)i*360.0/(double)NRC_VTU);
				//note this correction factor is an approximation - but does not create a true ellipse
				//double corr(360-( 90.0*int(angle/90) ));
				///@todo: the direction of the correction factor is wrong - remember perpendicular may be in any direction
				//corr=1;//+((k-1)*(1-corr/90));
				c.c.push_back(cp+rotateAroundVector(c0,np,angle,"degrees")*d/*corr*/);
			}

			//rotate circle points so that it doesn't twist with previous circle
			if(!v.empty() && c.next>0 && v.rbegin()->next>0){
				//create a copy of the circle
				circle c_shifted(c);
				//shift the copy closer to the previous circle, but make sure they do not overlap, afterall they have different orientation
				Coordinate shift( c.centre-v.rbegin()->centre );
				//TODO for now ignore orientation shift*=(1-(d*2/vectorlength(shift)).value);
				for(circle::C::iterator it(c_shifted.c.begin()) ; it!=c_shifted.c.end() ; ++it){
					//shift closer
					*(it)-=shift;
					//TODO fit diameter? if diameter is wildly different finding the nearest point won't work.

				}
				//determine nearest point
				double sd(1E10); int j(0),i(0);
				///@todo this could be faster ?? - since if we see length increasing we could forward with 0.5*c.c.size()
				circle::C::iterator nit(v.rbegin()->c.begin());
				for (circle::C::iterator it(c_shifted.c.begin());it!=c_shifted.c.end();++it){
					double l(vectorlength(*it,*nit));
					if(l<sd) {
						sd=l;
						j=i;
					}
					++i;
				}
				//determine rotation
				if (j<0) j+=NRC_VTU;
				//rotate positions
				for (int i(0); i<j ; i++){
					c.c.push_back(*c.c.begin());
					c.c.erase(c.c.begin());
				}
			}

			//at any data to the circle
			if(pointData) addData(t,c,*it);

			//insert circle
			v.push_back(c);
		}

	} else {
		//set plant type and id etc
		if(probe->getName()=="plantPosition"){
			++plantCount;
			std::string nm;
			probe->getSibling("plantType")->get(nm);
			plantType=-1;
			for(unsigned int i(0); i<plantTypes.size(); ++i){
				if(plantTypes[i]==nm){
					plantType=i;
					break;
				}
			}
			if(plantType<0){
				plantType=(int)plantTypes.size();
				plantTypes.push_back(nm);
			}
		}

		//probe the attributes of this object
			SimulaBase::List allattributes;
			probe->getAllChildren(allattributes,t);
			for (auto & it:allattributes){
				storeCircleRoot(t, it, v);
			}
	}
}

void VTU::addData(const Time &t, circle &c, SimulaBase* probe){
	//insert optional data
	double d;
	for (std::set<std::string>::iterator it(variableList.begin()); it!=variableList.end() ; ++it){
		if(*it=="plantID"){
			//add plant id
			c.data["plantID"]=plantCount;
			continue;
		}
		if(*it=="plantTypeID"){
			//add plant id
			c.data["plantTypeID"]=plantType;
			continue;
		}
		if(*it=="plantTypeID"){
			//add plant id
			c.data["plantTypeID"]=plantType;
			continue;
		}
		SimulaBase* p(probe->existingPath(*it));
		if(p) {
			p->get(t,d);
			c.data[*it]=d;
		}else if(it->substr(it->size()-4)=="Rate"){
			p=(probe->existingPath(it->substr(0,it->size()-4)));
			if (p){
				p->getRate(t,d);
				c.data[*it]=d;
			}
		}
		if(!p){
			if(!warning) warning=true;
			c.data[*it]=0;
		}
	}
}



