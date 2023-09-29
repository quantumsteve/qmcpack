//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by:   Raymond Clay III, rclay@sandia.gov, Sandia National Laboratories
//
// File created by:    Raymond Clay III, rclay@sandia.gov, Sandia National Laboratories
//////////////////////////////////////////////////////////////////////////////////////


/** @file EinsplineSpinorSetBuilderT.h
 * 
 * Derives EinsplineSetBuilder.  Overrides the createSPOSetFromXML method to read an up and down channel from hdf5
 *   and then construct an appropriate einspline spinor set object.
 *
 */
#ifndef QMCPLUSPLUS_EINSPLINE_SPINORSET_BUILDER_H
#define QMCPLUSPLUS_EINSPLINE_SPINORSET_BUILDER_H

#include "QMCWaveFunctions/SPOSetT.h"
#include "QMCWaveFunctions/SPOSetBuilderT.h"
#include "QMCWaveFunctions/EinsplineSetBuilderT.h"
class Communicate;

namespace qmcplusplus
{

template<typename T>
class EinsplineSpinorSetBuilderT : public EinsplineSetBuilderT<T>
{

  using ParticleSet = typename ParticleSetT<T>;
  using SPOSet       = typename SPOSetT<T>;
  using PSetMap = std::map<std::string, const std::unique_ptr<ParticleSet>>;

public:
  ///constructor
  EinsplineSpinorSetBuilderT(ParticleSet& p, const PSetMap& psets, Communicate* comm, xmlNodePtr cur)
      : EinsplineSetBuilder(p, psets, comm, cur){};

  ///destructor
  ~EinsplineSpinorSetBuilderT() override{};

  /** initialize the Antisymmetric wave function for electrons
   * @param cur the current xml node
   */
  std::unique_ptr<SPOSet> createSPOSetFromXML(xmlNodePtr cur) override;
};

} // namespace qmcplusplus


#endif
