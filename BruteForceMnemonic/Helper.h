/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V1.0.0
  * @date		20-March-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */

#pragma once
#include <stdint.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <omp.h>

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "stdafx.h"
#include "../Tools/utils.h"



class host_buffers_class
{
public:
	tableStruct tables[256] = { NULL };

	uint16_t* words_index = NULL;
	uint8_t* mnemonic = NULL;
	uint32_t* hash160 = NULL;

	retStruct* ret = NULL;

	uint64_t memory_size = 0;
public:
	host_buffers_class()
	{
	}

	int alignedMalloc(void** point, uint64_t size, uint64_t* all_ram_memory_size, std::string buff_name) {
		*point = _aligned_malloc(size, 4096);
		if (NULL == *point) { fprintf(stderr, "_aligned_malloc (%s) failed! Size: %s", buff_name.c_str(), tools::formatWithCommas(size).data()); return 1; }
		*all_ram_memory_size += size;
		//std::cout << "MALLOC RAM MEMORY SIZE (" << buff_name << "): " << std::to_string((float)size / (1024.0f * 1024.0f)) << " MB\n";
		return 0;
	}
	int mallocHost(void** point, uint64_t size, uint64_t* all_ram_memory_size, std::string buff_name) {
		if (cudaMallocHost((void**)point, size) != cudaSuccess) {
			fprintf(stderr, "cudaMallocHost (%s) failed! Size: %s", buff_name.c_str(), tools::formatWithCommas(size).data()); return -1;
		}
		*all_ram_memory_size += size;
		//std::cout << "MALLOC RAM MEMORY SIZE (" << buff_name << "): " << std::to_string((float)size / (1024.0f * 1024.0f)) << " MB\n";
		return 0;
	}
	int malloc(size_t size_words_index_buf, size_t size_mnemonic_buf, size_t size_hash160_buf)
	{
		memory_size = 0;
		if (mallocHost((void**)&words_index, size_words_index_buf, &memory_size, "words_index") != 0) return -1;
		if (alignedMalloc((void**)&mnemonic, size_mnemonic_buf, &memory_size, "mnemonic") != 0) return -1;
		if (mallocHost((void**)&hash160, size_hash160_buf, &memory_size, "hash160") != 0) return -1;
		if (mallocHost((void**)&ret, sizeof(retStruct), &memory_size, "ret") != 0) return -1;
		std::cout << "MALLOC ALL RAM MEMORY SIZE (HOST): " << std::to_string((float)memory_size / (1024.0f * 1024.0f)) << " MB\n";
		return 0;
	}
	void freeTableBuffers(void) {
		for (int x = 0; x < 256; x++) {
			if (tables[x].table != NULL)
			{
				free(tables[x].table);
				tables[x].table = NULL;
			}			
		}	
	}

	~host_buffers_class()
	{
		freeTableBuffers();
		cudaFreeHost(words_index);
		cudaFreeHost(hash160);
		cudaFreeHost(ret);
		//for CPU
		_aligned_free(mnemonic);

	}

};

class device_buffers_class
{
public:
	tableStruct tables[256] = { NULL };
	tableStruct* dev_tables;

	uint16_t* words_index = NULL;
	uint8_t* mnemonic = NULL;
	uint32_t* hash160 = NULL;

	retStruct* ret = NULL;

	uint64_t memory_size = 0;
public:
	device_buffers_class()
	{
	}

	int cudaMallocDevice(uint8_t** point, uint64_t size, uint64_t* all_gpu_memory_size, std::string buff_name) {
		if (cudaMalloc(point, size) != cudaSuccess) {
			fprintf(stderr, "cudaMalloc (%s) failed! Size: %s", buff_name.c_str(), tools::formatWithCommas(size).data()); return -1;
		}
		*all_gpu_memory_size += size;
		//std::cout << "MALLOC GPU MEMORY SIZE (" << buff_name << "): " << std::to_string((float)size / (1024.0f * 1024.0f)) << " MB\n";
		return 0;
	}
	int malloc(size_t size_words_index_buf, size_t size_mnemonic_buf, size_t size_hash160_buf, size_t num_wallet)
	{
		memory_size = 0;	
		if (cudaMallocDevice((uint8_t**)&words_index, size_words_index_buf, &memory_size, "words_index") != 0) return -1;
		if (cudaMallocDevice((uint8_t**)&mnemonic, size_mnemonic_buf, &memory_size, "mnemonic") != 0) return -1;
		if (cudaMallocDevice((uint8_t**)&hash160, size_hash160_buf, &memory_size, "hash160") != 0) return -1;
		if (cudaMallocDevice((uint8_t**)&dev_tables, sizeof(tableStruct) * 256, &memory_size, "dev_tables") != 0) return -1;
		if (cudaMallocDevice((uint8_t**)&ret, sizeof(retStruct), &memory_size, "ret") != 0) return -1;

		std::cout << "MALLOC ALL MEMORY SIZE (GPU): " << std::to_string((float)(memory_size) / (1024.0f * 1024.0f)) << " MB\n";
		return 0;
	}

	void freeTableBuffers(void) {
		for (int x = 0; x < 256; x++) {
			if (tables[x].table != NULL)
				cudaFree((void *)tables[x].table);
		}
		cudaFree(dev_tables);
	}

	~device_buffers_class()
	{
		freeTableBuffers();
		cudaFree(words_index);
		cudaFree(mnemonic);
		cudaFree(hash160);
		cudaFree(dev_tables);
		cudaFree(ret);
	}
};


class data_class
{
public:
	device_buffers_class dev;
	host_buffers_class host;

	cudaStream_t stream1 = NULL;
	size_t size_words_index_buf = 0;
	size_t size_mnemonic_buf = 0;
	size_t size_hash160_buf = 0;
	size_t wallets_in_round_gpu = 0;
public:
	data_class()
	{

	}

	int malloc(size_t cuda_grid, size_t cuda_block, bool alloc_buff_for_save)
	{
		size_t num_wallet = cuda_grid * cuda_block;
		size_t size_words_index_buf = SIZE_WORDS_IDX_FRAME;
		size_t size_mnemonic_buf = SIZE_MNEMONIC_FRAME * num_wallet;
		size_t size_hash160_buf = 20 * num_wallet * NUM_ALL_CHILDS;
		if (!alloc_buff_for_save)
		{
			size_mnemonic_buf = 0;
			size_hash160_buf = 0;
		}


		if (cudaStreamCreate(&stream1) != cudaSuccess) { fprintf(stderr, "cudaStreamCreate failed!  stream1"); return -1; }
		if (dev.malloc(size_words_index_buf, size_mnemonic_buf, size_hash160_buf, num_wallet) != 0) return -1;
		if (host.malloc(size_words_index_buf, size_mnemonic_buf, size_hash160_buf) != 0) return -1;
		this->size_words_index_buf = size_words_index_buf;
		this->size_mnemonic_buf = size_mnemonic_buf;
		this->size_hash160_buf = size_hash160_buf;
		this->wallets_in_round_gpu = num_wallet;
		return 0;
	}
	~data_class()
	{
		cudaStreamDestroy(stream1);
	}
};


cudaError_t deviceSynchronize(std::string name_kernel);
void devicesInfo(void);

