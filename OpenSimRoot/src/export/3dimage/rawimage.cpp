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
#include "../../export/3dimage/rawimage.hpp"

#include <iomanip>
#include "../../cli/ANSImanipulators.hpp"
#include "../../cli/Messages.hpp"
#include "../../tools/StringExtensions.hpp"
#include "../../engine/SimulaBase.hpp"
#include "../../engine/Origin.hpp"
#include "../../math/VectorMath.hpp"
#include <math.h>
#if _WIN32 || _WIN64
#include <algorithm>
#endif

extern ModuleList modules;

//write a VTU (.wrl) file using triangular mesh


//write wrl output
RawImage::RawImage(const std::string &name):ExportBase(name), filename(name+".pvd"),
		resolution(0),dilation(0),smoothing(0),voxelvolume(0),
		includeRAW(false), includeVTI(false), additive(true),
		substeps(0),
		dimx(0),dimy(0),dimz(0),
		tabledOutput(nullptr)
{}
RawImage::~RawImage(){
}
void RawImage::initialize(){
	//resolution (voxels/cm)
	double dummy;
	controls->getChild("voxelsize","cm")->get(dummy);
	voxelvolume=pow(dummy,3);
	resolution=1/dummy;
	//make roots thicker
	controls->getChild("rootDiameterScalingFactor","100%")->get(dilation);
	controls->getChild("edgeSmoothing","cm")->get(smoothing);
	controls->getChild("substeps","#")->get(substeps);
	controls->getChild("includeBinaryRawImage")->get(includeRAW);
	if(includeRAW) msg::warning("includeraw");
	controls->getChild("includeVTKImage")->get(includeVTI);

	SimulaBase* ctr=controls->existingChild("minCorner");
	if(ctr){
		ctr->get(minCorner);
	}else{
		ORIGIN->getChild("environment")->getChild("dimensions")->getChild("minCorner")->get(minCorner);
	}
	ctr=controls->existingChild("maxCorner");
	if(ctr){
		ctr->get(maxCorner);
	}else{
		ORIGIN->getChild("environment")->getChild("dimensions")->getChild("maxCorner")->get(maxCorner);
	}
	maxCorner+=Coordinate(0.5,0.5,0.5);
	minCorner-=Coordinate(0.5,0.5,0.5);
	Coordinate mc=maxCorner-minCorner;
	mc*=resolution;
	dimx=(int)ceil(mc.x+1);
	dimy=(int)ceil(mc.y+1);
	dimz=(int)ceil(mc.z+1);

	//open pvd file
	if(includeVTI){
		pvd_os.open( filename.c_str() );
		//check wether opening was succesfull
		if ( !pvd_os.is_open() ) msg::error("RawImage::run: Failed to open "+filename);
		//write pvd header
		pvd_os<<"\
<?xml version=\"1.0\"?>\n\
 <VTKFile type=\"Collection\" version=\"0.1\">\n\
  <Collection>\n";
	}

	//datanames
	std::string param;
	SimulaBase* p=controls->existingChild("radiusComesFrom");
	if(p){
		p->get(param);
	}else{
		param="rootDiameter"; // or "phosphorus/radiusDepletionZone" etc
	}

	std::string::size_type pos(0);
	p=(ORIGIN->getChild("dataPointTemplate"));
	while (pos<param.size()) {
		std::string ps(nextWord(param,pos,','));
		if(p->existingPath(ps)){
			variableList.insert(ps);
		}else if(ps.substr(ps.size()-4)=="Rate"){
			SimulaBase *pr(p->existingPath(ps.substr(0,ps.size()-4)));
			if(pr && pr->getType()=="SimulaVariable") variableList.insert(ps);
		}
	}

	//additive
	p=controls->existingChild("overlapFunction");
	if(p){
		std::string fn;
		p->get(fn);
		if(fn=="additive"){
			additive=true;
		}else if (fn=="maximum"){
			additive=false;
		}

	}else{
		additive=false;
	}

	//see if we can write data to the tabled output
	tabledOutput=nullptr;
	for(ModuleList::iterator it=modules.begin(); it!= modules.end() ; ++it){
		if((*it)->getName()=="table"){
			tabledOutput=dynamic_cast<Table*>(*it);
			if(!tabledOutput) msg::warning("RawImage: found tabled output");
		}
	}
}

void RawImage::finalize(){
	//write pvd footer and close pvd file
	if(includeVTI){
	pvd_os<<"\
   </Collection>\n\
  </VTKFile>\n";
	pvd_os.close();
	}
}


