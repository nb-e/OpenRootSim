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


#include "Mesh.hpp"

#include "../../engine/Origin.hpp"

//static tIndex NumKD = 4;       // Maximum permitted number of available code number values



Mesh6::Mesh6(int factor): Mesh_base(factor) {

	subelem_list.resize(6*myNumEl);
	subelem_vol.resize(6*myNumEl);

	setSubelements();
	setElementVolumes();

	Bi.resize(6*4*myNumEl);
	Ci.resize(6*4*myNumEl);
	Di.resize(6*4*myNumEl);
	set_det_coefficients();
}


Mesh_base::Mesh_base(int factor) : elemList(0),   myListNE(0), /*subelements(nullptr),*/
		myNumNP(0), myNumEl(0), myIJ(0), myNumBP(0),
		nx(0), ny(0), nz(0), nex(0), ney(0), nez(0),
		mask(1), dx(0.), dy(0.),dz(0.),
		NumKD(4), vol(0), subelem_vol(0),
		subelem_list(0), x(0), y(0), z(0), Width(0.),rLen(0.),
		xmin(0.), xmax(0.), ymin(0.), ymax(0.), zmin(0.), zmax(0.) ,da(0.), VolumeOfDomain(0.),
		VolumeOfInputBox(0.)
{

	prepare(factor);

	/*** this is for SINK also ******************* !!
	 *
	 */
		/// TODO reset of dx dy dz for sink ???!!!
//		dx = (xmax - xmin) / (nx - 1);
//		dy = (ymax - ymin) / (ny - 1);
//		dz = (zmax - zmin) / (nz - 1);
		//max square distance minimum is 0.8 and maximum is 3.01
		da = 0.51 * (dx * dx + dy * dy + dz * dz); // 3.01
	//***************************************************************//

	elemList.resize(myNumEl);
	setElementList(); // neighbors in list

	setBoundaryNodes();
	
	myListNE.resize(myNumNP); // Number of subelements adjacent to a particular node.
	vol.resize(myNumNP); // weighted (partial element-)volume per node

	Width.resize(myNumNP);
	setWidth();

}


