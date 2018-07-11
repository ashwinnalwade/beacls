/*
 * Copyright (c) 2017, The Regents of the University of California (Regents).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *    3. Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Please contact the author(s) of this library if you have any questions.
 * Authors: Jaime F. Fisac   ( jfisac@eecs.berkeley.edu )
 */

///////////////////////////////////////////////////////////////////////////////
//
// DynSys subclass: relative dynamics between a bicycle model car and a
//                  dynamically extended (+acceleration) simple car
//
///////////////////////////////////////////////////////////////////////////////


#include <helperOC/DynSys/BicycleCAvoid/BicycleCAvoid.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <typeinfo>
#include <levelset/Grids/HJI_Grid.hpp>
using namespace helperOC;


BicycleCAvoid::BicycleCAvoid(
    const beacls::FloatVec& x,      // state of tracker (relative to planner)
    const beacls::IntegerVec& dims
) : DynSys(
      7, // states: [x_rel, y_rel, psi_rel, Ux, Uy, V, r]
      3, // controls: [d, Fxf, Fxr]
      2  // disturbances: dynamically extended simple car [w, a]
    ), dims(dims) {
    std::cout << "Constructed BicycleCAvoid object." << std::endl;
    //!< Process control range
    if (x.size() != DynSys::get_nx()) {
      std::cerr << "Error: " << __func__ << " : Initial state does not have right dimension!" << std::endl;
    }
    //!< Process initial state
    DynSys::set_x(x);
    DynSys::push_back_xhist(x);
}

BicycleCAvoid::BicycleCAvoid(
    beacls::MatFStream* fs,
    beacls::MatVariable* variable_ptr
) :
    DynSys(fs, variable_ptr),
    dims(beacls::IntegerVec()) {
    beacls::IntegerVec dummy;
    load_vector(dims, std::string("dims"), dummy, true, fs, variable_ptr);
}

BicycleCAvoid::~BicycleCAvoid() {}

bool BicycleCAvoid::operator==(const BicycleCAvoid& rhs) const {
  if (this == &rhs) return true;
  else if (!DynSys::operator==(rhs)) return false;
  else if ((dims.size() != rhs.dims.size()) || !std::equal(dims.cbegin(), dims.cend(), rhs.dims.cbegin())) return false;
  return true;
}

bool BicycleCAvoid::operator==(const DynSys& rhs) const {
    if (this == &rhs) return true;
    else if (typeid(*this) != typeid(rhs)) return false;
    else return operator==(dynamic_cast<const BicycleCAvoid&>(rhs));
}

bool BicycleCAvoid::save(
    beacls::MatFStream* fs,
    beacls::MatVariable* variable_ptr) {

  bool result = DynSys::save(fs, variable_ptr);
  if (!dims.empty()) result &= save_vector(dims, std::string("dims"), beacls::IntegerVec(), true, fs, variable_ptr);
  return result;
}

bool BicycleCAvoid::optCtrl_i_cell_helper(
    beacls::FloatVec& uOpt_i, // Relevant component i of the optimal input
    const std::vector<const FLOAT_TYPE*>& derivs,
    const beacls::IntegerVec& deriv_sizes,
    const helperOC::DynSys_UMode_Type uMode,
    const size_t src_target_dim_index, // Relevant state j affected by input i
    const beacls::FloatVec& uExtr_i // [u_minimizer_xj_dot, u_maximizer_xj_dot]
    ) const {

  if (src_target_dim_index < dims.size()) {
    const FLOAT_TYPE* deriv_j = derivs[src_target_dim_index];
    const size_t length = deriv_sizes[src_target_dim_index];

    if (length == 0 || deriv_j == NULL) 
      return false;

    uOpt_i.resize(length);
    
    switch (uMode) {
      case helperOC::DynSys_UMode_Max:
        for (size_t ii = 0; ii < length; ++ii) { // iterate over grid
          uOpt_i[ii] = (deriv_j[ii] >= 0) ? uExtr_i[1] : uExtr_i[0];
        }
        break;
     
      case helperOC::DynSys_UMode_Min:
        for (size_t ii = 0; ii < length; ++ii) {
          uOpt_i[ii] = (deriv_j[ii] >= 0) ? uExtr_i[0] : uExtr_i[1];
        }
        break;
     
      case helperOC::DynSys_UMode_Invalid:
     
      default:
          std::cerr << "Unknown uMode!: " << uMode << std::endl;
          return false;
    }
  }

  return true;
} 


