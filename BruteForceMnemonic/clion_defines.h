#ifndef __CLION_DEFINES // pragma once
#define __CLION_DEFINES

#include "CL/cl.h"

#ifndef STATIC_KEYWORD
#define STATIC_KEYWORD static
#endif

#define __kernel
#define __global
#define __local
#define __constant	const
#define __private

#define half float

struct float2 { float x;          };
struct float3 { float x, y, z;    };
struct float4 { float x, y, z, w; };

// https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/commonFunctions.html
#define gentype float
//gentype		clamp		(gentype x, float minval, float maxval);
//gentype		degrees		(gentype radians);
//gentype		max			(gentype x, gentype y);
//gentype		min			(gentype x, gentype y);
//gentype		mix			(gentype x, gentype y, gentype a);
//gentype		radians		(gentype degrees);
//gentype		sign		(gentype x);
//gentype		smoothstep	(gentype edge0, gentype edge1, gentype x);
//gentype		step		(gentype edge, gentype x);
#undef gentype

// https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/barrier.html
enum	cl_mem_fence_flags
{
	CLK_LOCAL_MEM_FENCE,
	CLK_GLOBAL_MEM_FENCE
};
//void	barrier(cl_mem_fence_flags flags);

// https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/vectorDataLoadandStoreFunctions.html
#define gentype float
#define gentypen float4
//gentypen	vload4			(size_t offset, const gentype *p);
//void		vstore4			(gentypen data, size_t offset, gentype *p);
//void		vstore4			(gentypen data, size_t offset, gentype *p);
#undef gentypen
#undef gentype
//float		vload_half		(size_t offset, const half *p);
//float4		vload_half4		(size_t offset, const half *p);
//void		vstore_half		(float data, size_t offset, half *p);
//void		vstore_half4	(float4 data, size_t offset, half *p);
//float4		vloada_half4	(size_t offset, const half *p);
//void		vstorea_half4	(float4 data, size_t offset, half *p);

// https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/workItemFunctions.html
//size_t	get_global_size		(cl_uint dimindx);
//size_t	get_global_id		(cl_uint dimindx);
//size_t	get_local_size		(cl_uint dimindx);
//size_t	get_local_id		(cl_uint dimindx);
//size_t	get_num_groups		(cl_uint dimindx);
//size_t	get_group_id		(cl_uint dimindx);
//size_t	get_global_offset	(cl_uint dimindx);
//cl_uint	get_work_dim		(void);

// Defined in libs/gpu/libgpu/opencl/engine.cpp:584
// 64 for AMD, 32 for NVidia, 8 for intel GPUs, 1 for CPU
#define WARP_SIZE 64



#endif //__CLION_DEFINES
