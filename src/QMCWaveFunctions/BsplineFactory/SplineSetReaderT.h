//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2019 QMCPACK developers.
//
// File developed by: Jeremy McMinnis, jmcminis@gmail.com, University of Illinois at Urbana-Champaign
//                    Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//                    Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//                    Jeongnim Kim, jeongnim.kim@inte.com, Intel Corp.
//
// File created by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//////////////////////////////////////////////////////////////////////////////////////

#ifndef QMCPLUSPLUS_SPLINESET_READERT_H
#define QMCPLUSPLUS_SPLINESET_READERT_H
#include "BsplineFactory/BsplineReaderBaseT.h"
#include "Utilities/FairDivide.h"
#include "mpi/collectives.h"
#include "mpi/point2point.h"
#include "QMCWaveFunctions/BsplineFactory/einspline_helper.hpp"
#include "Utilities/ProgressReportEngine.h"
#include "QMCWaveFunctions/EinsplineSetBuilderT.h"
#include "fftw3.h"

namespace qmcplusplus
{
/** General SplineSetReader to handle any unitcell
 */
template<typename SA>
class SplineSetReaderT : public BsplineReaderBaseT<typename SA::ValueType>
{
public:
  using splineset_t = SA;
  using DataType    = typename splineset_t::DataType;
  using SplineType  = typename splineset_t::SplineType;
  using ValueType   = typename splineset_t::ValueType;

  Array<std::complex<double>, 3> FFTbox;
  Array<double, 3> splineData_r, splineData_i;
  double rotate_phase_r, rotate_phase_i;
  UBspline_3d_d* spline_r;
  UBspline_3d_d* spline_i;
  splineset_t* bspline;
  fftw_plan FFTplan;

  SplineSetReaderT(EinsplineSetBuilderT<ValueType>* e)
      : BsplineReaderBaseT<ValueType>(e), spline_r(nullptr), spline_i(nullptr), bspline(nullptr), FFTplan(nullptr)
  {}

  ~SplineSetReaderT() override { clear(); }

  void clear()
  {
    einspline::destroy(spline_r);
    einspline::destroy(spline_i);
    if (FFTplan != nullptr)
      fftw_destroy_plan(FFTplan);
    FFTplan = nullptr;
  }

  // set info for Hybrid
  virtual void initialize_hybridrep_atomic_centers() {}
  // transform cG to radial functions
  virtual void create_atomic_centers_Gspace(Vector<std::complex<double>>& cG, Communicate& band_group_comm, int iorb) {}