void Mesh_base::prepare(int factor){
	// get upper corner and lower corner - note that coordinate system y and z are swapped
	SimulaBase* p;
	p = ORIGIN->getPath("environment/dimensions/maxCorner");
	Coordinate X;
	p->get(X);
	xmax = X.x;
	ymax = X.z;
	zmax = X.y;

	p = ORIGIN->getPath("environment/dimensions/minCorner");
	p->get(X);
	xmin = X.x;
	ymin = X.z;
	zmin = X.y;

	// get resolution
	p = ORIGIN->existingPath(
			"environment/dimensions/resolution");
	if (p) {
		//p = ORIGIN->getPath("environment/dimensions/resolution");
		//Coordinate X;
		p->get(X);
		dx = X.x;
		dy = X.z;
		dz = X.y;

	} else {
		dx = 1.;
		dy = 1.;
		dz = 1.;
	}
	// for output (Balance):
	VolumeOfInputBox = (xmax-xmin)*(ymax-ymin)*(zmax-zmin);

	//increase the box a little // another version of doing this
//	--zmin;
//	--xmin;
//	++xmax;
//	--ymin;
//	++ymax;

    // increase the box a little so roots are not at box boundary nodes.
    zmin -= dz; // note that if zmax-zmin us bit a multiple of dz, the box will be less deep.
    xmin -= dx/2.;
    xmax += dx/2.;
    ymin -= dy/2.;
    ymax += dy/2.;

	/* old:
	 xmin = fmin(x[0], x[nx - 1]);		// x.min(); // valarray version
	 xmax = fmax(x[0], x[nx - 1]);		// x.max(); //
	 ymin = fmin(y[0], y[myIJ - 1]);	// y.min(); //
	 ymax = fmax(y[0], y[myIJ - 1]);	// y.max(); //
	 zmin = fmin(z[0], z[myNumNP - 1]);	// z.min(); //
	 zmax = fmax(z[0], z[myNumNP - 1]);	// z.max(); //
	 */
	// for output (Balance):
	VolumeOfDomain = (xmax-xmin)*(ymax-ymin)*(zmax-zmin);
	// old: std::fabs((x[myNumNP-1] - x[0]) * (y[myNumNP-1] - y[0]) * (z[myNumNP-1] - z[0]));


	// calculate nx,ny,nz
	nx =(tIndex) floor((xmax-xmin)/dx)+1;
	ny =(tIndex) floor((ymax-ymin)/dy)+1;
	nz =(tIndex) floor((zmax-zmin)/dz)+1;

	if(factor > 1.){
		nx *= factor;
		ny *= factor;
		nz *= factor;
		dx /= factor;
		dy /= factor;
		dz /= factor;
		--nx; --ny; --nz;
	}

	// integers for
	myIJ    = nx*ny; 				// number of nodes per layer
	myNumEl = (nz-1)*(nx-1)*(ny-1); // number of elements
	myNumBP = myIJ*2; 				// number of boundary nodes (top and bottom layer)
	myNumNP = nz * nx * ny; 		// number of nodes;


	// check dimensions
	//if(myNumNP > (sizeof(Conc)/sizeof(*Conc))){
	//  msg::error("Mesh: Array dimension exceeded maximum");
	//}

	x.resize(myNumNP); // 0  0  0 ...
	y.resize(myNumNP);
	z.resize(myNumNP);

	// TODO here we can change the axis IF we want to
	// setup x y z arrays
	tIndex n = 0;
	for (tIndex iz = 0; iz < nz; ++iz) {
		for (tIndex iy = 0; iy < ny; ++iy) {
			for (tIndex ix = 0; ix < nx; ++ix) {
				x[n] = (double(ix) * dx) + xmin;
				y[n] = (double(iy) * dy) + ymin;
				z[n] = zmax - (double(iz) * dz);
				++n;
			}
		}
	}

	mask.resize(myNumNP);

	// set shape
	p = ORIGIN->existingPath("environment/dimensions/roundPot");
	bool pot=false;
	if (p) p->get(pot);
	if (pot){
		// simulating round pots
		double xc = (xmax - xmin) * 0.5;
		double yc = (ymax - ymin) * 0.5;
		double r = std::min(yc,xc);
		r = r * r;

		for (tIndex n = 0; n < myNumNP; ++n) {
			if (x[n] * x[n] + y[n] * y[n] < r && z[n]>(zmin+dz) ) {
				mask[n] = 1;  //inside
			} else {
				mask[n] = 0; //outside
			}
		}

	} else {
		for (tIndex n = 0; n < myNumNP; ++n) {
			mask[n] = 1.;//no pot mask, set to 1 so diffusion coeff and Ksat are multiplied by one.
		}
	}

	return;
}

/*********************************************************************************/
void Mesh6::easy_init_matrix(SparseSymmetricMatrix &M)const {
	tIndex i(0),j(0),k(0),l(0);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 6; ++n) {
			calc_elem_corners(n,elemList[e], i, j, k, l);
			M.insert(i, i, 0.);
			M.insert(i, j, 0.);
			M.insert(i, k, 0.);
			M.insert(i, l, 0.);

			M.insert(j, j, 0.);
			M.insert(j, k, 0.);
			M.insert(j, l, 0.);

			M.insert(k, k, 0.);
			M.insert(k, l, 0.);

			M.insert(l, l, 0.);
		}
	}
	return;
}

/*********************************************************************************/
void Mesh6::easy_init_matrix(SparseMatrix &M)const {
	tIndex i(0),j(0),k(0),l(0);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 6; ++n) {
			calc_elem_corners(n,elemList[e], i, j, k, l);
			M.insert(i, i, 0.);
			M.insert(i, j, 0.);
			M.insert(i, k, 0.);
			M.insert(i, l, 0.);

			M.insert(j, i, 0.);
			M.insert(j, j, 0.);
			M.insert(j, k, 0.);
			M.insert(j, l, 0.);

			M.insert(k, i, 0.);
			M.insert(k, j, 0.);
			M.insert(k, k, 0.);
			M.insert(k, l, 0.);

			M.insert(l, i, 0.);
			M.insert(l, j, 0.);
			M.insert(l, k, 0.);
			M.insert(l, l, 0.);
		}
	}
	return;
}