bool BicycleCAvoid::optDstb_i_cell_helper(
    beacls::FloatVec& dOpt_i, // Relevant component i of the optimal input
    const std::vector<const FLOAT_TYPE*>& derivs,
    const beacls::IntegerVec& deriv_sizes,
    const helperOC::DynSys_DMode_Type dMode,
    const size_t src_target_dim_index, // Relevant state j affected by input i
    const beacls::FloatVec& dExtr_i // [u_minimizer_xj_dot, u_maximizer_xj_dot]
    ) const {
  
  if (src_target_dim_index < dims.size()) {
    const FLOAT_TYPE* deriv_j = derivs[src_target_dim_index];
    const size_t length = deriv_sizes[src_target_dim_index];

    if (length == 0 || deriv_j == NULL) 
      return false;

    dOpt_i.resize(length);

    switch (dMode) {
      case helperOC::DynSys_DMode_Max:
        for (size_t ii = 0; ii < length; ++ii) { // iterate over grid
            dOpt_i[ii] = (deriv_j[ii] >= 0) ? dExtr_i[1] : dExtr_i[0];
        }
        break;
      case helperOC::DynSys_DMode_Min:
        for (size_t ii = 0; ii < length; ++ii) {
            dOpt_i[ii] = (deriv_j[ii] >= 0) ? dExtr_i[0] : dExtr_i[1];
        }
        break;
      case helperOC::DynSys_DMode_Invalid:
      default:
        std::cerr << "Unknown dMode!: " << dMode << std::endl;
        return false;
    }
  }
  return true;
} 


bool BicycleCAvoid::optCtrl(
    std::vector<beacls::FloatVec>& uOpts,
    const FLOAT_TYPE,
    const std::vector<beacls::FloatVec::const_iterator>&,
    const std::vector<const FLOAT_TYPE*>& deriv_ptrs,
    const beacls::IntegerVec&,
    const beacls::IntegerVec& deriv_sizes,
    const helperOC::DynSys_UMode_Type uMode) const {
  const helperOC::DynSys_UMode_Type modified_uMode =
    (uMode == helperOC::DynSys_UMode_Default) ? 
    helperOC::DynSys_UMode_Max : uMode;

  // Why is this needed? It is found in Plane.cpp
  for (size_t dim = 0; dim < 5; ++dim) {
    if (deriv_sizes[dim] == 0 || deriv_ptrs[dim] == NULL) {
      return false;
    }
  }

  uOpts.resize(get_nu());

  bool result = true;

  // Call helper to determine optimal value for each control component
  // (we feed the relevant state component affected by each input as well as
  //  the input values that maximize and minimize this state's derivative).
  result &= optCtrl_i_cell_helper(uOpts[0], deriv_ptrs, deriv_sizes,
      modified_uMode, 3, aRange);

  const beacls::FloatVec& alphaRange{-alphaMax, alphaMax};
  result &= optCtrl_i_cell_helper(uOpts[1], deriv_ptrs, deriv_sizes,
      modified_uMode, 4, alphaRange);
  return result;
}


