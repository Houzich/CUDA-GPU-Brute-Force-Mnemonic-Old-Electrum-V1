/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V1.0.0
  * @date		20-March-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */

#include <stdafx.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "Helper.h"




cudaError_t deviceSynchronize(std::string name_kernel) {
	cudaError_t cudaStatus = cudaSuccess;
	// Check for any errors launching the kernel
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaGetLastError \"%s\" launch failed: %s\n", name_kernel.c_str(), cudaGetErrorString(cudaStatus));
		return cudaStatus;
	}

	// cudaDeviceSynchronize waits for the kernel to finish, and returns
	// any errors encountered during the launch.
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceSynchronize \"%s\" returned error code \"%s\" after launching addKernel!\n", name_kernel.c_str(), cudaGetErrorString(cudaStatus));
		return cudaStatus;
	}
	return cudaStatus;
}

// Beginning of GPU Architecture definitions
inline int _ConvertSMVer2Cores(int major, int minor)
{
	// Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
	typedef struct
	{
		int SM; // 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
		int Cores;
	} sSMtoCores;

	sSMtoCores nGpuArchCoresPerSM[] =
	{
		{ 0x10,  8 }, // Tesla Generation (SM 1.0) G80 class
		{ 0x11,  8 }, // Tesla Generation (SM 1.1) G8x class
		{ 0x12,  8 }, // Tesla Generation (SM 1.2) G9x class
		{ 0x13,  8 }, // Tesla Generation (SM 1.3) GT200 class
		{ 0x20, 32 }, // Fermi Generation (SM 2.0) GF100 class
		{ 0x21, 48 }, // Fermi Generation (SM 2.1) GF10x class
		{ 0x30, 192}, // Kepler Generation (SM 3.0) GK10x class
		{ 0x35, 192}, // Kepler Generation (SM 3.5) GK11x class
		{   -1, -1 }
	};

	int index = 0;

	while (nGpuArchCoresPerSM[index].SM != -1)
	{
		if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor))
		{
			return nGpuArchCoresPerSM[index].Cores;
		}

		index++;
	}

	// If we don't find the values, we default use the previous one to run properly
	printf("MapSMtoCores for SM %d.%d is undefined.  Default to use %d Cores/SM\n", major, minor, nGpuArchCoresPerSM[7].Cores);
	return nGpuArchCoresPerSM[7].Cores;
}
// end of GPU Architecture definitions