/*********************************************************************************/
void Mesh5::easy_init_matrix(SparseSymmetricMatrix &M)const {
	tIndex i(0),j(0),k(0),l(0);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 5; ++n) {
			calc_elem_corners(n,elemList[e], i, j, k, l);
			M.insert(i, i, 0.);
			M.insert(i, j, 0.);
			M.insert(i, k, 0.);
			M.insert(i, l, 0.);

			M.insert(j, j, 0.);
			M.insert(j, k, 0.);
			M.insert(j, l, 0.);

			M.insert(k, k, 0.);
			M.insert(k, l, 0.);

			M.insert(l, l, 0.);
		}
	}
	return;
}

/*********************************************************************************/
void Mesh5::easy_init_matrix(SparseMatrix &M)const {
	tIndex i(0),j(0),k(0),l(0);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 5; ++n) {
			calc_elem_corners(n,elemList[e], i, j, k, l);
			M.insert(i, i, 0.);
			M.insert(i, j, 0.);
			M.insert(i, k, 0.);
			M.insert(i, l, 0.);

			M.insert(j, i, 0.);
			M.insert(j, j, 0.);
			M.insert(j, k, 0.);
			M.insert(j, l, 0.);

			M.insert(k, i, 0.);
			M.insert(k, j, 0.);
			M.insert(k, k, 0.);
			M.insert(k, l, 0.);

			M.insert(l, i, 0.);
			M.insert(l, j, 0.);
			M.insert(l, k, 0.);
			M.insert(l, l, 0.);
		}
	}
	return;
}



/*********************************************************************************/
void Mesh_base::setElementList() {
	/*****************************************************************************/
	sKX * elem;

	nex = nx - 1;
	ney = ny - 1;
	nez = nz - 1;
	tIndex iCord;

	tIndex e = 0;
	for (tIndex iz = 1; iz <= nez; ++iz) {
		for (tIndex iy = 1; iy <= ney; ++iy) {
			for (tIndex ix = 1; ix <= nex; ++ix) {
			// 8 node position that form the corner of the cube element

			// numbering of global nodes vs. local node numbers of cubes as index
			// local numbering can be changed here for a typical FE numbering counter-clockwise,
			// but that has to match the numbering of calc_elem because associated node number will change
			// right now its simply the global row-wise order.
			// also if red-black global numbering is considered it could be changed here
				elem = &(elemList[e]);
				elem->nodeIDs[0] = (ix) + nx * (iy-1) + nx * ny * (iz-1) -1;
				elem->nodeIDs[1] = elem->nodeIDs[0] + 1;
				elem->nodeIDs[2] = elem->nodeIDs[0] + nx;
				elem->nodeIDs[3] = elem->nodeIDs[0] + nx + 1;
				elem->nodeIDs[4] = elem->nodeIDs[0] + nx * ny;
				elem->nodeIDs[5] = elem->nodeIDs[0] + nx * ny + 1;
				elem->nodeIDs[6] = elem->nodeIDs[0] + nx * ny + nx;
				elem->nodeIDs[7] = elem->nodeIDs[0] + nx * ny + nx +1;
				// even or uneven element
				iCord = (ix) + (iy) + (iz);
				if (iCord % 2 != 0) {
					/* if uneven */
					elem->elemCode = 1;
				} else {
					/* if even */
					elem->elemCode = 2;
				}
				++e; // element counter
			}
		}
	}
	return;
}



/*********************************************************************************/
void Mesh6::setSubelements() {

	myListNE = 0; // this is double because needed later for division
	tIndex i(0), j(0), k(0), l(0);

	list_iter it_subelem_list = subelem_list.begin();
	tIndex volcounter(0);
	double Bl(0.), Cl(0.), Dl(0.), Det(0.);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 6; ++n) {
			calc_elem_corners(n, elemList[e], i, j, k, l);
			++myListNE[i];
			++myListNE[j];
			++myListNE[k];
			++myListNE[l];

//			subelements[counter].nodeIDs ={i,j,k,l};
			it_subelem_list->at(0) = i;
			it_subelem_list->at(1) = j;
			it_subelem_list->at(2) = k;
			it_subelem_list->at(3) = l;

			Bl = +(y[j] - y[i]) * (z[k] - z[i]) - (y[k] - y[i]) * (z[j] - z[i]);
			Cl = -(x[j] - x[i]) * (z[k] - z[i]) + (x[k] - x[i]) * (z[j] - z[i]);
			Dl = +(x[j] - x[i]) * (y[k] - y[i]) - (x[k] - x[i]) * (y[j] - y[i]);
			Det = (x[l] - x[i]) * Bl + (y[l] - y[i]) * Cl + (z[l] - z[i]) * Dl;

			subelem_vol[volcounter] = Det / 6.; // this is rve for a structured mesh

			if (Det <= 0.) {
				msg::error(
						"Mesh: Wrong orientation of nodes of element: "
								+ std::to_string(e) + ", subelement: "
								+ std::to_string(n)
								+ "! Determinant <= 0 !!!");
			}

//	   	 *it_subelem_list = {i,j,k,l};
			++it_subelem_list;
			++volcounter;
		}
	}

	return;
}