bool BicycleCAvoid::optDstb(
    std::vector<beacls::FloatVec>& dOpts,
    const FLOAT_TYPE,
    const std::vector<beacls::FloatVec::const_iterator>& x_ites,
    const std::vector<const FLOAT_TYPE*>& deriv_ptrs,
    const beacls::IntegerVec&,
    const beacls::IntegerVec& deriv_sizes,
    const helperOC::DynSys_DMode_Type dMode
    ) const {
  const helperOC::DynSys_DMode_Type modified_dMode =
    (dMode == helperOC::DynSys_DMode_Default) ?
    helperOC::DynSys_DMode_Min : dMode;

  if ((modified_dMode != helperOC::DynSys_DMode_Max) && 
      (modified_dMode != helperOC::DynSys_DMode_Min)) {
    std::cerr << "Unknown dMode!: " << modified_dMode << std::endl;
    return false;
  }

  // Why is this needed? It is found in Plane.cpp
  for (size_t dim = 0; dim < 5; ++dim) {
    if (deriv_sizes[dim] == 0 || deriv_ptrs[dim] == NULL) {
      return false;
    }
  }

  dOpts.resize(get_nd());
  bool result = true;

  // Call helper to determine optimal value for each disturbance component
  // (we feed the relevant state component affected by each input as well as
  //  the input values that maximize and minimize this state's derivative).

  // Disturbances
  const beacls::FloatVec& dRange0{-dMax[0],dMax[0]};
  const beacls::FloatVec& dRange1{-dMax[1],dMax[1]};
  const beacls::FloatVec& dRange2{-dMax[2],dMax[2]};
  const beacls::FloatVec& dRange3{-dMax[3],dMax[3]};
  const beacls::FloatVec& dRange4{-dMax[4],dMax[4]};
  
  result &= optDstb_i_cell_helper(dOpts[0], deriv_ptrs, deriv_sizes,
      modified_dMode, 0, dRange0);
  result &= optDstb_i_cell_helper(dOpts[1], deriv_ptrs, deriv_sizes,
      modified_dMode, 1, dRange1);
  result &= optDstb_i_cell_helper(dOpts[2], deriv_ptrs, deriv_sizes,
      modified_dMode, 2, dRange2);
  result &= optDstb_i_cell_helper(dOpts[3], deriv_ptrs, deriv_sizes,
      modified_dMode, 3, dRange3);
  result &= optDstb_i_cell_helper(dOpts[4], deriv_ptrs, deriv_sizes,
      modified_dMode, 4, dRange4);

  // Planning control
  // assume state is a matrix (not a single state)

  dOpts[5].resize(deriv_sizes[0]);

  beacls::FloatVec::const_iterator xs0 = x_ites[0];
  beacls::FloatVec::const_iterator xs1 = x_ites[1];

  const FLOAT_TYPE* deriv0_ptr = deriv_ptrs[0];
  const FLOAT_TYPE* deriv1_ptr = deriv_ptrs[1];
  const FLOAT_TYPE* deriv2_ptr = deriv_ptrs[2];

  const FLOAT_TYPE w_if_det5_pos = 
    (modified_dMode == helperOC::DynSys_DMode_Max) ? wMax : -wMax;
  const FLOAT_TYPE w_if_det5_neg =
    (modified_dMode == helperOC::DynSys_DMode_Max) ? -wMax : wMax;

  for (size_t index = 0; index < deriv_sizes[0]; ++index) {
    const FLOAT_TYPE x0 = xs0[index];
    const FLOAT_TYPE x1 = xs1[index];
    const FLOAT_TYPE deriv0 = deriv0_ptr[index];
    const FLOAT_TYPE deriv1 = deriv1_ptr[index];
    const FLOAT_TYPE deriv2 = deriv2_ptr[index];

    const FLOAT_TYPE det5 = deriv0*x1 - deriv1*x0 - deriv2;

    dOpts[5][index] = (det5 >= 0) ? w_if_det5_pos : w_if_det5_neg;
  }
  return result;
}

FLOAT_TYPE fialaTireModel(const FLOAT_TYPE a,
                          const FLOAT_TYPE Ca,
                          const FLOAT_TYPE mu,
                          const FLOAT_TYPE Fx,
                          const FLOAT_TYPE Fz) {
  FLOAT_TYPE Fmax = mu * Fz;
  if (std::abs(Fx) >= Fmax)
    return 0;
  else {
    FLOAT_TYPE Fymax = std::sqrt(Fmax * Fmax - Fx * Fx);
    FLOAT_TYPE tana = std::tan(a);
    FLOAT_TYPE tana_slide = 3 * Fymax / Ca;
    FLOAT_TYPE ratio = std::abs(tana / tana_slide);
    if (ratio < 1)
      return -Ca * tana * (1 - ratio + ratio * ratio / 3);
    else
      return -std::copysign(Fymax, tana);
  }
}