void devicesInfo(void)
{
	int deviceCount = 0;
	cudaGetDeviceCount(&deviceCount);

	if (deviceCount == 0)
	{
		printf("\nThere are no available device(s) that support CUDA\n");
	}
	else
	{
		printf("\nDetected %d CUDA Capable device(s)\n", deviceCount);
	}

	int dev = 0, driverVersion = 0, runtimeVersion = 0;
	deviceCount = 1;
	for (dev = 0; dev < deviceCount; ++dev)
	{
		cudaSetDevice(dev);
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, dev);

		printf("\nDevice %d: \"%s\"\n", dev, deviceProp.name);

		// Console log
		cudaDriverGetVersion(&driverVersion);
		cudaRuntimeGetVersion(&runtimeVersion);
		printf("  CUDA Driver Version / Runtime Version          %d.%d / %d.%d\n", driverVersion / 1000, (driverVersion % 100) / 10, runtimeVersion / 1000, (runtimeVersion % 100) / 10);
		//printf("  CUDA Capability Major/Minor version number:    %d.%d\n", deviceProp.major, deviceProp.minor);

		char msg[256];
		sprintf(msg, "  Total amount of global memory:                 %.0f MBytes (%llu bytes)\n",
			(float)deviceProp.totalGlobalMem / 1048576.0f, (unsigned long long) deviceProp.totalGlobalMem);
		printf("%s", msg);

		//printf("  (%2d) Multiprocessors, (%3d) CUDA Cores/MP:     %d CUDA Cores\n",
		//	deviceProp.multiProcessorCount,
		//	_ConvertSMVer2Cores(deviceProp.major, deviceProp.minor),
		//	_ConvertSMVer2Cores(deviceProp.major, deviceProp.minor) * deviceProp.multiProcessorCount);
		printf("  GPU Max Clock rate:                            %.0f MHz (%0.2f GHz)\n", (float)deviceProp.clockRate * 1e-3f, deviceProp.clockRate * 1e-6f);

//
//#if CUDART_VERSION >= 5000
//		// This is supported in CUDA 5.0 (runtime API device properties)
//		printf("  Memory Clock rate:                             %.0f Mhz\n", deviceProp.memoryClockRate * 1e-3f);
//		printf("  Memory Bus Width:                              %d-bit\n", deviceProp.memoryBusWidth);
//
//		if (deviceProp.l2CacheSize)
//		{
//			printf("  L2 Cache Size:                                 %d bytes\n", deviceProp.l2CacheSize);
//		}
//
//#else
//		// This only available in CUDA 4.0-4.2 (but these were only exposed in the CUDA Driver API)
//		int memoryClock;
//		getCudaAttribute<int>(&memoryClock, CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE, dev);
//		printf("  Memory Clock rate:                             %.0f Mhz\n", memoryClock * 1e-3f);
//		int memBusWidth;
//		getCudaAttribute<int>(&memBusWidth, CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH, dev);
//		printf("  Memory Bus Width:                              %d-bit\n", memBusWidth);
//		int L2CacheSize;
//		getCudaAttribute<int>(&L2CacheSize, CU_DEVICE_ATTRIBUTE_L2_CACHE_SIZE, dev);
//
//		if (L2CacheSize)
//		{
//			printf("  L2 Cache Size:                                 %d bytes\n", L2CacheSize);
//		}
//
//#endif
//
//		printf("  Maximum Texture Dimension Size (x,y,z)         1D=(%d), 2D=(%d, %d), 3D=(%d, %d, %d)\n",
//			deviceProp.maxTexture1D, deviceProp.maxTexture2D[0], deviceProp.maxTexture2D[1],
//			deviceProp.maxTexture3D[0], deviceProp.maxTexture3D[1], deviceProp.maxTexture3D[2]);
//		printf("  Maximum Layered 1D Texture Size, (num) layers  1D=(%d), %d layers\n",
//			deviceProp.maxTexture1DLayered[0], deviceProp.maxTexture1DLayered[1]);
//		printf("  Maximum Layered 2D Texture Size, (num) layers  2D=(%d, %d), %d layers\n",
//			deviceProp.maxTexture2DLayered[0], deviceProp.maxTexture2DLayered[1], deviceProp.maxTexture2DLayered[2]);
//
//
//		printf("  Total amount of constant memory:               %llu bytes\n", (uint64_t)deviceProp.totalConstMem);
//		printf("  Total amount of shared memory per block:       %llu bytes\n", (uint64_t)deviceProp.sharedMemPerBlock);
//		printf("  Total number of registers available per block: %d\n", deviceProp.regsPerBlock);
//		printf("  Warp size:                                     %d\n", deviceProp.warpSize);
//		printf("  Maximum number of threads per multiprocessor:  %d\n", deviceProp.maxThreadsPerMultiProcessor);
//		printf("  Maximum number of threads per block:           %d\n", deviceProp.maxThreadsPerBlock);
//		printf("  Max dimension size of a thread block (x,y,z): (%d, %d, %d)\n",
//			deviceProp.maxThreadsDim[0],
//			deviceProp.maxThreadsDim[1],
//			deviceProp.maxThreadsDim[2]);
//		printf("  Max dimension size of a grid size    (x,y,z): (%d, %d, %d)\n",
//			deviceProp.maxGridSize[0],
//			deviceProp.maxGridSize[1],
//			deviceProp.maxGridSize[2]);
//		printf("  Maximum memory pitch:                          %llu bytes\n", (uint64_t)deviceProp.memPitch);
//		printf("  Texture alignment:                             %llu bytes\n", (uint64_t)deviceProp.textureAlignment);
//		printf("  Concurrent copy and kernel execution:          %s with %d copy engine(s)\n", (deviceProp.deviceOverlap ? "Yes" : "No"), deviceProp.asyncEngineCount);
//		printf("  Run time limit on kernels:                     %s\n", deviceProp.kernelExecTimeoutEnabled ? "Yes" : "No");
//		printf("  Integrated GPU sharing Host Memory:            %s\n", deviceProp.integrated ? "Yes" : "No");
//		printf("  Support host page-locked memory mapping:       %s\n", deviceProp.canMapHostMemory ? "Yes" : "No");
//		printf("  Alignment requirement for Surfaces:            %s\n", deviceProp.surfaceAlignment ? "Yes" : "No");
//		printf("  Device has ECC support:                        %s\n", deviceProp.ECCEnabled ? "Enabled" : "Disabled");
//#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
//		printf("  CUDA Device Driver Mode (TCC or WDDM):         %s\n", deviceProp.tccDriver ? "TCC (Tesla Compute Cluster Driver)" : "WDDM (Windows Display Driver Model)");
//#endif
//		printf("  Device supports Unified Addressing (UVA):      %s\n", deviceProp.unifiedAddressing ? "Yes" : "No");
//		printf("  Device PCI Domain ID / Bus ID / location ID:   %d / %d / %d\n", deviceProp.pciDomainID, deviceProp.pciBusID, deviceProp.pciDeviceID);

		const char* sComputeMode[] =
		{
			"Default (multiple host threads can use ::cudaSetDevice() with device simultaneously)",
			"Exclusive (only one host thread in one process is able to use ::cudaSetDevice() with this device)",
			"Prohibited (no host thread can use ::cudaSetDevice() with this device)",
			"Exclusive Process (many threads in one process is able to use ::cudaSetDevice() with this device)",
			"Unknown",
			NULL
		};
		//printf("  Compute Mode:\n");
		//printf("     < %s >\n", sComputeMode[deviceProp.computeMode]);
	}
}