/*********************************************************************************/
void Mesh5::setSubelements() {

	myListNE = 0; // this is double because needed later for division

	tIndex i(0), j(0), k(0), l(0);

	list_iter it_subelem_list = subelem_list.begin();
	tIndex volcounter(0);
	double Bl(0.), Cl(0.), Dl(0.), Det(0.);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 5; ++n) {
			calc_elem_corners(n, elemList[e], i, j, k, l);
			++myListNE[i];
			++myListNE[j];
			++myListNE[k];
			++myListNE[l];

//			subelements[counter].nodeIDs ={i,j,k,l};
			it_subelem_list->at(0) = i;
			it_subelem_list->at(1) = j;
			it_subelem_list->at(2) = k;
			it_subelem_list->at(3) = l;

			Bl = +(y[j] - y[i]) * (z[k] - z[i]) - (y[k] - y[i]) * (z[j] - z[i]);
			Cl = -(x[j] - x[i]) * (z[k] - z[i]) + (x[k] - x[i]) * (z[j] - z[i]);
			Dl = +(x[j] - x[i]) * (y[k] - y[i]) - (x[k] - x[i]) * (y[j] - y[i]);
			Det = (x[l] - x[i]) * Bl + (y[l] - y[i]) * Cl + (z[l] - z[i]) * Dl;

			subelem_vol[volcounter] = Det / 6.;

			if (Det <= 0.) {
				msg::error(
						"Mesh6: Wrong orientation of nodes of element: "
								+ std::to_string(e) + ", subelement: "
								+ std::to_string(n)
								+ "! Determinant <= 0 !!!");
			}

//	   	 *it_subelem_list = {i,j,k,l};
			++it_subelem_list;
			++volcounter;
		}
	}
	return;
}


/*********************************************************************************/
void Mesh_base::setElementVolumes() {
	//these are simply the volumes as they are used in the spatial integration. Weighing nodes and count them as done elsewhere.
	tIndex it_subelem = 0;
	for (list_const_iter it = subelem_list.begin(); it != subelem_list.end(); ++it) { // Loop over sub-elements
		const tIndex i = (*it)[0], j = (*it)[1], k = (*it)[2], l = (*it)[3];
		const double VE = subelem_vol[it_subelem];
		// volumeTetraHedron(myNumNP,i,j,k,l,x,y,z), this is for a regular grid simply 1/6 or for the outer tetrahedra and 2/6 for the inner in Mesh5
		//sum of globale_weights should be 1.

			vol[i] += VE*0.25;
			vol[j] += VE*0.25;
			vol[k] += VE*0.25;
			vol[l] += VE*0.25;

		++it_subelem;
	}
	return;
}



/*********************************************************************************/
void Mesh_base::setBoundaryNodes() {
	//todo these codes should go in an enum so that the code becomes more readable.
	//todo not so sure if these system specific things should be part of the mesh?
	tIndex count(0);
	topBoundaryNodes_.resize(myIJ);
	for (tIndex i = 0; i < myIJ; ++i) {
		topBoundaryNodes_[count]=(i);
		++count;
	}
	count=0;
	bottomBoundaryNodes_.resize(myIJ);
	for (tIndex i = myNumNP - myIJ; i < myNumNP; ++i) {
		bottomBoundaryNodes_[count]=(i);
		++count;
	}
	return;
}


void Mesh_base::setWidth() {

	// width (name may be legacy from 2d version?)
	// this surface area of 4 cubes/3 or 6 depending if it is an even or uneven node
	// this is used for surface area pressure and free drainage calculations
	// TODO I (jouke) wonder if this code is doing what is meant to do.
	// according to manual this is the surface area that is associated with a node

	//our mesh is regular, all nodes have the same value.
	double w=dx*dy;
	for (auto i : topBoundaryNodes_) {
		Width[i]=w;
	}
	for (auto i : bottomBoundaryNodes_) {
		Width[i]=w;
	}
	///todo correct for edges and corners
	//we had corners time 0.25 and edges time 0.5, but motivation is vague



	// surface area of the soil surface to calculate the transpiration/surface area
	rLen = std::fabs((x[nx - 1] - x[0]) * (y[(ny) * (nx) - 1] - y[0]));

	return;
}