void RawImage::run(const Time &dt){
		//open the file
		std::string filename(std::to_string(dt)), filenameshort;
		filename.resize(6,0);
		std::string::size_type pos(filename.find(".",0));
		if (pos<3){
			size_t n(3-pos);
			for(std::size_t i=0; i<n; i++) filename.insert(0,"0");
			filename.resize(6,0);
		}
		filenameshort=filename;


		//write vtk image
		std::ofstream os,ros;
		if(includeVTI){
			filename="roots_"+moduleName+"_"+filenameshort+".vti";
			os.open( filename.c_str() );
			if ( !os.is_open() ) msg::error("RawImage: Failed to open "+filename);

			//spacing
			double s(1/resolution);

			//write header
			os	<<"\
<?xml version=\"1.0\"?>\n\
<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n\
<ImageData WholeExtent=\""<<0<<" "<<dimx-1<<" "<< 0<<" "<< dimy-1<<" "<< 0<<" "<< dimz-1<<"\" Origin=\""<<minCorner<<"\" Spacing=\""<<s<<" "<<s<<" "<<s<<"\">\n\
	<Piece Extent=\""<<0<<" "<<dimx-1<<" "<< 0<<" "<< dimy-1<<" "<< 0<<" "<< dimz-1<<"\">\n\
		<PointData>\n";
		}

		double errorvol(0);
		///todo we using three times the image size, which costs a lot of memory.
		/// we could reduce vrd and v to hash_map which would not store all the zero's but has slower access. but we iterate through most of the time anyway, only copy vtmp to the hash would be slower.
		//std::vector<float> v(dimx*dimy*dimz, 0);
		//std::vector<float> vrd(v.size(), 0);
		std::map<int,float> v,vrd;
		std::map<int,float> vtmp;
		rasterRoot(dt,"rootDiameter", 2,ORIGIN,vrd,vtmp,errorvol);

		for ( std::set<std::string>::iterator it(variableList.begin()) ; it!=variableList.end() ; ++it ){
			//container for data points
			v.clear();
			if((*it)=="rootDiameter"){
				v.insert(vrd.begin(),vrd.end());
				//for(unsigned int i=0; i<v.size(); ++i){
				//for(std::map<int,float>::iterator it(v.begin()); it!=v.end(); ++it){
					//v[i]=vrd[i];
				//}
			}else{
				//for(unsigned int i=0; i<v.size() ; ++i){
				//	v[i]=0;
				//}
				errorvol=0;
				rasterRoot(dt,*it, 1,ORIGIN,v,vtmp,errorvol);
				if (errorvol>0.01) msg::warning("RawImage: ignoring roots outside grid. Volume error for "+*it+" at time "+std::to_string(dt)+" = "+std::to_string(errorvol));
			}


			//do raw image
			if(includeRAW && (*it)=="rootDiameter"){
				filename="roots_"+moduleName+"_"+filenameshort+"_dim_xyz_"+std::to_string(dimx)+","+std::to_string(dimy)+","+std::to_string(dimz)+"_ori_xyz_"+std::to_string(minCorner.x)+","+std::to_string(minCorner.y)+","+std::to_string(minCorner.z)+".raw";

				ros.open(filename.c_str());
				if ( !ros.is_open() ) msg::error("RawImage: Failed to open "+filename);


				//write the coordinates
				//os.write((const char*)&v[0], v.size() * sizeof(float));
				int index(0);
				float value;
				for(std::map<int,float>::iterator it(v.begin()); it!=v.end(); ++it){
					//write zero's
					value=0;
					for(;index<it->first;++index){
						ros.write((const char*) &value, sizeof(value));
					}
					//write values
					++index;
					value=it->second;
					ros.write((const char*) &value, sizeof(value));
				}
				value=0;
				for(;index<dimx*dimy*dimz;++index){
					ros.write((const char*) &value, sizeof(value));
				}
				ros.close();

			}

			if(includeVTI){

		//write biomass data
				os<<"				<DataArray type=\"Float32\" Name=\""<< *it <<"\" NumberOfComponents=\"1\" format=\"ascii\">\n";
				int cnt(0);
				/*for(unsigned int i(0); i< v.size() ; ++i){
					os<<v[i]<<" ";
					if(cnt>50){
						cnt=0;
						os<<"\n";
					}
					++cnt;
				}*/
				int index(0);
				float value;
				for(std::map<int,float>::iterator mit(v.begin()); mit!=v.end(); ++mit){
					//write zero's
					value=0;
					for(;index<mit->first;++index){
						os<<value<<" ";
						++cnt;
					}
					//write values
					++index;
					value=mit->second;
					os<<value<<" ";
					++cnt;
					if(cnt>50){
						cnt=0;
						os<<"\n";
					}
				}
				value=0;
				for(;index<dimx*dimy*dimz;++index){
					os<<value<<" ";
					++cnt;
				}

				os	<<"\n				</DataArray>\n";
			}

			if(tabledOutput && it->find("DepletionZone")!=std::string::npos){
				//calculate overlap depletion zones and write to tabled output
				double perc,overlap(0),deplvol(0);
/*					for(unsigned int i=0; i<v.size(); ++i){
					perc=v[i];
					if(vrd[i]>1){
						//this is inside the root
						continue;
					}else if(vrd[i]>0.001 && perc+vrd[i]>1.001){
						//correct perc for root overlap, this is not accurate, but
						perc*=(1-vrd[i]);
					}
					deplvol+=minimum(perc,1);
					if(perc>1)overlap+=minimum(perc-1,1);

				}*/
				float bmv;
				for(std::map<int,float>::iterator mit(v.begin()); mit!=v.end(); ++mit){
					perc=mit->second;
					std::map<int,float>::iterator itb=vrd.find(mit->first);
					bmv=1;
					if(itb != vrd.end()){
						bmv=itb->second;
						if(bmv>1){
							//this is inside the root
							continue;
						}
						//correct perc for root overlap, this is not accurate, but
						bmv=1-bmv;
					}
					deplvol+=std::min(perc,(double)bmv);
					if(perc>1)overlap+=std::min((perc-1)*(double)bmv,(double)bmv);
				}
				deplvol*=voxelvolume;
				overlap*=voxelvolume/deplvol;
				std::string::size_type pos=0;
				std::string pathstr("//soil/"+nextWord(*it,pos,'/'));
				tabledOutput->writeLine("OverlapDepletionzones",dt,overlap,0,"100%",pathstr);
				tabledOutput->writeLine("VolumeDepletionzones",dt,deplvol,0,"cm3",pathstr);
			}
		}

		if(includeVTI){

			os	<<"			</PointData>\n";
	/*		os	<<"		<CellData>";
			//write cell data

			os	<<"</CellData>\n";*/
			os	<<"		</Piece>\n\
	</ImageData>\n\
</VTKFile>\n";

			os.close();
		}

		//write pvd line
		pvd_os<<"   <DataSet timestep=\""<<dt<<"\" group=\"\" part=\"0\" file=\""<<filename<<"\"/>\n";


}