bool BicycleCAvoid::dynamics_cell_helper(
    std::vector<beacls::FloatVec>& dxs,
    const beacls::FloatVec::const_iterator& x_rel,
    const beacls::FloatVec::const_iterator& y_rel,
    const beacls::FloatVec::const_iterator& psi_rel,
    const beacls::FloatVec::const_iterator& Ux,
    const beacls::FloatVec::const_iterator& Uy,
    const beacls::FloatVec::const_iterator& V,
    const beacls::FloatVec::const_iterator& r,
    const std::vector<beacls::FloatVec >& us,
    const std::vector<beacls::FloatVec >& ds,
    const size_t size_x_rel,
    const size_t size_y_rel,
    const size_t size_psi_rel,
    const size_t size_Ux,
    const size_t size_Uy,
    const size_t size_V,
    const size_t size_r,
    const size_t dim) const {

  beacls::FloatVec& dx_i = dxs[dim];
  bool result = true;

  switch (dims[dim]) {
    case 0: { // x_rel_dot = V * cos(psi_rel) - Ux + y_rel * r
      dx_i.resize(size_x_rel);

      for (size_t i = 0; i < size_x_rel; ++i) {
        dx_i[i] = V[i] * std::cos(psi_rel[i]) - Ux[i] + y_rel[i] * r[i];
      }
    } break;

    case 1: { // y_rel_dot = V * sin(psi_rel) - Uy - x_rel * r
      dx_i.resize(size_y_rel);

      for (size_t i = 0; i < size_y_rel; ++i) {
        dx_i[i] = V[i] * std::sin(psi_rel[i]) - Uy[i] - x_rel[i] * r[i];
      }
    } break;

    case 2: { // psi_rel_dot = w - r
      dx_i.resize(size_psi_rel);
      const beacls::FloatVec& w = ds[0];
      FLOAT_TYPE wi;

      for (size_t i = 0; i < size_psi_rel; ++i) {
        if (w.size() == size_psi_rel)
          wi = w[i];
        else
          wi = w[0];
        dx_i[i] = wi - r[i];
      }
    } break;

    case 3: {   // Ux_dot = (Fxf + Fxr + Fx_drag) / m + r * Uy
      dx_i.resize(size_Ux);
      const beacls::FloatVec& Fxf = us[1];
      const beacls::FloatVec& Fxr = us[2];
      FLOAT_TYPE Fxfi, Fxri, Fx_dragi;

      for (size_t i = 0; i < size_Ux; ++i) {
        if (Fxf.size() == size_Ux)
          Fxfi = Fxf[i];
        else
          Fxfi = Fxf[0];
        if (Fxr.size() == size_Ux)
          Fxri = Fxr[i];
        else
          Fxri = Fxr[0];
        Fx_dragi = -X1::Cd0 - Ux[i] * (X1::Cd1 + X1::Cd2 * Ux[i]);
        dx_i[i] = (Fxfi + Fxri + Fx_dragi) / X1::m + r[i] * Uy[i];
      }
    } break;

    case 4: { // Uy_dot = (Fyf + Fyr) / m - r * Ux
      dx_i.resize(size_Uy);
      const beacls::FloatVec&   d = us[0];
      const beacls::FloatVec& Fxf = us[1];
      const beacls::FloatVec& Fxr = us[2];
      FLOAT_TYPE di, Fxfi, Fxri, afi, ari, Fxi, Fzfi, Fzri, Fyfi, Fyri;

      for (size_t i = 0; i < size_Uy; ++i) {
        if (d.size() == size_Uy)
          di = d[i];
        else
          di = d[0];
        if (Fxf.size() == size_Uy)
          Fxfi = Fxf[i];
        else
          Fxfi = Fxf[0];
        if (Fxr.size() == size_Uy)
          Fxri = Fxr[i];
        else
          Fxri = Fxr[0];
        afi = std::atan((Uy[i] + X1::a * r[i]) / Ux[i]) - di;
        ari = std::atan((Uy[i] - X1::b * r[i]) / Ux[i]);
        Fxi = Fxfi + Fxri;
        Fzfi = (X1::m * X1::G * X1::b - X1::h * Fxi) / X1::L;
        Fzri = (X1::m * X1::G * X1::a + X1::h * Fxi) / X1::L;
        Fyfi = fialaTireModel(afi, X1::Caf, X1::mu, Fxfi, Fzfi);
        Fyri = fialaTireModel(ari, X1::Car, X1::mu, Fxri, Fzri);
        dx_i[i] = (Fyfi + Fyri) / X1::m - r[i] * Ux[i];
      }
    } break;

    case 5: { // V_dot = a
      dx_i.resize(size_V);
      const beacls::FloatVec& a = ds[1];
      FLOAT_TYPE ai;

      for (size_t i = 0; i < size_V; ++i) {
        if (a.size() == size_V)
          ai = a[i];
        else
          ai = a[0];
        dx_i[i] = ai;
      }
    } break;

    case 6: { // r_dot = (a * Fyf - b * Fyr) / Izz
      dx_i.resize(size_Ur);
      const beacls::FloatVec&   d = us[0];
      const beacls::FloatVec& Fxf = us[1];
      const beacls::FloatVec& Fxr = us[2];
      FLOAT_TYPE di, Fxfi, Fxri, afi, ari, Fxi, Fzfi, Fzri, Fyfi, Fyri;

      for (size_t i = 0; i < size_Ur; ++i) {
        if (d.size() == size_Ur)
          di = d[i];
        else
          di = d[0];
        if (Fxf.size() == size_Ur)
          Fxfi = Fxf[i];
        else
          Fxfi = Fxf[0];
        if (Fxr.size() == size_Ur)
          Fxri = Fxr[i];
        else
          Fxri = Fxr[0];
        afi = std::atan((Uy[i] + X1::a * r[i]) / Ux[i]) - di;
        ari = std::atan((Uy[i] - X1::b * r[i]) / Ux[i]);
        Fxi = Fxfi + Fxri;
        Fzfi = (X1::m * X1::G * X1::b - X1::h * Fxi) / X1::L;
        Fzri = (X1::m * X1::G * X1::a + X1::h * Fxi) / X1::L;
        Fyfi = fialaTireModel(afi, X1::Caf, X1::mu, Fxfi, Fzfi);
        Fyri = fialaTireModel(ari, X1::Car, X1::mu, Fxri, Fzri);
        dx_i[i] = (X1::a * Fyfi - X1::b * Fyri) / X1::Izz;
      }
    } break;

    default:
      std::cerr << "Only dimension 1-7 are defined for dynamics of BicycleCAvoid!" << std::endl;
      result = false;
      break;
  }
  return result;
}

