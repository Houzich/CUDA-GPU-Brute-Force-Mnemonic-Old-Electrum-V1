/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V2.0.0
  * @date		9-May-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */


#include <stdafx.h>

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



#include "Dispatcher.h"
#include "GPU.h"
#include "KernelStride.hpp"
#include "Helper.h"


#include "cuda_runtime.h"
#include "device_launch_parameters.h"


#include "../Tools/tools.h"
#include "../Tools/utils.h"
#include "../config/Config.hpp"
#include "../Tools/segwit_addr.h"



static std::thread save_thread;

int Generate_Mnemonic_And_Hash(void)
{
	cudaError_t cudaStatus = cudaSuccess;
	int err;
	ConfigClass Config;
	try {
		parse_config(&Config, "config.cfg");
		err = tools::stringToWordIndices(Config.static_words_generate_mnemonic, Config.words_indicies_mnemonic);
		if (err != 0)
		{
			std::cerr << "Error stringToWordIndices()!" << std::endl;
			return -1;
		}
		uint64_t number_of_generated_mnemonics = (Config.number_of_generated_mnemonics / (Config.cuda_block * Config.cuda_grid)) * (Config.cuda_block * Config.cuda_grid);
		if ((Config.number_of_generated_mnemonics % (Config.cuda_block * Config.cuda_grid)) != 0) number_of_generated_mnemonics += Config.cuda_block * Config.cuda_grid;
		Config.number_of_generated_mnemonics = number_of_generated_mnemonics;
	}
	catch (...) {
		for (;;)
			std::this_thread::sleep_for(std::chrono::seconds(30));
	}


	devicesInfo();
	// Choose which GPU to run on, change this on a multi-GPU system.
	uint32_t num_device = 0;
#ifndef TEST_MODE
	std::cout << "\n\nEnter number of device: ";
	std::cin >> num_device;
#endif //GENERATE_INFINITY
	cudaStatus = cudaSetDevice(num_device);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		return -1;
	}

	size_t num_wallets_gpu = Config.cuda_grid * Config.cuda_block;
	if (num_wallets_gpu < NUM_PACKETS_SAVE_IN_FILE)
	{
		std::cerr << "Error num_wallets_gpu < NUM_PACKETS_SAVE_IN_FILE!" << std::endl;
		return -1;
	}

	uint32_t num_bytes = 0;
	if (Config.chech_equal_bytes_in_adresses == "yes")
	{
#ifdef TEST_MODE
		num_bytes = 6;
#else
		num_bytes = 8;
#endif //TEST_MODE
	}

	std::cout << "\nNUM WALLETS IN ROUND GPU: " << tools::formatWithCommas(num_wallets_gpu) << std::endl << std::endl;

	data_class* Data = new data_class();
	stride_class* Stride = new stride_class(Data);
	size_t num_addresses_in_tables = 0;
	std::cout << "READ TABLES! WAIT..." << std::endl;
	tools::clearFiles();
	err = tools::readAllTables(Data->host.tables, Config.folder_tables, "", &num_addresses_in_tables);
	if (err == -1) {
		std::cerr << "Error get_all_tables segwit!" << std::endl;
		goto Error;
	}

	if (num_addresses_in_tables == 0) {
		std::cerr << "ERROR READ TABLES!! NO ADDRESSES IN FILES!!" << std::endl;
		goto Error;
	}

	if (Data->malloc(Config.cuda_grid, Config.cuda_block, Config.num_paths, Config.num_child_addresses, Config.save_generation_result_in_file == "yes" ? true : false) != 0) {
		std::cerr << "Error Data->Malloc()!" << std::endl;
		goto Error;
	}

	if (Stride->init() != 0) {
		std::cerr << "Error INIT!!" << std::endl;
		goto Error;
	}

	Data->host.freeTableBuffers();

	std::cout << "START GENERATE ADDRESSES!" << std::endl;
	std::cout << "\nGENERATE " << tools::formatWithCommas(Config.number_of_generated_mnemonics) << " MNEMONICS. " << tools::formatWithCommas(Config.number_of_generated_mnemonics * Data->num_all_childs) << " ADDRESSES. MNEMONICS IN ROUNDS " << tools::formatWithCommas(Data->wallets_in_round_gpu) << ". WAIT...\n\n";



	if (cudaMemcpyToSymbol(dev_num_bytes_find, &num_bytes, 4, 0, cudaMemcpyHostToDevice) != cudaSuccess)
	{
		std::cerr << "cudaMemcpyToSymbol to num_bytes_find failed!" << std::endl;
		goto Error;
	}
	if (cudaMemcpyToSymbol(dev_generate_path, &Config.generate_path, sizeof(Config.generate_path), 0, cudaMemcpyHostToDevice) != cudaSuccess)
	{
		std::cerr << "cudaMemcpyToSymbol to dev_generate_path failed!" << std::endl;
		goto Error;
	}
	if (cudaMemcpyToSymbol(dev_num_childs, &Config.num_child_addresses, 4, 0, cudaMemcpyHostToDevice) != cudaSuccess)
	{
		std::cerr << "cudaMemcpyToSymbol to dev_num_child failed!" << std::endl;
		goto Error;
	}
	if (cudaMemcpyToSymbol(dev_num_paths, &Config.num_paths, 4, 0, cudaMemcpyHostToDevice) != cudaSuccess)
	{
		std::cerr << "cudaMemcpyToSymbol to dev_num_paths failed!" << std::endl;
		goto Error;
	}
	if (cudaMemcpyToSymbol(dev_static_words_indices, &Config.words_indicies_mnemonic, NUM_WORDS_MNEMONIC * sizeof(uint16_t), 0, cudaMemcpyHostToDevice) != cudaSuccess)
	{
		std::cerr << "cudaMemcpyToSymbol to dev_gen_words_indices failed!" << std::endl;
		goto Error;
	}

	tools::generateRandomWordsIndex(Data->host.words_index, Data->size_words_index_buf / sizeof(uint16_t));

	for (uint64_t step = 0; step < Config.number_of_generated_mnemonics / (Data->wallets_in_round_gpu); step++)
	{
		tools::start_time();

		if (Config.save_generation_result_in_file == "yes") {
			if (Stride->start_for_save(Config.cuda_grid, Config.cuda_block) != 0) {
				std::cerr << "Error START!!" << std::endl;
				goto Error;
			}
		}
		else
		{
			if (Stride->start(Config.cuda_grid, Config.cuda_block) != 0) {
				std::cerr << "Error START!!" << std::endl;
				goto Error;
			}
		}


		tools::generateRandomWordsIndex(Data->host.words_index, Data->size_words_index_buf / sizeof(uint16_t));

		if (save_thread.joinable()) save_thread.join();

		if (Config.save_generation_result_in_file == "yes") {
			if (Stride->end_for_save() != 0) {
				std::cerr << "Error END!!" << std::endl;
				goto Error;
			}
		}
		else
		{
			if (Stride->end() != 0) {
				std::cerr << "Error END!!" << std::endl;
				goto Error;
			}
		}

		if (Config.save_generation_result_in_file == "yes") {
			save_thread = std::thread(&tools::saveResult, (char*)Data->host.mnemonic, (uint8_t*)Data->host.hash160, Data->wallets_in_round_gpu, Data->num_all_childs);
		}

		tools::checkResult(Data->host.ret);



		double delay;
		tools::stop_time_and_calc_sec(&delay);
		std::cout << "\rGENERATE: " << tools::formatWithCommas((double)Data->wallets_in_round_gpu / delay) << " MNEMONICS/SEC AND "
			<< tools::formatWithCommas((double)(Data->wallets_in_round_gpu * Data->num_all_childs) / delay) << " ADDRESSES/SEC"
			<< " | SCAN: " << tools::formatPrefix((double)(Data->wallets_in_round_gpu * Data->num_all_childs * num_addresses_in_tables) / delay) << " ADDRESSES/SEC"
			<< " | ROUND: " << step;
	}

	std::cout << "\n\nEND!" << std::endl;
	if (save_thread.joinable()) save_thread.join();
	// cudaDeviceReset must be called before exiting in order for profiling and
	// tracing tools such as Nsight and Visual Profiler to show complete traces.
	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
		return -1;
	}

	return 0;
Error:
	std::cout << "\n\nERROR!" << std::endl;
	if (save_thread.joinable()) save_thread.join();
	// cudaDeviceReset must be called before exiting in order for profiling and
	// tracing tools such as Nsight and Visual Profiler to show complete traces.
	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
		return -1;
	}

	return -1;
}