/*********************************************************************************/
void Mesh5::calc_elem_corners(const tIndex &n,
			const sKX & elem, /* global nodal numbers */
			tIndex &i,
			tIndex &j,
			tIndex &k,
			tIndex &l )const{
	/*****************************************************************************/
	if (1 == elem.elemCode ) {
		// uneven node orientation one way
		switch (n) {
		case 0:
			i = elem.nodeIDs[0];
			j = elem.nodeIDs[3];
			k = elem.nodeIDs[6];
			l = elem.nodeIDs[2]; // rectangular corner node
			break;
		case 1:
			i = elem.nodeIDs[0];
			j = elem.nodeIDs[5];
			k = elem.nodeIDs[3];
			l = elem.nodeIDs[1]; // rectangular corner node
			break;
		case 2:
			i = elem.nodeIDs[4]; // rectangular corner node
			j = elem.nodeIDs[5];
			k = elem.nodeIDs[6];
			l = elem.nodeIDs[0];
			break;
		case 3:
			i = elem.nodeIDs[6];
			j = elem.nodeIDs[5];
			k = elem.nodeIDs[7]; //rectangular corner node
			l = elem.nodeIDs[3];
			break;
		default: // middle tetrahedra with larger volume
			i = elem.nodeIDs[0];
			j = elem.nodeIDs[5];
			k = elem.nodeIDs[6];
			l = elem.nodeIDs[3];
			break;
		}
	} else if (elem.elemCode == 2){ // KX
		//even node: orientation the other way
		switch (n) {
		case 0:
			i = elem.nodeIDs[4];
			j = elem.nodeIDs[1];
			k = elem.nodeIDs[2];
			l = elem.nodeIDs[0]; // rectangular corner node
			break;
		case 1:
			i = elem.nodeIDs[4];
			j = elem.nodeIDs[5]; // rectangular corner node
			k = elem.nodeIDs[7];
			l = elem.nodeIDs[1];
			break;
		case 2:
			i = elem.nodeIDs[4];
			j = elem.nodeIDs[7];
			k = elem.nodeIDs[6]; // rectangular corner node
			l = elem.nodeIDs[2];
			break;
		case 3:
			// middle tetrahedra with larger volume
			i = elem.nodeIDs[4];
			j = elem.nodeIDs[1];
			k = elem.nodeIDs[7];
			l = elem.nodeIDs[2];
			break;
		case 4:
			i = elem.nodeIDs[1];
			j = elem.nodeIDs[7];
			k = elem.nodeIDs[2];
			l = elem.nodeIDs[3]; // rectangular corner node
			break;
		default:
			msg::error("Mesh: Element number not known.");
			break;
		}
	} else if(elem.elemCode == -1){
		msg::error("Mesh: elemCode not set!");
	}
	return;
}


