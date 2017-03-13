#include <helperOC/DynSys/Plane/Plane.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <array>
#include <cuda_macro.hpp>
#include "Plane_cuda.hpp"

#if !defined(WITH_GPU) 
#if defined(USER_DEFINED_GPU_DYNSYS_FUNC)
namespace Plane_CUDA {
	bool optCtrl_execute_cuda(
		std::vector<beacls::UVec>& u_uvecs,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& deriv_uvecs,
		const FLOAT_TYPE wMax,
		const FLOAT_TYPE vrange_min,
		const FLOAT_TYPE vrange_max,
		const DynSys_UMode_Type uMode
	)
	{
		bool result = true;
		if ((uMode == DynSys_UMode_Max) || (uMode == DynSys_UMode_Min)) {
			const FLOAT_TYPE moded_vrange_max = (uMode == DynSys_UMode_Max) ? vrange_max : vrange_min;
			const FLOAT_TYPE moded_vrange_min = (uMode == DynSys_UMode_Max) ? vrange_min : vrange_max;
			const FLOAT_TYPE moded_wMax = (uMode == DynSys_UMode_Max) ? wMax : -wMax;
			beacls::reallocateAsSrc(u_uvecs[0], x_uvecs[2]);
			beacls::reallocateAsSrc(u_uvecs[1], deriv_uvecs[2]);
			FLOAT_TYPE* uOpt0_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[0]).ptr();
			FLOAT_TYPE* uOpt1_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[1]).ptr();
			const FLOAT_TYPE* y2_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[2]).ptr();
			const FLOAT_TYPE* deriv0_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[0]).ptr();
			const FLOAT_TYPE* deriv1_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[1]).ptr();
			const FLOAT_TYPE* deriv2_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[2]).ptr();
			if (is_cuda(deriv_uvecs[2])){
				for (size_t index = 0; index < x_uvecs[2].size(); ++index) {
					const FLOAT_TYPE y2 = y2_ptr[index];
					const FLOAT_TYPE deriv0 = deriv0_ptr[index];
					const FLOAT_TYPE deriv1 = deriv1_ptr[index];
					const FLOAT_TYPE det1 = deriv0 * cos_float_type(y2) + deriv1 * sin_float_type(y2);
					uOpt0_ptr[index] = (det1 >= 0) ? moded_vrange_max : moded_vrange_min;
				}
				for (size_t index = 0; index < deriv_uvecs[2].size(); ++index) {
					uOpt1_ptr[index] = (deriv2_ptr[index] >= 0) ? moded_wMax : -moded_wMax;
				}
			}
			else {
				const FLOAT_TYPE deriv0 = deriv0_ptr[0];
				const FLOAT_TYPE deriv1 = deriv1_ptr[0];
				const FLOAT_TYPE deriv2 = deriv2_ptr[0];
				for (size_t index = 0; index < x_uvecs[2].size(); ++index) {
					const FLOAT_TYPE y2 = y2_ptr[index];
					const FLOAT_TYPE det1 = deriv0 * cos_float_type(y2) + deriv1 * sin_float_type(y2);
					uOpt0_ptr[index] = (det1 >= 0) ? moded_vrange_max : moded_vrange_min;
				}
				uOpt1_ptr[0] = (deriv2 >= 0) ? moded_wMax : -moded_wMax;
			}
		}
		else {
			std::cerr << "Unknown uMode!: " << uMode << std::endl;
			result = false;
		}
		return result;
	}

	bool optDstb_execute_cuda(
		std::vector<beacls::UVec>& d_uvecs,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& deriv_uvecs,
		const beacls::FloatVec& dMax,
		const DynSys_DMode_Type dMode
	)
	{
		bool result = true;
		const FLOAT_TYPE dMax_0 = dMax[0];
		const FLOAT_TYPE dMax_1 = dMax[1];
		if ((dMode == DynSys_DMode_Max) || (dMode == DynSys_DMode_Min)) {
			const FLOAT_TYPE moded_dMax_0 = (dMode == DynSys_DMode_Max) ? dMax_0 : -dMax_0;
			const FLOAT_TYPE moded_dMax_1 = (dMode == DynSys_DMode_Max) ? dMax_1 : -dMax_1;
			beacls::reallocateAsSrc(d_uvecs[0], deriv_uvecs[0]);
			beacls::reallocateAsSrc(d_uvecs[1], deriv_uvecs[0]);
			beacls::reallocateAsSrc(d_uvecs[2], deriv_uvecs[2]);
			FLOAT_TYPE* dOpt0_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[0]).ptr();
			FLOAT_TYPE* dOpt1_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[1]).ptr();
			FLOAT_TYPE* dOpt2_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[2]).ptr();
			const FLOAT_TYPE* deriv0_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[0]).ptr();
			const FLOAT_TYPE* deriv1_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[1]).ptr();
			const FLOAT_TYPE* deriv2_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[2]).ptr();

			if (is_cuda(deriv_uvecs[0]) && is_cuda(deriv_uvecs[1]) && is_cuda(deriv_uvecs[2])) {
				for (size_t index = 0; index < x_uvecs[2].size(); ++index) {
					const FLOAT_TYPE deriv0 = deriv0_ptr[index];
					const FLOAT_TYPE deriv1 = deriv1_ptr[index];
					const FLOAT_TYPE deriv2 = deriv2_ptr[index];
					const FLOAT_TYPE normDeriv01 = std::sqrt(deriv0 * deriv0 + deriv1 * deriv1);
					dOpt0_ptr[index] = (normDeriv01 == 0) ? 0 : moded_dMax_0 * deriv0 / normDeriv01;
					dOpt1_ptr[index] = (normDeriv01 == 0) ? 0 : moded_dMax_0 * deriv1 / normDeriv01;
					dOpt2_ptr[index] = (deriv2 >= 0) ? moded_dMax_1 : -moded_dMax_1;
				}
			}
			else {
				const FLOAT_TYPE deriv0 = deriv0_ptr[0];
				const FLOAT_TYPE deriv1 = deriv1_ptr[0];
				const FLOAT_TYPE deriv2 = deriv2_ptr[0];
				const FLOAT_TYPE normDeriv = sqrt_float_type(deriv0 * deriv0 + deriv1 * deriv1);
				if (normDeriv == 0) {
					dOpt0_ptr[0] = 0;
					dOpt1_ptr[0] = 0;
				}
				else {
					dOpt0_ptr[0] = moded_dMax_0 * deriv0 / normDeriv;
					dOpt1_ptr[0] = moded_dMax_0 * deriv1 / normDeriv;
				}
				dOpt2_ptr[0] = (deriv2 >= 0) ? moded_dMax_1 : -moded_dMax_1;
			}
		}
		else {
			std::cerr << "Unknown dMode!: " << dMode << std::endl;
			result = false;
		}
		return result;
	}

	bool dynamics_cell_helper_execute_cuda_dimAll(
		std::vector<beacls::UVec>& dx_uvecs,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& u_uvecs,
		const std::vector<beacls::UVec>& d_uvecs
	) {
		bool result = true;
		const size_t src_x_dim_index = 2;
		beacls::reallocateAsSrc(dx_uvecs[0], x_uvecs[src_x_dim_index]);
		beacls::reallocateAsSrc(dx_uvecs[1], x_uvecs[src_x_dim_index]);
		beacls::reallocateAsSrc(dx_uvecs[2], u_uvecs[1]);
		FLOAT_TYPE* dx_dim0_ptr = beacls::UVec_<FLOAT_TYPE>(dx_uvecs[0]).ptr();
		FLOAT_TYPE* dx_dim1_ptr = beacls::UVec_<FLOAT_TYPE>(dx_uvecs[1]).ptr();
		FLOAT_TYPE* dx_dim2_ptr = beacls::UVec_<FLOAT_TYPE>(dx_uvecs[2]).ptr();
		const FLOAT_TYPE* us_0_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[0]).ptr();
		const FLOAT_TYPE* us_1_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[1]).ptr();
		const FLOAT_TYPE* ds_0_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[0]).ptr();
		const FLOAT_TYPE* ds_1_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[1]).ptr();
		const FLOAT_TYPE* ds_2_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[2]).ptr();
		const FLOAT_TYPE* x_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
		if (is_cuda(d_uvecs[0]) && is_cuda(d_uvecs[1])&& is_cuda(u_uvecs[1]) && is_cuda(d_uvecs[2])) {
			for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
				const FLOAT_TYPE u0 = us_0_ptr[index];
				const FLOAT_TYPE d0 = ds_0_ptr[index];
				const FLOAT_TYPE d1 = ds_1_ptr[index];
				const FLOAT_TYPE x = x_ptr[index];
				dx_dim0_ptr[index] = u0 * cos_float_type(x) + d0;
				dx_dim1_ptr[index] = u0 * sin_float_type(x) + d1;
				const FLOAT_TYPE u1 = us_1_ptr[index];
				const FLOAT_TYPE d2 = ds_2_ptr[index];
				dx_dim2_ptr[index] = u1 + d2;
			}
		}
		else if (!is_cuda(d_uvecs[0]) && !is_cuda(d_uvecs[1])&& !is_cuda(u_uvecs[1]) && !is_cuda(d_uvecs[2])) {
			const FLOAT_TYPE d0 = ds_0_ptr[0];
			const FLOAT_TYPE d1 = ds_1_ptr[0];
			for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
				const FLOAT_TYPE u0 = us_0_ptr[index];
				const FLOAT_TYPE x = x_ptr[index];
				dx_dim0_ptr[index] = u0 * cos_float_type(x) + d0;
				dx_dim1_ptr[index] = u0 * sin_float_type(x) + d1;
				const FLOAT_TYPE u1 = us_1_ptr[0];
				const FLOAT_TYPE d2 = ds_2_ptr[0];
				dx_dim2_ptr[0] = u1 + d2;
			}
		} else {
			std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
			result = false;
		}
		return result;
	}

	bool dynamics_cell_helper_execute_cuda(
		beacls::UVec& dx_uvec,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& u_uvecs,
		const std::vector<beacls::UVec>& d_uvecs,
		const size_t dim
	) {
		bool result = true;
		const size_t src_x_dim_index = 2;
		switch (dim) {
		case 0:
			if (beacls::is_cuda(u_uvecs[0])){
				beacls::reallocateAsSrc(dx_uvec, x_uvecs[src_x_dim_index]);
				FLOAT_TYPE* dx_dim_ptr = beacls::UVec_<FLOAT_TYPE>(dx_uvec).ptr();
				const FLOAT_TYPE* x_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
				const FLOAT_TYPE* us_0_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[0]).ptr();
				const FLOAT_TYPE* ds_0_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[0]).ptr();
				if (is_cuda(d_uvecs[0])) {
					for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
						const FLOAT_TYPE u0 = us_0_ptr[index];
						const FLOAT_TYPE d0 = ds_0_ptr[index];
						const FLOAT_TYPE x = x_ptr[index];
						dx_dim_ptr[index] = u0 * cos_float_type(x) + d0;
					}
				}
				else {	//!< ds_0_size != length
					const FLOAT_TYPE d0 = ds_0_ptr[0];
					for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
						const FLOAT_TYPE u0 = us_0_ptr[index];
						const FLOAT_TYPE x = x_ptr[index];
						dx_dim_ptr[index] = u0 * cos_float_type(x) + d0;
					}
				}
			}
			else {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			break;
		case 1:
			if (beacls::is_cuda(u_uvecs[0])) {
				beacls::reallocateAsSrc(dx_uvec, x_uvecs[src_x_dim_index]);
				FLOAT_TYPE* dx_dim_ptr = beacls::UVec_<FLOAT_TYPE>(dx_uvec).ptr();
				const FLOAT_TYPE* x_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
				const FLOAT_TYPE* us_0_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[0]).ptr();
				const FLOAT_TYPE* ds_1_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[1]).ptr();
				if (is_cuda(d_uvecs[1])) {
					for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
						const FLOAT_TYPE u0 = us_0_ptr[index];
						const FLOAT_TYPE d1 = ds_1_ptr[index];
						const FLOAT_TYPE x = x_ptr[index];
						dx_dim_ptr[index] = u0 * sin_float_type(x) + d1;
					}
				}
				else {	//!< ds_1_size != length
					const FLOAT_TYPE d1 = ds_1_ptr[0];
					for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
						const FLOAT_TYPE u0 = us_0_ptr[index];
						const FLOAT_TYPE x = x_ptr[index];
						dx_dim_ptr[index] = u0 * sin_float_type(x) + d1;
					}
				}
			}
			else {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			break;
		case 2:
			{
				beacls::reallocateAsSrc(dx_uvec, u_uvecs[1]);
				FLOAT_TYPE* dx_dim_ptr = beacls::UVec_<FLOAT_TYPE>(dx_uvec).ptr();
				const FLOAT_TYPE* us_1_ptr = beacls::UVec_<FLOAT_TYPE>(u_uvecs[1]).ptr();
				const FLOAT_TYPE* ds_2_ptr = beacls::UVec_<FLOAT_TYPE>(d_uvecs[2]).ptr();
				if (is_cuda(u_uvecs[1]) && is_cuda(d_uvecs[2])) {
					for (size_t index = 0; index < u_uvecs[1].size(); ++index) {
						const FLOAT_TYPE u1 = us_1_ptr[index];
						const FLOAT_TYPE d2 = ds_2_ptr[index];
						dx_dim_ptr[index] = u1 + d2;
					}
				}
				else if (!is_cuda(u_uvecs[1]) && !is_cuda(d_uvecs[2])) {
					const FLOAT_TYPE u1 = us_1_ptr[0];
					const FLOAT_TYPE d2 = ds_2_ptr[0];
					dx_dim_ptr[0] = u1 + d2;
				}
				else {
					std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
					result = false;
				}
			}
			break;
		default:
			std::cerr << "Only dimension 1-4 are defined for dynamics of Plane!" << std::endl;
			result = false;
			break;
		}
		return result;
	}

	bool optCtrl_execute_cuda(
		std::vector<beacls::UVec>& uL_uvecs,
		std::vector<beacls::UVec>& uU_uvecs,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& derivMin_uvecs,
		const std::vector<beacls::UVec>& derivMax_uvecs,
		const FLOAT_TYPE wMax,
		const FLOAT_TYPE vrange_min,
		const FLOAT_TYPE vrange_max,
		const DynSys_UMode_Type uMode
	)
	{
		bool result = true;
		if ((uMode == DynSys_UMode_Max) || (uMode == DynSys_UMode_Min)) {
			const FLOAT_TYPE moded_vrange_max = (uMode == DynSys_UMode_Max) ? vrange_max : vrange_min;
			const FLOAT_TYPE moded_vrange_min = (uMode == DynSys_UMode_Max) ? vrange_min : vrange_max;
			const FLOAT_TYPE moded_wMax = (uMode == DynSys_UMode_Max) ? wMax : -wMax;
			beacls::reallocateAsSrc(uU_uvecs[0], x_uvecs[2]);
			beacls::reallocateAsSrc(uU_uvecs[1], derivMax_uvecs[2]);
			beacls::reallocateAsSrc(uL_uvecs[0], x_uvecs[2]);
			beacls::reallocateAsSrc(uL_uvecs[1], derivMin_uvecs[2]);
			FLOAT_TYPE* uU_0_ptr = beacls::UVec_<FLOAT_TYPE>(uU_uvecs[0]).ptr();
			FLOAT_TYPE* uU_1_ptr = beacls::UVec_<FLOAT_TYPE>(uU_uvecs[1]).ptr();
			FLOAT_TYPE* uL_0_ptr = beacls::UVec_<FLOAT_TYPE>(uL_uvecs[0]).ptr();
			FLOAT_TYPE* uL_1_ptr = beacls::UVec_<FLOAT_TYPE>(uL_uvecs[1]).ptr();
			const FLOAT_TYPE* y2_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[2]).ptr();
			const FLOAT_TYPE* derivMax0_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[0]).ptr();
			const FLOAT_TYPE* derivMax1_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[1]).ptr();
			const FLOAT_TYPE* derivMax2_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[2]).ptr();
			const FLOAT_TYPE* derivMin0_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[0]).ptr();
			const FLOAT_TYPE* derivMin1_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[1]).ptr();
			const FLOAT_TYPE* derivMin2_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[2]).ptr();
			if (is_cuda(derivMax_uvecs[2]) && is_cuda(derivMin_uvecs[2])) {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			else {
				const FLOAT_TYPE derivMax0 = derivMax0_ptr[0];
				const FLOAT_TYPE derivMax1 = derivMax1_ptr[0];
				const FLOAT_TYPE derivMax2 = derivMax2_ptr[0];
				const FLOAT_TYPE derivMin0 = derivMin0_ptr[0];
				const FLOAT_TYPE derivMin1 = derivMin1_ptr[0];
				const FLOAT_TYPE derivMin2 = derivMin2_ptr[0];
				for (size_t index = 0; index < x_uvecs[2].size(); ++index) {
					const FLOAT_TYPE y2 = y2_ptr[index];
					const FLOAT_TYPE con_y2 = cos_float_type(y2);
					const FLOAT_TYPE sin_y2 = sin_float_type(y2);
					const FLOAT_TYPE detMax1 = derivMax0 * con_y2 + derivMax1 * sin_y2;
					uU_0_ptr[index] = (detMax1 >= 0) ? moded_vrange_max : moded_vrange_min;
					const FLOAT_TYPE detMin1 = derivMin0 * con_y2 + derivMin1 * sin_y2;
					uL_0_ptr[index] = (detMin1 >= 0) ? moded_vrange_max : moded_vrange_min;
				}
				uU_1_ptr[0] = (derivMax2 >= 0) ? moded_wMax : -moded_wMax;
				uL_1_ptr[0] = (derivMin2 >= 0) ? moded_wMax : -moded_wMax;
			}
		}
		else {
			std::cerr << "Unknown uMode!: " << uMode << std::endl;
			result = false;
		}
		return result;
	}

	bool optDstb_execute_cuda(
		std::vector<beacls::UVec>& dL_uvecs,
		std::vector<beacls::UVec>& dU_uvecs,
		const std::vector<beacls::UVec>&,
		const std::vector<beacls::UVec>& derivMin_uvecs,
		const std::vector<beacls::UVec>& derivMax_uvecs,
		const beacls::FloatVec& dMax,
		const DynSys_DMode_Type dMode
	)
	{
		bool result = true;
		const FLOAT_TYPE dMax_0 = dMax[0];
		const FLOAT_TYPE dMax_1 = dMax[1];
		if ((dMode == DynSys_DMode_Max) || (dMode == DynSys_DMode_Min)) {
			const FLOAT_TYPE moded_dMax_0 = (dMode == DynSys_DMode_Max) ? dMax_0 : -dMax_0;
			const FLOAT_TYPE moded_dMax_1 = (dMode == DynSys_DMode_Max) ? dMax_1 : -dMax_1;
			beacls::reallocateAsSrc(dU_uvecs[0], derivMax_uvecs[0]);
			beacls::reallocateAsSrc(dU_uvecs[1], derivMax_uvecs[0]);
			beacls::reallocateAsSrc(dU_uvecs[2], derivMax_uvecs[2]);
			beacls::reallocateAsSrc(dL_uvecs[0], derivMin_uvecs[0]);
			beacls::reallocateAsSrc(dL_uvecs[1], derivMin_uvecs[0]);
			beacls::reallocateAsSrc(dL_uvecs[2], derivMin_uvecs[2]);
			FLOAT_TYPE* dU_0_ptr = beacls::UVec_<FLOAT_TYPE>(dU_uvecs[0]).ptr();
			FLOAT_TYPE* dU_1_ptr = beacls::UVec_<FLOAT_TYPE>(dU_uvecs[1]).ptr();
			FLOAT_TYPE* dU_2_ptr = beacls::UVec_<FLOAT_TYPE>(dU_uvecs[2]).ptr();
			FLOAT_TYPE* dL_0_ptr = beacls::UVec_<FLOAT_TYPE>(dL_uvecs[0]).ptr();
			FLOAT_TYPE* dL_1_ptr = beacls::UVec_<FLOAT_TYPE>(dL_uvecs[1]).ptr();
			FLOAT_TYPE* dL_2_ptr = beacls::UVec_<FLOAT_TYPE>(dL_uvecs[2]).ptr();
			const FLOAT_TYPE* derivMax0_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[0]).ptr();
			const FLOAT_TYPE* derivMax1_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[1]).ptr();
			const FLOAT_TYPE* derivMax2_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[2]).ptr();
			const FLOAT_TYPE* derivMin0_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[0]).ptr();
			const FLOAT_TYPE* derivMin1_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[1]).ptr();
			const FLOAT_TYPE* derivMin2_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[2]).ptr();

			if (is_cuda(derivMax_uvecs[0]) && is_cuda(derivMax_uvecs[1]) && is_cuda(derivMax_uvecs[2])) {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			else {
				const FLOAT_TYPE derivMax0 = derivMax0_ptr[0];
				const FLOAT_TYPE derivMax1 = derivMax1_ptr[0];
				const FLOAT_TYPE derivMax2 = derivMax2_ptr[0];
				const FLOAT_TYPE derivMin0 = derivMin0_ptr[0];
				const FLOAT_TYPE derivMin1 = derivMin1_ptr[0];
				const FLOAT_TYPE derivMin2 = derivMin2_ptr[0];
				const FLOAT_TYPE normDerivMax = sqrt_float_type(derivMax0 * derivMax0 + derivMax1 * derivMax1);
				const FLOAT_TYPE normDerivMin = sqrt_float_type(derivMin0 * derivMin0 + derivMin1 * derivMin1);
				if (normDerivMax == 0) {
					dU_0_ptr[0] = 0;
					dU_1_ptr[0] = 0;
				}
				else {
					dU_0_ptr[0] = moded_dMax_0 * derivMax0 / normDerivMax;
					dU_1_ptr[0] = moded_dMax_0 * derivMax1 / normDerivMax;
				}
				if (normDerivMin == 0) {
					dL_0_ptr[0] = 0;
					dL_1_ptr[0] = 0;
				}
				else {
					dL_0_ptr[0] = moded_dMax_0 * derivMin0 / normDerivMin;
					dL_1_ptr[0] = moded_dMax_0 * derivMin1 / normDerivMin;
				}
				dU_2_ptr[0] = (derivMax2 >= 0) ? moded_dMax_1 : -moded_dMax_1;
				dL_2_ptr[0] = (derivMin2 >= 0) ? moded_dMax_1 : -moded_dMax_1;
			}
		}
		else {
			std::cerr << "Unknown dMode!: " << dMode << std::endl;
			result = false;
		}
		return result;
	}

	bool dynamics_cell_helper_execute_cuda(
		beacls::UVec& alpha_uvec,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& uL_uvecs,
		const std::vector<beacls::UVec>& uU_uvecs,
		const std::vector<beacls::UVec>& dL_uvecs,
		const std::vector<beacls::UVec>& dU_uvecs,
		const size_t dim
	) {
		bool result = true;
		const size_t src_x_dim_index = 2;
		switch (dim) {
		case 0:
			if (beacls::is_cuda(uU_uvecs[0]) && beacls::is_cuda(uL_uvecs[0])) {
				beacls::reallocateAsSrc(alpha_uvec, x_uvecs[src_x_dim_index]);
				FLOAT_TYPE* alpha_ptr = beacls::UVec_<FLOAT_TYPE>(alpha_uvec).ptr();
				const FLOAT_TYPE* x_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
				const FLOAT_TYPE* uUs_0_ptr = beacls::UVec_<FLOAT_TYPE>(uU_uvecs[0]).ptr();
				const FLOAT_TYPE* dUs_0_ptr = beacls::UVec_<FLOAT_TYPE>(dU_uvecs[0]).ptr();
				const FLOAT_TYPE* uLs_0_ptr = beacls::UVec_<FLOAT_TYPE>(uL_uvecs[0]).ptr();
				const FLOAT_TYPE* dLs_0_ptr = beacls::UVec_<FLOAT_TYPE>(dL_uvecs[0]).ptr();
				if (is_cuda(dU_uvecs[0]) && is_cuda(dL_uvecs[0])) {
					std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
					result = false;
				}
				else {	//!< ds_0_size != length
					const FLOAT_TYPE dU0 = dUs_0_ptr[0];
					const FLOAT_TYPE dL0 = dLs_0_ptr[0];
					for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
						const FLOAT_TYPE uU0 = uUs_0_ptr[index];
						const FLOAT_TYPE uL0 = uLs_0_ptr[index];
						const FLOAT_TYPE x = x_ptr[index];
						const FLOAT_TYPE dxUU = uU0 * cos_float_type(x) + dU0;
						const FLOAT_TYPE dxUL = uU0 * cos_float_type(x) + dL0;
						const FLOAT_TYPE dxLU = uL0 * cos_float_type(x) + dU0;
						const FLOAT_TYPE dxLL = uL0 * cos_float_type(x) + dL0;
						const FLOAT_TYPE max0 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxUU), abs_float_type<FLOAT_TYPE>(dxUL));
						const FLOAT_TYPE max1 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxLL), abs_float_type<FLOAT_TYPE>(dxLU));
						alpha_ptr[index] = max_float_type<FLOAT_TYPE>(max0, max1);
					}
				}
			}
			else {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			break;
		case 1:
			if (beacls::is_cuda(uU_uvecs[0]) && beacls::is_cuda(uL_uvecs[0])) {
				beacls::reallocateAsSrc(alpha_uvec, x_uvecs[src_x_dim_index]);
				FLOAT_TYPE* alpha_ptr = beacls::UVec_<FLOAT_TYPE>(alpha_uvec).ptr();
				const FLOAT_TYPE* x_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
				const FLOAT_TYPE* uUs_0_ptr = beacls::UVec_<FLOAT_TYPE>(uU_uvecs[0]).ptr();
				const FLOAT_TYPE* dUs_1_ptr = beacls::UVec_<FLOAT_TYPE>(dU_uvecs[1]).ptr();
				const FLOAT_TYPE* uLs_0_ptr = beacls::UVec_<FLOAT_TYPE>(uL_uvecs[0]).ptr();
				const FLOAT_TYPE* dLs_1_ptr = beacls::UVec_<FLOAT_TYPE>(dL_uvecs[1]).ptr();
				if (is_cuda(dU_uvecs[1]) && is_cuda(dU_uvecs[1])) {
					std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
					result = false;
				}
				else {	//!< ds_1_size != length
					const FLOAT_TYPE dU1 = dUs_1_ptr[0];
					const FLOAT_TYPE dL1 = dLs_1_ptr[0];
					for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
						const FLOAT_TYPE uU0 = uUs_0_ptr[index];
						const FLOAT_TYPE uL0 = uLs_0_ptr[index];
						const FLOAT_TYPE x = x_ptr[index];
						const FLOAT_TYPE dxUU = uU0 * sin_float_type(x) + dU1;
						const FLOAT_TYPE dxUL = uU0 * sin_float_type(x) + dL1;
						const FLOAT_TYPE dxLU = uL0 * sin_float_type(x) + dU1;
						const FLOAT_TYPE dxLL = uL0 * sin_float_type(x) + dL1;
						const FLOAT_TYPE max0 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxUU), abs_float_type<FLOAT_TYPE>(dxUL));
						const FLOAT_TYPE max1 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxLL), abs_float_type<FLOAT_TYPE>(dxLU));
						alpha_ptr[index] = max_float_type<FLOAT_TYPE>(max0, max1);
					}
				}
			}
			else {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			break;
		case 2:
		{
			beacls::reallocateAsSrc(alpha_uvec, uU_uvecs[1]);
			FLOAT_TYPE* alpha_ptr = beacls::UVec_<FLOAT_TYPE>(alpha_uvec).ptr();
			const FLOAT_TYPE* uUs_1_ptr = beacls::UVec_<FLOAT_TYPE>(uU_uvecs[1]).ptr();
			const FLOAT_TYPE* dUs_2_ptr = beacls::UVec_<FLOAT_TYPE>(dU_uvecs[2]).ptr();
			const FLOAT_TYPE* uLs_1_ptr = beacls::UVec_<FLOAT_TYPE>(uL_uvecs[1]).ptr();
			const FLOAT_TYPE* dLs_2_ptr = beacls::UVec_<FLOAT_TYPE>(dL_uvecs[2]).ptr();
			if (is_cuda(uU_uvecs[1]) && is_cuda(dU_uvecs[2])) {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
			else if (!is_cuda(uU_uvecs[1]) && !is_cuda(dU_uvecs[2]) && !is_cuda(uL_uvecs[1]) && !is_cuda(dL_uvecs[2])) {
				const FLOAT_TYPE uU1 = uUs_1_ptr[0];
				const FLOAT_TYPE dU2 = dUs_2_ptr[0];
				const FLOAT_TYPE uL1 = uLs_1_ptr[0];
				const FLOAT_TYPE dL2 = dLs_2_ptr[0];
				const FLOAT_TYPE dxUU = uU1 + dU2;
				const FLOAT_TYPE dxUL = uU1 + dL2;
				const FLOAT_TYPE dxLU = uL1 + dU2;
				const FLOAT_TYPE dxLL = uL1 + dL2;
				const FLOAT_TYPE max0 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxUU), abs_float_type<FLOAT_TYPE>(dxUL));
				const FLOAT_TYPE max1 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxLL), abs_float_type<FLOAT_TYPE>(dxLU));
				alpha_ptr[0] = max_float_type<FLOAT_TYPE>(max0, max1);
			}
			else {
				std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << " Invalid data size" << std::endl;
				result = false;
			}
		}
		break;
		default:
			std::cerr << "Only dimension 1-4 are defined for dynamics of Plane!" << std::endl;
			result = false;
			break;
		}
		return result;
	}

	bool HamFunction_cuda(
		beacls::UVec& hamValue_uvec,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& deriv_uvecs,
		const FLOAT_TYPE wMax,
		const FLOAT_TYPE vrange_min,
		const FLOAT_TYPE vrange_max,
		const beacls::FloatVec& dMax,
		const DynSys_UMode_Type uMode,
		const DynSys_DMode_Type dMode,
		const bool negate) {
		if ((uMode != DynSys_UMode_Max) && (uMode != DynSys_UMode_Min)) return false;
		if ((dMode != DynSys_DMode_Max) && (dMode != DynSys_DMode_Min)) return false;
		const FLOAT_TYPE moded_vrange_max = (uMode == DynSys_UMode_Max) ? vrange_max : vrange_min;
		const FLOAT_TYPE moded_vrange_min = (uMode == DynSys_UMode_Max) ? vrange_min : vrange_max;
		const FLOAT_TYPE moded_wMax = (uMode == DynSys_UMode_Max) ? wMax : -wMax;
		const FLOAT_TYPE dMax_0 = dMax[0];
		const FLOAT_TYPE dMax_1 = dMax[1];
		const FLOAT_TYPE moded_dMax_0 = (dMode == DynSys_DMode_Max) ? dMax_0 : -dMax_0;
		const FLOAT_TYPE moded_dMax_1 = (dMode == DynSys_DMode_Max) ? dMax_1 : -dMax_1;
		const size_t src_x_dim_index = 2;
		const FLOAT_TYPE* y2_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
		const FLOAT_TYPE* deriv0_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[0]).ptr();
		const FLOAT_TYPE* deriv1_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[1]).ptr();
		const FLOAT_TYPE* deriv2_ptr = beacls::UVec_<FLOAT_TYPE>(deriv_uvecs[2]).ptr();

		bool result = true;

		beacls::reallocateAsSrc(hamValue_uvec, x_uvecs[src_x_dim_index]);
		FLOAT_TYPE* hamValue_ptr = beacls::UVec_<FLOAT_TYPE>(hamValue_uvec).ptr();

		//!< Negate hamValue_ptr if backward reachable set
		if (negate) {
			for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
				const FLOAT_TYPE y2 = y2_ptr[index];
				const FLOAT_TYPE deriv0 = deriv0_ptr[index];
				const FLOAT_TYPE deriv1 = deriv1_ptr[index];
				const FLOAT_TYPE deriv2 = deriv2_ptr[index];

				FLOAT_TYPE dx0;
				FLOAT_TYPE dx1;
				FLOAT_TYPE dx2;
				get_dxs(dx0, dx1, dx2, y2, deriv0, deriv1, deriv2, moded_wMax, moded_vrange_min, moded_vrange_max, moded_dMax_0, moded_dMax_1);
				hamValue_ptr[index] = - (deriv0 * dx0 + deriv1 * dx1 + deriv2 * dx2);
			}
		}
		else {
			for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
				const FLOAT_TYPE y2 = y2_ptr[index];
				const FLOAT_TYPE deriv0 = deriv0_ptr[index];
				const FLOAT_TYPE deriv1 = deriv1_ptr[index];
				const FLOAT_TYPE deriv2 = deriv2_ptr[index];

				FLOAT_TYPE dx0;
				FLOAT_TYPE dx1;
				FLOAT_TYPE dx2;
				get_dxs(dx0, dx1, dx2, y2, deriv0, deriv1, deriv2, moded_wMax, moded_vrange_min, moded_vrange_max, moded_dMax_0, moded_dMax_1);
				hamValue_ptr[index] = (deriv0 * dx0 + deriv1 * dx1 + deriv2 * dx2);
			}
		}
		return result;
	}



	bool PartialFunction_cuda(
		beacls::UVec& alpha_uvec,
		const std::vector<beacls::UVec>& x_uvecs,
		const std::vector<beacls::UVec>& derivMin_uvecs,
		const std::vector<beacls::UVec>& derivMax_uvecs,
		const size_t dim,
		const FLOAT_TYPE wMax,
		const FLOAT_TYPE vrange_min,
		const FLOAT_TYPE vrange_max,
		const beacls::FloatVec& dMax,
		const DynSys_UMode_Type uMode,
		const DynSys_DMode_Type dMode
	) {
		if ((uMode != DynSys_UMode_Max) && (uMode != DynSys_UMode_Min)) return false;
		if ((dMode != DynSys_DMode_Max) && (dMode != DynSys_DMode_Min)) return false;
		const FLOAT_TYPE moded_vrange_max = (uMode == DynSys_UMode_Max) ? vrange_max : vrange_min;
		const FLOAT_TYPE moded_vrange_min = (uMode == DynSys_UMode_Max) ? vrange_min : vrange_max;
		const FLOAT_TYPE moded_wMax = (uMode == DynSys_UMode_Max) ? wMax : -wMax;
		const FLOAT_TYPE dMax_0 = dMax[0];
		const FLOAT_TYPE dMax_1 = dMax[1];
		const FLOAT_TYPE moded_dMax_0 = (dMode == DynSys_DMode_Max) ? dMax_0 : -dMax_0;
		const FLOAT_TYPE moded_dMax_1 = (dMode == DynSys_DMode_Max) ? dMax_1 : -dMax_1;
		const size_t src_x_dim_index = 2;

		const FLOAT_TYPE* y2_ptr = beacls::UVec_<FLOAT_TYPE>(x_uvecs[src_x_dim_index]).ptr();
		const FLOAT_TYPE* derivMax0_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[0]).ptr();
		const FLOAT_TYPE* derivMax1_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[1]).ptr();
		const FLOAT_TYPE* derivMax2_ptr = beacls::UVec_<FLOAT_TYPE>(derivMax_uvecs[2]).ptr();
		const FLOAT_TYPE* derivMin0_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[0]).ptr();
		const FLOAT_TYPE* derivMin1_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[1]).ptr();
		const FLOAT_TYPE* derivMin2_ptr = beacls::UVec_<FLOAT_TYPE>(derivMin_uvecs[2]).ptr();

		bool result = true;
		switch (dim) {
		case 0:
		{
			beacls::reallocateAsSrc(alpha_uvec, x_uvecs[src_x_dim_index]);
			FLOAT_TYPE* alpha_ptr = beacls::UVec_<FLOAT_TYPE>(alpha_uvec).ptr();
			const FLOAT_TYPE derivMax0 = derivMax0_ptr[0];
			const FLOAT_TYPE derivMax1 = derivMax1_ptr[0];
			const FLOAT_TYPE derivMin0 = derivMin0_ptr[0];
			const FLOAT_TYPE derivMin1 = derivMin1_ptr[0];
			const FLOAT_TYPE normDerivMax = sqrt_float_type(derivMax0 * derivMax0 + derivMax1 * derivMax1);
			const FLOAT_TYPE normDerivMin = sqrt_float_type(derivMin0 * derivMin0 + derivMin1 * derivMin1);
			const FLOAT_TYPE dU0 = (normDerivMax == 0) ? 0 : moded_dMax_0 * derivMax0 / normDerivMax;
			const FLOAT_TYPE dL0 = (normDerivMin == 0) ? 0 : moded_dMax_0 * derivMin0 / normDerivMin;

			for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
				const FLOAT_TYPE y2 = y2_ptr[index];
				FLOAT_TYPE cos_y2;
				FLOAT_TYPE sin_y2;
				sincos_float_type(y2, sin_y2, cos_y2);
				alpha_ptr[index] = getAlpha(cos_y2, sin_y2, cos_y2, derivMin0, derivMax0, derivMin1, derivMax1, dL0, dU0, moded_vrange_min, moded_vrange_max);
			}
		}
			break;
		case 1:
		{
			beacls::reallocateAsSrc(alpha_uvec, x_uvecs[src_x_dim_index]);
			FLOAT_TYPE* alpha_ptr = beacls::UVec_<FLOAT_TYPE>(alpha_uvec).ptr();
			const FLOAT_TYPE derivMax0 = derivMax0_ptr[0];
			const FLOAT_TYPE derivMax1 = derivMax1_ptr[0];
			const FLOAT_TYPE derivMin0 = derivMin0_ptr[0];
			const FLOAT_TYPE derivMin1 = derivMin1_ptr[0];
			const FLOAT_TYPE normDerivMax = sqrt_float_type(derivMax0 * derivMax0 + derivMax1 * derivMax1);
			const FLOAT_TYPE normDerivMin = sqrt_float_type(derivMin0 * derivMin0 + derivMin1 * derivMin1);
			const FLOAT_TYPE dU1 = (normDerivMax == 0) ? 0 : moded_dMax_0 * derivMax1 / normDerivMax;
			const FLOAT_TYPE dL1 = (normDerivMin == 0) ? 0 : moded_dMax_0 * derivMin1 / normDerivMin;

			for (size_t index = 0; index < x_uvecs[src_x_dim_index].size(); ++index) {
				const FLOAT_TYPE y2 = y2_ptr[index];
				FLOAT_TYPE cos_y2;
				FLOAT_TYPE sin_y2;
				sincos_float_type(y2, sin_y2, cos_y2);
				alpha_ptr[index] = getAlpha(cos_y2, sin_y2, sin_y2, derivMin0, derivMax0, derivMin1, derivMax1, dL1, dU1, moded_vrange_min, moded_vrange_max);
			}
		}
			break;
		case 2:
		{
			beacls::reallocateAsSrc(alpha_uvec, derivMax_uvecs[0]);
			FLOAT_TYPE* alpha_ptr = beacls::UVec_<FLOAT_TYPE>(alpha_uvec).ptr();
			const FLOAT_TYPE derivMax2 = derivMax2_ptr[0];
			const FLOAT_TYPE derivMin2 = derivMin2_ptr[0];
			const FLOAT_TYPE uU1 = (derivMax2 >= 0) ? moded_wMax : -moded_wMax;
			const FLOAT_TYPE dU2 = (derivMax2 >= 0) ? moded_dMax_1 : -moded_dMax_1;
			const FLOAT_TYPE uL1 = (derivMin2 >= 0) ? moded_wMax : -moded_wMax;
			const FLOAT_TYPE dL2 = (derivMin2 >= 0) ? moded_dMax_1 : -moded_dMax_1;
			const FLOAT_TYPE dxUU = uU1 + dU2;
			const FLOAT_TYPE dxUL = uU1 + dL2;
			const FLOAT_TYPE dxLU = uL1 + dU2;
			const FLOAT_TYPE dxLL = uL1 + dL2;
			const FLOAT_TYPE max0 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxUU), abs_float_type<FLOAT_TYPE>(dxUL));
			const FLOAT_TYPE max1 = max_float_type<FLOAT_TYPE>(abs_float_type<FLOAT_TYPE>(dxLL), abs_float_type<FLOAT_TYPE>(dxLU));
			alpha_ptr[0] = max_float_type<FLOAT_TYPE>(max0, max1);
		}
		break;
		default:
			std::cerr << "Only dimension 1-4 are defined for dynamics of Plane!" << std::endl;
			result = false;
			break;
		}

		return result;
	}
};
#endif /* defined(USER_DEFINED_GPU_DYNSYS_FUNC) */
#endif /* !defined(WITH_GPU) */
