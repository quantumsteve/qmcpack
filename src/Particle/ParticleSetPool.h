//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeremy McMinnis, jmcminis@gmail.com, University of Illinois at Urbana-Champaign
//                    Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//////////////////////////////////////////////////////////////////////////////////////


/**@file ParticleSetPool.h
 * @brief Declaration of ParticleSetPool
 */
#ifndef QMCPLUSPLUS_PARTICLESETPOOL_H
#define QMCPLUSPLUS_PARTICLESETPOOL_H

#include "Configuration.h"
#include "Particle/ParticleSetPoolT.h"

namespace qmcplusplus
{

using ParticleSetPool = ParticleSetPoolT<QMCTraits::ValueType>;

} // namespace qmcplusplus
#endif