// determinant is used for scaling and calculation of voxel volumes
// since we assume a regular rectangular grid, oriented along the x,y,z, axis, we can simplify this a lot
void Mesh5::calc_det(const tIndex &n,
		const sKX & elem, /* global nodal numbers */
		tVector &Bi,
		tVector &Ci,
		tVector &Di) {

	        if(1 == elem.elemCode ) {
	        // uneven node orientation one way
	        switch (n) {
	        case(0):
	        	 Bi={0.,1.,0.,-1.};
	        	 Ci={-1.,0.,0.,1.};
	        	 Di={0.,0.,-1.,1.};
	        	 break;
	        case(1):
	            Bi={-1.,0.,0.,1.};
	            Ci={0.,0.,1.,-1.};
	            Di={0.,-1.,0.,1.};
	          break;
	        case(2):
	            Bi={-1.,1.,0.,0.};
	            Ci={-1.,0.,1.,0.};
	            Di={-1.,0.,0.,1.};
	          break;
	        case(3):
	            Bi={-1.,0.,1.,0.};
	            Ci={0.,-1.,1.,0.};
	            Di={0.,0.,-1.,1.};
	          break;
	        default: //(n == 4)
	            Bi={-1.,1.,-1.,1.};
	            Ci={-1.,-1.,1.,1.};
	            Di={1.,-1.,-1.,1.};
	          break;
	        }
	        }
	        else{
	        // even node: orientation the other way
	        switch (n) {
	         case(0):
	            Bi={0.,1.,0.,-1.};
	            Ci={0.,0.,1.,-1.};
	            Di={-1.,0.,0.,1.};
	          break;
	         case(1):
	            Bi={-1.,1.,0.,0.};
	            Ci={0.,-1.,1.,0.};
	            Di={0.,-1.,0.,1.};
	          break;
	         case(2):
	            Bi={0.,1.,-1.,0.};
	            Ci={-1.,0.,1.,0.};
	            Di={0.,0.,-1.,1.};
	          break;
	         case(3):
	            Bi={-1.,1.,1.,-1.};
	            Ci={-1.,-1.,1.,1.};
	            Di={-1.,1.,-1.,1.};
	           break;
	         default:
	            Bi={0.,0.,-1.,1.};
	            Ci={-1.,0.,0.,1.};
	            Di={0.,-1.,0.,1.};
	          break;
	        }
	       }

	       // multiply array with dimensions
	        Bi *= (dy)*(dz);
	        Ci *= (dx)*(dz);
	        Di *= (dx)*(dy);

	return;
}
void Mesh6::set_det_coefficients(){
	tIndex counter(0);
	for (tIndex e = 0; e < myNumEl; ++e) {
		// Loop on sub-elements
		for (tIndex n = 0; n < 6; ++n) {
			tIndex i(0), j(0), k(0), l(0);
			calc_elem_corners(n, elemList[e], i, j, k, l);

	        const double Bl0=-(y[k]-y[j])*(z[l]-z[j])+(y[l]-y[j])*(z[k]-z[j]);
	        const double Bl1=+(y[l]-y[k])*(z[i]-z[k])-(y[i]-y[k])*(z[l]-z[k]);
	        const double Bl2=-(y[i]-y[l])*(z[j]-z[l])+(y[j]-y[l])*(z[i]-z[l]);
	        const double Bl3=+(y[j]-y[i])*(z[k]-z[i])-(y[k]-y[i])*(z[j]-z[i]);

	        const double Cl0=+(x[k]-x[j])*(z[l]-z[j])-(x[l]-x[j])*(z[k]-z[j]);
	        const double Cl1=-(x[l]-x[k])*(z[i]-z[k])+(x[i]-x[k])*(z[l]-z[k]);
	        const double Cl2=+(x[i]-x[l])*(z[j]-z[l])-(x[j]-x[l])*(z[i]-z[l]);
	        const double Cl3=-(x[j]-x[i])*(z[k]-z[i])+(x[k]-x[i])*(z[j]-z[i]);

	        const double Dl0=-(x[k]-x[j])*(y[l]-y[j])+(x[l]-x[j])*(y[k]-y[j]);
	        const double Dl1=+(x[l]-x[k])*(y[i]-y[k])-(x[i]-x[k])*(y[l]-y[k]);
	        const double Dl2=-(x[i]-x[l])*(y[j]-y[l])+(x[j]-x[l])*(y[i]-y[l]);
	        const double Dl3=+(x[j]-x[i])*(y[k]-y[i])-(x[k]-x[i])*(y[j]-y[i]);

			Bi[counter] = Bl0;
			Ci[counter] = Cl0;
			Di[counter] = Dl0;

			Bi[counter + 1] = Bl1;
			Ci[counter + 1] = Cl1;
			Di[counter + 1] = Dl1;

			Bi[counter + 2] = Bl2;
			Ci[counter + 2] = Cl2;
			Di[counter + 2] = Dl2;

			Bi[counter + 3] = Bl3;
			Ci[counter + 3] = Cl3;
			Di[counter + 3] = Dl3;
			counter += 4;
		}
	}
	return;
}