  std::unique_ptr<SPOSetT<ValueType>> create_spline_set(const std::string& my_name,
                                                        int spin,
                                                        const BandInfoGroup& bandgroup) override
  {
    ReportEngine PRE("SplineSetReader", "create_spline_set(spin,SPE*)");
    // Timer c_prep, c_unpack,c_fft, c_phase, c_spline, c_newphase, c_h5,
    // c_init; double t_prep=0.0, t_unpack=0.0, t_fft=0.0, t_phase=0.0,
    // t_spline=0.0, t_newphase=0.0, t_h5=0.0, t_init=0.0;
    bspline = new splineset_t(my_name);
    app_log() << "  ClassName = " << bspline->getClassName() << std::endl;
    if (bspline->isComplex())
      app_log() << "  Using complex einspline table" << std::endl;
    else
      app_log() << "  Using real einspline table" << std::endl;

    // set info for Hybrid
    this->initialize_hybridrep_atomic_centers();

    // baseclass handles twists
    this->check_twists(bspline, bandgroup);

    Ugrid xyz_grid[3];

    typename splineset_t::BCType xyz_bc[3];
    bool havePsig = this->set_grid(bspline->HalfG, xyz_grid, xyz_bc);
    if (!havePsig)
      this->myComm->barrier_and_abort("SplineSetReader needs psi_g. Set precision=\"double\".");
    bspline->create_spline(xyz_grid, xyz_bc);

    std::ostringstream oo;
    oo << bandgroup.myName << ".g" << this->MeshSize[0] << "x" << this->MeshSize[1] << "x" << this->MeshSize[2]
       << ".h5";

    const std::string splinefile(oo.str());
    bool root       = (this->myComm->rank() == 0);
    int foundspline = 0;
    Timer now;
    if (root)
    {
      now.restart();
      hdf_archive h5f(this->myComm);
      foundspline = h5f.open(splinefile, H5F_ACC_RDONLY);
      if (foundspline)
      {
        std::string aname("none");
        foundspline = h5f.readEntry(aname, "class_name");
        foundspline = (aname.find(bspline->getKeyword()) != std::string::npos);
      }
      if (foundspline)
      {
        int sizeD   = 0;
        foundspline = h5f.readEntry(sizeD, "sizeof");
        foundspline = (sizeD == sizeof(DataType));
      }
      if (foundspline)
      {
        foundspline = bspline->read_splines(h5f);
        if (foundspline)
          app_log() << "  Successfully restored coefficients from " << splinefile << ". The reading time is "
                    << now.elapsed() << " sec." << std::endl;
      }
      h5f.close();
    }
    this->myComm->bcast(foundspline);
    if (foundspline)
    {
      now.restart();
      bspline->bcast_tables(this->myComm);
      app_log() << "  SplineSetReader bcast the full table " << now.elapsed() << " sec." << std::endl;
      app_log().flush();
    }
    else
    {
      bspline->flush_zero();

      int nx = this->MeshSize[0];
      int ny = this->MeshSize[1];
      int nz = this->MeshSize[2];
      if (havePsig) // perform FFT using FFTW
      {
        FFTbox.resize(nx, ny, nz);
        FFTplan = fftw_plan_dft_3d(nx, ny, nz, reinterpret_cast<fftw_complex*>(FFTbox.data()),
                                   reinterpret_cast<fftw_complex*>(FFTbox.data()), +1, FFTW_ESTIMATE);
        splineData_r.resize(nx, ny, nz);
        if (bspline->isComplex())
          splineData_i.resize(nx, ny, nz);

        TinyVector<double, 3> start(0.0);
        TinyVector<double, 3> end(1.0);
        spline_r = einspline::create(spline_r, start, end, this->MeshSize, bspline->HalfG);
        if (bspline->isComplex())
          spline_i = einspline::create(spline_i, start, end, this->MeshSize, bspline->HalfG);

        now.restart();
        initialize_spline_pio_gather(spin, bandgroup);
        app_log() << "  SplineSetReader initialize_spline_pio " << now.elapsed() << " sec" << std::endl;

        fftw_destroy_plan(FFTplan);
        FFTplan = NULL;
      }
      else // why, don't know
        initialize_spline_psi_r(spin, bandgroup);
      if (this->saveSplineCoefs && root)
      {
        now.restart();
        hdf_archive h5f;
        h5f.create(splinefile);
        std::string classname = bspline->getClassName();
        h5f.write(classname, "class_name");
        int sizeD = sizeof(DataType);
        h5f.write(sizeD, "sizeof");
        bspline->write_splines(h5f);
        h5f.close();
        app_log() << "  Stored spline coefficients in " << splinefile << " for potential reuse. The writing time is "
                  << now.elapsed() << " sec." << std::endl;
      }
    }

    clear();
    return std::unique_ptr<SPOSetT<ValueType>>{bspline};
  }

  /** fft and spline cG
     * @param cG psi_g to be processed
     * @param ti twist index
     * @param iorb orbital index
     *
     * Perform FFT and spline to spline_r and spline_i
     */
  inline void fft_spline(Vector<std::complex<double>>& cG, int ti)
  {
    unpack4fftw(cG, this->mybuilder->Gvecs[0], this->MeshSize, FFTbox);
    fftw_execute(FFTplan);
    if (bspline->isComplex())
    {
      if (this->rotate)
        fix_phase_rotate_c2c(FFTbox, splineData_r, splineData_i, this->mybuilder->primcell_kpoints[ti], rotate_phase_r,
                             rotate_phase_i);
      else
      {
        split_real_components_c2c(FFTbox, splineData_r, splineData_i);
        rotate_phase_r = 1.0;
        rotate_phase_i = 0.0;
      }
      einspline::set(spline_r, splineData_r.data());
      einspline::set(spline_i, splineData_i.data());
    }
    else
    {
      fix_phase_rotate_c2r(FFTbox, splineData_r, this->mybuilder->primcell_kpoints[ti], rotate_phase_r, rotate_phase_i);
      einspline::set(spline_r, splineData_r.data());
    }
  }

