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


#ifndef MESH_HPP__
#define MESH_HPP__

#include "../../math/SparseMatrix.hpp"
#include "../../math/SparseSymmetricMatrix.hpp"
#include "../SimRootTypes.hpp"

#include <list>
#include <array>
#include "../../engine/DataDefinitions/Coordinates.hpp"


typedef std::vector< std::array<tIndex,4> >::iterator list_iter;
typedef std::vector< std::array<tIndex,4> >::const_iterator list_const_iter;
typedef std::list<tIndex> node_iter;


class Mesh_base {

public:

	struct sKX {  // TODO elements do not have to be in a struct. Is that inefficient coding?
		std::array<tIndex,8> nodeIDs;
		int elemCode;
		sKX(): nodeIDs(),elemCode(-1) {}
		sKX(const sKX & other) : nodeIDs(other.nodeIDs), elemCode(other.elemCode) {
		}
	};

	/// constructor
	Mesh_base(const int factor = 1);
	/// destructor
	virtual ~Mesh_base(){
	}

	const std::vector<sKX> 	  & getElements() const { return elemList; }
    const tVector             & getListNE()   const { return myListNE; }

    const std::valarray<int> & getpotmask()const{ return mask; }

    const tIndex & getNumNP() const { return myNumNP; } // when returning by value we do not need const
    const tIndex & getNumEl() const { return myNumEl; }
    const tIndex & getIJ()    const { return myIJ;    }
    const tIndex & getNumBP() const { return myNumBP; }
    const tIndex & getNumKD() const { return NumKD;   }

    const tIndex & getnx() const { return nx; }
    const tIndex & getny() const { return ny; }
    const tIndex & getnz() const { return nz; }

    const tVector & getCordX()  const { return x; }
    const tVector & getCordY()  const { return y; }
    const tVector & getCordZ()  const { return z; }
    const tFloat  & getrLen()   const { return rLen;   }
    const tVector & getWidth()  const { return Width;  }
    const tVector & getvol()    const { return vol; } /// volume per node

    const std::vector< std::array<tIndex,4> > & getSubelementlist()const{ return subelem_list;}
    const tVector & getSubelemetVolumes()const{ return subelem_vol;}
    double integrateOverMesh(const tVector & data)const;

    void getIndex(const Coordinate & posC, tIndex & im) const;
    void matchNodes(const Coordinate & pos, std::vector<int> & indexes, std::vector<double> & nodedis, double &sumdis)const;

    /// Determinant coefficients
    const tVector & getDi()const{ return Di;}
    const tVector & getCi()const{ return Ci;}
    const tVector & getBi()const{ return Bi;}
    const std::valarray<tIndex> & getBottomBoundaryNodes()const{ return bottomBoundaryNodes_;}
    const std::valarray<tIndex> & getTopBoundaryNodes()const{ return topBoundaryNodes_;}

    const double & getVolumeOfDomain() const { return VolumeOfDomain;}
    const double & getVolumeOfInputBox() const { return VolumeOfInputBox;}

    virtual void easy_init_matrix(SparseSymmetricMatrix &M)const = 0;
    virtual void easy_init_matrix(SparseMatrix &M)const = 0;
protected:

    void prepare(const int factor);
	void setElementList();

	virtual void setElementVolumes();
	void setBoundaryNodes();
	void setWidth();

	std::vector<sKX> elemList;
	tVector myListNE; /// Number of subelements adjacent to a particular node.
	tIndex myNumNP;
	tIndex myNumEl;
	tIndex myIJ;
	tIndex myNumBP;

	tIndex nx, ny, nz, nex, ney, nez;

	std::valarray<int> mask;
	double dx,dy,dz;
	//todo this should replace the fortran boundary interface
	std::valarray<tIndex> bottomBoundaryNodes_, topBoundaryNodes_; //alternative boundary interface

	tIndex NumKD; /// loop over NumKD from 1 to NumKD+1 because these are identifiers.

	tVector vol;
	tVector subelem_vol;
	std::vector< std::array<tIndex,4> > subelem_list;

	tVector x;
	tVector y;
	tVector z;

	tVector Width;
	tFloat rLen;

    double xmin,xmax,ymin,ymax,zmin,zmax ,da;
    double VolumeOfDomain,VolumeOfInputBox;
    tVector Bi, Ci, Di;
};

class Mesh6: public virtual Mesh_base {
public:
	/// constructor
	Mesh6(const int factor= 1);
	/// destructor
	virtual ~Mesh6(){
	}

	virtual void easy_init_matrix(SparseSymmetricMatrix &M)const;
	virtual void easy_init_matrix(SparseMatrix &M)const;

	tIndex getNsubel() const{
		const tIndex N = 6;
		return N;}

protected:
	virtual void calc_elem_corners(const tIndex &n, /* subelement */
				const sKX & elem, /* elementList */
				tIndex &i,  /* global nodal numbers */
				tIndex &j,
				tIndex &k,
				tIndex &l )const;

	virtual void setSubelements();
	virtual void set_det_coefficients();

};


class Mesh5: public virtual Mesh_base {
public:
	/// constructor
	Mesh5(const int factor= 1);
	/// destructor
	virtual ~Mesh5(){
	}

	virtual void easy_init_matrix(SparseSymmetricMatrix &M)const;
	virtual void easy_init_matrix(SparseMatrix &M)const;

    tIndex getNsubel()const{
    	const tIndex N = 5;
    	return N;}
protected:
	/**
	 * NUS, number of subelements, not used anymore, for simroot assumes cubes divided in 5 tetrahedra \n
	 * NumElD, number of elements for the dimensions of KX \n
	 * e, the e'th element \n
	 * n, the n'th subelement n=1-nus \n
	 * KX, the node numbers that belong to an element \n
	 * i,j,k,l, return values, will indicate the corners of subelement n of element e \n
	 * order of the elements is such that the determinant is correct
	 */
	virtual void calc_elem_corners(const tIndex &n, /* subelement */
				const sKX & elem, /* elementList */
				tIndex &i,  /* global nodal numbers */
				tIndex &j,
				tIndex &k,
				tIndex &l )const;
	/** determinant is used for scaling and calculation of voxel volumes \n
	* since we assume a regular rectangular grid, oriented along the x,y,z, axis, we can simplify this a lot \n
	*/
    void calc_det(const tIndex &n,const sKX & elem,
			tVector &Bi,
			tVector &Ci,
			tVector &Di);
	virtual void setSubelements();
	virtual void set_det_coefficients();

};

#endif