void Mesh5::set_det_coefficients(){  // TODO re-implement ??
	tVector Bl(0.,4);
	tVector Cl(0.,4);
	tVector Dl(0.,4);
	tIndex counter = 0;
	for (tIndex e = 0; e < myNumEl; ++e) {
			// Loop on sub-elements
			for (tIndex n = 0; n < 5; ++n) {
				calc_det(n, elemList[e], Bl, Cl, Dl);
					Bi[counter] = Bl[0];
					Ci[counter] = Cl[0];
					Di[counter] = Dl[0];

					Bi[counter+1] = Bl[1];
					Ci[counter+1] = Cl[1];
					Di[counter+1] = Dl[1];

					Bi[counter+2] = Bl[2];
					Ci[counter+2] = Cl[2];
					Di[counter+2] = Dl[2];

					Bi[counter+3] = Bl[3];
					Ci[counter+3] = Cl[3];
					Di[counter+3] = Dl[3];
					counter += 4;
			}
	}
	return;
}


void Mesh_base::getIndex(const Coordinate & posC, tIndex & im) const{
	const double xr=posC.x;
	const double yr=/*zmax-*/posC.z; // why was this different from the Sink::matchNodes version for simunek
	const double zr=posC.y;
	//the order is z, y, x, note that higher numbers are lower in z (z is inverted)
	int izr = (int)floor(0.5 + ((zmax - zr) / dz));
	int iyr = (int)floor(0.5 + ((yr - ymin) / dy));
	int ixr = (int)floor(0.5 + ((xr - xmin) / dx));
	for (int iz = izr - 1; iz < izr + 2; ++iz) {
		for (int iy = iyr - 1; iy < iyr + 2; ++iy) {
			for (int ix = ixr - 1; ix < ixr + 2; ++ix) {
				im = (iz * (int)myIJ) + (iy * (int)nx) + ix;
			}
		}
	}
	return;
}

double Mesh_base::integrateOverMesh(const tVector & data)const{

	//todo this loop can be flattened, as the weights for each element or known.
	double r(0);
	for (auto& it : subelem_list) { // Loop over sub-elements
		//index values of the nodes of this tetrahedal ellement
		const tIndex i(it[0]), j(it[1]), k(it[2]), l(it[3]);
		const double VE = getSubelemetVolumes()[&it - &subelem_list[0]];//&it - &subelem_list[0] calcs the index or the iterator

		r += VE / 4. * (data[i] + data[j] + data[k] + data[l]);
	}
	return (r);

}


void Mesh_base::matchNodes(const Coordinate & posC, std::vector<int> & indexes, std::vector<double> & nodedis, double &sumdis) const{

	const double xr=posC.x;
	const double yr=/*zmax-*/posC.z; // why was this different from the Sink::matchNodes version for simunek
	const double zr=posC.y;
	indexes.clear();
	nodedis.clear();
	sumdis=0;


	//!check if the root is within the fem grid, otherwise ignore it
	if (xr > xmax || xr < xmin || yr > ymax || yr < ymin || zr > zmax || zr < zmin) {
		if(zr < zmax) msg::warning("Mesh::matchNodes: Ignoring root node outside FEM grid below the surface");
		indexes.push_back(-1);
		return;
	}

	//the order is z, y, x, note that higher numbers are lower in z (z is inverted)
	int izr = (int)floor(0.5 + ((zmax - zr) / dz));
	int iyr = (int)floor(0.5 + ((yr - ymin) / dy));
	int ixr = (int)floor(0.5 + ((xr - xmin) / dx));

	for (int iz = izr - 1; iz < izr + 2; ++iz) {
		for (int iy = iyr - 1; iy < iyr + 2; ++iy) {
			for (int ix = ixr - 1; ix < ixr + 2; ++ix) {
				int im = (iz * (int)myIJ) + (iy * (int)nx) + ix;
				if (im >= 0 && im < (int)myNumNP) {
					double sdx = xr - x[im];
					double sdy = yr - y[im];
					double sdz = zr - z[im];
					//std::cout<<std::endl<<"point"<<xr<<" "<<yr<<" "<<zr;
					//std::cout<<std::endl<<"coord"<<x[im]<<" "<<y[im]<<" "<<z[im];
					double sqd = sdx * sdx + sdy * sdy + sdz * sdz;
					if (sqd < da) {
						//!we found a neighbouring node
						//!number of the neighbouring nodes in the fem grid
						indexes.push_back(im);
						//!number of nodes increase with distance qubic
						sqd = std::pow(sqd, 1.5);
						//!invert distance of the neigbouring node to the root node
						if (sqd < 1e-8)
							sqd = 1e-8;
						nodedis.push_back(1. / sqd);
						//!sum of the invert distances
						sumdis += 1./sqd;
					}
				}

			}
		}
	}
	/*
	 int im=(izr*myIJ)+(iyr*nx)+ixr;
	 double sdx=xr-x[im];
	 double sdy=yr-y[im];
	 double sdz=zr-z[im];
	 std::cout<<std::endl<<"point"<<xr<<" "<<yr<<" "<<zr;
	 std::cout<<std::endl<<"coord"<<x[im]<<" "<<y[im]<<" "<<z[im];
	 double sqd=sdx*sdx+sdy*sdy+sdz*sdz;
	 //!number of the neighbouring nodes in the fem grid
	 neighbs[nrneigh]=im+1;
	 //!number of nodes increase with distance qubic
	 sqd= std::pow(sqd,1.5);
	 //!invert distance of the neigbouring node to the root node
	 if(sqd<1e-8) sqd=1e-8;
	 nodedis[nrneigh]=1/sqd;
	 //!sum of the invert distances
	 sumdis=sumdis+nodedis[nrneigh];
	 //!counting the neighbouring fem nodes
	 ++nrneigh;
	 */

	if (indexes.empty()) {
		indexes.push_back(-1);
		msg::warning("Mesh::matchingRootNodes: no matchin FEM nodes found. This is probably a programming error. ignoring this root node.");
	}
	if (sumdis <= 0 && nodedis.size() > 1)
		msg::error("Mesh::matchingRootNodes: programming error: sumdis<=0 while number of nodes is larger than 1");

	return;
}