  /** initialize the splines
     */
  void initialize_spline_pio_gather(int spin, const BandInfoGroup& bandgroup)
  {
    // distribute bands over processor groups
    int Nbands            = bandgroup.getNumDistinctOrbitals();
    const int Nprocs      = this->myComm->size();
    const int Nbandgroups = std::min(Nbands, Nprocs);
    Communicate band_group_comm(*this->myComm, Nbandgroups);
    std::vector<int> band_groups(Nbandgroups + 1, 0);
    FairDivideLow(Nbands, Nbandgroups, band_groups);
    int iorb_first = band_groups[band_group_comm.getGroupID()];
    int iorb_last  = band_groups[band_group_comm.getGroupID() + 1];

    app_log() << "Start transforming plane waves to 3D B-Splines." << std::endl;
    hdf_archive h5f(&band_group_comm, false);
    Vector<std::complex<double>> cG(this->mybuilder->Gvecs[0].size());
    const std::vector<BandInfo>& cur_bands = bandgroup.myBands;
    if (band_group_comm.isGroupLeader())
      h5f.open(this->mybuilder->H5FileName, H5F_ACC_RDONLY);
    for (int iorb = iorb_first; iorb < iorb_last; iorb++)
    {
      if (band_group_comm.isGroupLeader())
      {
        int iorb_h5   = bspline->BandIndexMap[iorb];
        int ti        = cur_bands[iorb_h5].TwistIndex;
        std::string s = this->psi_g_path(ti, spin, cur_bands[iorb_h5].BandIndex);
        if (!h5f.readEntry(cG, s))
        {
          std::ostringstream msg;
          msg << "SplineSetReader Failed to read band(s) from h5 "
                 "file. "
              << "Attempted dataset " << s << " with " << cG.size() << " complex numbers." << std::endl;
          throw std::runtime_error(msg.str());
        }
        double total_norm = compute_norm(cG);
        if ((this->checkNorm) && (std::abs(total_norm - 1.0) > PW_COEFF_NORM_TOLERANCE))
        {
          std::ostringstream msg;
          msg << "SplineSetReader The orbital " << iorb_h5 << " has a wrong norm " << total_norm
              << ", computed from plane wave coefficients!" << std::endl
              << "This may indicate a problem with the HDF5 library "
                 "versions used "
              << "during wavefunction conversion or read." << std::endl;
          throw std::runtime_error(msg.str());
        }
        fft_spline(cG, ti);
        bspline->set_spline(spline_r, spline_i, cur_bands[iorb_h5].TwistIndex, iorb, 0);
      }
      this->create_atomic_centers_Gspace(cG, band_group_comm, iorb);
    }

    this->myComm->barrier();
    Timer now;
    if (band_group_comm.isGroupLeader())
    {
      now.restart();
      bspline->gather_tables(band_group_comm.getGroupLeaderComm());
      app_log() << "  Time to gather the table = " << now.elapsed() << std::endl;
    }
    now.restart();
    bspline->bcast_tables(this->myComm);
    app_log() << "  Time to bcast the table = " << now.elapsed() << std::endl;
  }

  void initialize_spline_psi_r(int spin, const BandInfoGroup& bandgroup)
  {
    // old implementation buried in the history
    this->myComm->barrier_and_abort("SplineSetReaderP initialize_spline_psi_r "
                                    "implementation not finished.");
  }
};
} // namespace qmcplusplus
#endif