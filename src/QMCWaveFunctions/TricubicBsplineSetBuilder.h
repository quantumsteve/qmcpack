//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_TRICUBIC_BSPLINESETBUILDER_H
#define QMCPLUSPLUS_TRICUBIC_BSPLINESETBUILDER_H

#include "QMCWaveFunctions/BasisSetBase.h"
#include "QMCWaveFunctions/Bspline3DSetBase.h"
#include "Numerics/HDFNumericAttrib.h"

namespace qmcplusplus {

  class PWParameterSet;

  /**@ingroup WFSBuilder
   * A builder class for a set of Spline functions
   */
  class TricubicBsplineSetBuilder: public BasisSetBuilder {

  public:

    typedef Bspline3DSetBase              BsplineBasisType;
    typedef Bspline3DSetBase::StorageType StorageType;
    typedef map<string,ParticleSet*>      PtclPoolType;

    /** constructor
     * @param p target ParticleSet
     * @param psets a set of ParticleSet objects
     */
    TricubicBsplineSetBuilder(ParticleSet& p, PtclPoolType& psets, xmlNodePtr cur);

    ///destructor
    ~TricubicBsplineSetBuilder();

    ///implement put function to process an xml node
    bool put(xmlNodePtr cur);

    /** initialize the Antisymmetric wave function for electrons
     *@param cur the current xml node
     */
    SPOSetBase* createSPOSet(xmlNodePtr cur);

  private:
    /** set of StorageType*
     *
     * Key is $1#$2 where $1 is the hdf5 file name and $2 is the band indenx
     */
    static map<string,StorageType*> BigDataSet;
    ///boolean to enable debug with EG
    bool DebugWithEG;
    ///if true, grid is open-ended [0,nx) x [0,ny) x [0, nz)
    bool OpenEndGrid;
    ///if true, write stuff
    bool print_log;
    ///twist angle
    PosType TwistAngle;
    ///target ParticleSet
    ParticleSet& targetPtcl;
    ///reference to a ParticleSetPool
    PtclPoolType& ptclPool;
    ///current hdf5 file name
    std::string curH5Fname;
    ///save xml node
    xmlNodePtr rootNode;
    PosType LowerBox;
    PosType UpperBox;
    TinyVector<IndexType,DIM> BoxGrid;
    ///parameter set for h5 tags
    PWParameterSet* myParam;
    ///hdf5 handler to clean up
    hid_t hfileID;
    ///pointer to the BsplineBasisType use
    BsplineBasisType* activeBasis;
    ///set of WFSetType*
    map<string,BsplineBasisType*> myBasis;
    ///single-particle orbital sets
    map<string,SPOSetBase*> mySPOSet;

    ///set Bspline functions
    void setBsplineBasisSet(xmlNodePtr cur);

    ///read numerical orbitals and spline them
    void readData(const char* hroot, const vector<int>& occSet, 
        int spinIndex, int degeneracy);

    /** read numerical orbitals and spline them
     * 
     * With QMC_COPMLEX=0, convert the complex data in hdf5 to real.
     */
    void readComplex2RealData(const char* hroot, const vector<int>& occSet,
        int spinIndex, int degeneracy);

    ///a function to test with EG
    //SPOSetBase* createSPOSetWithEG();
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