/*********************************************************************************/
void Mesh6::calc_elem_corners(const tIndex &n,
			const sKX & elem, /* global nodal numbers */
			tIndex &i,
			tIndex &j,
			tIndex &k,
			tIndex &l)const{
	/*****************************************************************************/
	int meshdirection = 1;
	if ( meshdirection == 1 ) {
		switch (n) {
		case 0:
			i = elem.nodeIDs[0];
			j = elem.nodeIDs[6];
			k = elem.nodeIDs[5];
			l = elem.nodeIDs[4];
			break;
		case 1:
			i = elem.nodeIDs[1];
			j = elem.nodeIDs[5];
			k = elem.nodeIDs[0];
			l = elem.nodeIDs[6];
			break;
		case 2:
			i = elem.nodeIDs[6];
			j = elem.nodeIDs[0];
			k = elem.nodeIDs[1];
			l = elem.nodeIDs[2];
			break;
		case 3:
			i = elem.nodeIDs[6];
			j = elem.nodeIDs[7];
			k = elem.nodeIDs[2];
			l = elem.nodeIDs[1];
			break;
		case 4:
			i = elem.nodeIDs[7];
			j = elem.nodeIDs[2];
			k = elem.nodeIDs[1];
			l = elem.nodeIDs[3];
			break;
		case 5:
			i = elem.nodeIDs[6];
			j = elem.nodeIDs[5];
			k = elem.nodeIDs[7];
			l = elem.nodeIDs[1];
			break;
		default:
			msg::error("Mesh: Element number not known.");
			break;
		}
	} else if (meshdirection == 2){
		switch (n) {
		case 0:
			i = elem.nodeIDs[4];
			j = elem.nodeIDs[0];
			k = elem.nodeIDs[7];
			l = elem.nodeIDs[6];
			break;
		case 1:
			i = elem.nodeIDs[7];
			j = elem.nodeIDs[2];
			k = elem.nodeIDs[0];
			l = elem.nodeIDs[3];
			break;
		case 2:
			i = elem.nodeIDs[6];
			j = elem.nodeIDs[0];
			k = elem.nodeIDs[7];
			l = elem.nodeIDs[2];
			break;
		case 3:
			i = elem.nodeIDs[1];
			j = elem.nodeIDs[0];
			k = elem.nodeIDs[3];
			l = elem.nodeIDs[7];
			break;
		case 4:
			i = elem.nodeIDs[0];
			j = elem.nodeIDs[1];
			k = elem.nodeIDs[5];
			l = elem.nodeIDs[7];
			break;
		case 5:
			i = elem.nodeIDs[4];
			j = elem.nodeIDs[0];
			k = elem.nodeIDs[5];
			l = elem.nodeIDs[7];
			break;
		default:
			msg::error("Mesh: Element number not known.");
			break;
		}
	}
	return;
}
