//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2019 QMCPACK developers.
//
// File developed by: Peter Doak, doakpw@ornl.gov, Oak Ridge National Laboratory
//
// File refactored from ParticleSet.h
//////////////////////////////////////////////////////////////////////////////////////

#ifndef QMCPLUSPLUS_MOVECONTEXT_H
#define QMCPLUSPLUS_MOVECONTEXT_H

#include <map>
#include <vector>
#include "OhmmsSoA/VectorSoaContainer.h"
#include "Configuration.h"
#include "Particle/Walker.h"
#include "QMCDrivers/Crowd.h"
#include "ParticleBase/RandomSeqGenerator.h"

namespace qmcplusplus
{
/** Thread local context for moving walkers
 *
 *  created once per driver per crowd
 *  It's two significant responsibilities are holding the thread local RandomGen_t
 *  And the particle group indexes.
 *
 *  
 */
template<typename T>
class ContextForSteps
{
public:
  ContextForSteps(RandomBase<T>& random_gen);
  RandomBase<T>& get_random_gen();
protected:
  RandomBase<T>& random_gen_;
};

extern template class ContextForSteps<QMCTraits::RealType>;

} // namespace qmcplusplus
#endif