bool BicycleCAvoid::dynamics(
    std::vector<beacls::FloatVec>& dx,
    const FLOAT_TYPE,
    const std::vector<beacls::FloatVec::const_iterator>& x_ites,
    const std::vector<beacls::FloatVec>& us,
    const std::vector<beacls::FloatVec>& ds,
    const beacls::IntegerVec& x_sizes,
    const size_t dst_target_dim) const {

    bool result = true;
    // Compute dynamics for all components.
    if (dst_target_dim == std::numeric_limits<size_t>::max()) {
      for (size_t dim = 0; dim < 7; ++dim) {
        // printf("Dimension %zu \n", dim);
        result &= dynamics_cell_helper(
          dx, x_ites[0], x_ites[1], x_ites[2], x_ites[3], x_ites[4], x_ites[5], x_ites[6], us, ds,
          x_sizes[0], x_sizes[1], x_sizes[2], x_sizes[3], x_sizes[4], x_sizes[5], x_sizes[6], dim);
      }
    }
    // Compute dynamics for a single, specified component.
    else
    {
      if (dst_target_dim < dims.size()) {
        // printf("Target dimension %zu \n", dst_target_dim);
        result &= dynamics_cell_helper(
          dx, x_ites[0], x_ites[1], x_ites[2], x_ites[3], x_ites[4], x_ites[5], x_ites[6], us, ds,
          x_sizes[0], x_sizes[1], x_sizes[2], x_sizes[3], x_sizes[4], x_sizes[5], x_sizes[6], dst_target_dim);
      }

      else {
        std::cerr << "Invalid target dimension for dynamics: " << dst_target_dim << std::endl;
        result = false;
      }
    }
    return result;
}