void RawImage::rasterRoot(const Time &t, const std::string& radiusSimulatorName, const double & factor, SimulaBase* probe, std::map<int,float> &v, std::map<int,float> &vtmp, double & errorvol){
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
		vtmp.clear();
/*		for(unsigned int i=0; i<vtmp.size() ; ++i){
			vtmp[i]=0;
		}*/
		for (auto it(l.begin()); it!=l.end(); ++it){
			//check if size of l contains at least two elements
			if(l.size()<2) break;

			// next point
			Coordinate np;
			auto nit(it);
			++nit;
			if(nit==l.end()) break;

			// center point relative
			///todo, cp and np and all other coordinates have same basepoint
			Coordinate cp;
			double volume;
			(*it)->getAbsolute(t,cp);
			(*it)->getChild("rootSegmentVolume")->get(t,volume);
			//next center point
			(*nit)->getAbsolute(t,np);
			//this saves time if not the whole rootsystem is rastered, but normally would just delay .
			if(cp.x<minCorner.x || cp.y<minCorner.y || cp.z<minCorner.z || cp.x>maxCorner.x || cp.y>maxCorner.y || cp.z>maxCorner.z){
				if(np.x<minCorner.x || np.y<minCorner.y || np.z<minCorner.z || np.x>maxCorner.x || np.y>maxCorner.y || np.z>maxCorner.z) {
					errorvol+=volume;
					continue;
				}
			}

			//np is relative to cp
			np-=cp;

			//distance between points
			//double cp_np_length(vectorlength(np));
			/*if(cp_np_length< 0.5/resolution){
				//remove nit form list
				--it;
				--nit;
				l.erase(nit);
				continue;
			}*/

			// diameter
			double d, nd, md;
			(*it)->getPath(radiusSimulatorName)->get(t,d);
			d/=factor;
			d*=dilation;
			(*nit)->getPath(radiusSimulatorName)->get(t,nd);
			nd/=factor;
			nd*=dilation;
			d > nd ? md=d : md=nd ;

			//cube within which the full root segment fits
			Coordinate corner1(cp), corner2(np+cp);
			//find minimum and maximum values
			Coordinate cornerMin(corner1), cornerMax(corner2);
			if(cornerMin.x > cornerMax.x ) {cornerMin.x=corner2.x; cornerMax.x=corner1.x; }
			if(cornerMin.y > cornerMax.y ) {cornerMin.y=corner2.y; cornerMax.y=corner1.y; }
			if(cornerMin.z > cornerMax.z)  {cornerMin.z=corner2.z; cornerMax.z=corner1.z; }
			//extend range with maximum diamter
			double ext = 2*md+smoothing;//ext is an extension of the maximum diameter (md)
			cornerMin-=Coordinate(ext,ext,ext);
			cornerMax+=Coordinate(ext,ext,ext);
			//snap to voxels
			cornerMin.x=floor(cornerMin.x*resolution)/resolution;
			cornerMin.y=floor(cornerMin.y*resolution)/resolution;
			cornerMin.z=floor(cornerMin.z*resolution)/resolution;
			cornerMax.x=ceil(cornerMax.x*resolution)/resolution;
			cornerMax.y=ceil(cornerMax.y*resolution)/resolution;
			cornerMax.z=ceil(cornerMax.z*resolution)/resolution;

			//loop trough pixels
			double step(1/resolution);
			double substep=step/substeps;
			double shift(0.5*step-0.5*substep);
			double voxvol(step*step*step);
			double rl;//relative length of projection point
			//double tot(0.0);
			for (double i(cornerMin.x); i<=cornerMax.x; i+=step ){
				for (double j(cornerMin.y); j<=cornerMax.y; j+=step ){
					for (double k(cornerMin.z); k<=cornerMax.z; k+=step ){
						Coordinate pm(i,j,k);
						double distm=distance(np,pm-cp,rl);
						if (distm>(md+step))
							continue;
						double tvalue=0;
						int count=0;
						for (double si(i-shift); si<=i+shift; si+=substep ){
							for (double sj(j-shift); sj<=j+shift; sj+=substep ){
								for (double sk(k-shift); sk<=k+shift; sk+=substep ){

									//mid position of the voxel
									Coordinate p(si,sj,sk);
									//if(p.x<minCorner.x || p.y<minCorner.y || p.z<minCorner.z || p.x>maxCorner.x || p.y>maxCorner.y || p.z>maxCorner.z) continue;
									p-=cp;
									//distance
									double dist=distance(np,p,rl);
									if(rl>1) {
										rl=1;
										dist=vectorlength(p,np);
									}
									if(rl<0) {
										rl=0;
										dist=vectorlength(p);
									}
									//interpolate root diameter
									double diam= d + rl * (nd-d);
									//compare
									double value;
									if(dist>diam+smoothing){
										value=0;//root is not inside voxel
									}else if(dist<=diam-smoothing){
										value=1;
									}else{
										value=0.5*(1+ (diam-dist)/smoothing);
									}

									//at any data to the circle
									//if(pointData) addData(t,c,(*it));

									tvalue+=value;
									++count;
								}
							}
						}
						//scale value back
						tvalue/=count;
						//tot+=tvalue;
						//store point
						pm-= minCorner;
						//pm-=Coordinate(1,1,1);
						pm*=resolution;
						int ix((int)(pm.x+0.49)),iy((int)(pm.y+0.49)),iz((int)(pm.z+0.49));
						if(ix>=0 && iy>=0 && iz>=0 && ix<dimx && iy<dimy && iz<dimz){
							int loc=(iz)*dimy*dimx + (iy)*dimx + ix;
							///todo we are assuming here that roots / root depletion zones never overlap with themselves, this to avoid that root segments overlap at the ends, creating 'intensity bubbles' around the nodes
							//if( tvalue > vtmp[loc] ) vtmp[loc] = tvalue;
							std::map<int,float>::iterator it(vtmp.insert(vtmp.end(),std::pair<int,float>(loc,(float)tvalue)));
							if(it->second < tvalue) it->second=(float)tvalue;
						}else{
							errorvol+=tvalue*voxvol;
						}
					}
				}
			}
		}
		//add k to v
		//for (unsigned int i=0; i<v.size() ; ++i){
		for(std::map<int,float>::iterator it(vtmp.begin()); it!=vtmp.end(); ++it){
			std::map<int,float>::iterator itb=v.lower_bound(it->first);
			if(itb!=v.end() && itb->first==it->first){
				itb->second+=it->second;
			}else{
			    v.insert(itb,*it);
			}

		}
	} else {
		//probe the attributes of this object
		SimulaBase::List allattributes;
			probe->getAllChildren(allattributes,t);
			for (auto & it:allattributes){
				rasterRoot(t, radiusSimulatorName, factor,  it, v, vtmp, errorvol);
			}
	}
}

/*void RawImage::addData(const Time &t, circle &c, SimulaBase* probe){
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
}*/



