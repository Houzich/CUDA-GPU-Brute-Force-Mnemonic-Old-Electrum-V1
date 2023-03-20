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



uint64_t number_of_addresses_generate = 0;
int num_bytes = 0;




int Generate_Mnemonic_And_Hash(void)
{
	cudaError_t cudaStatus = cudaSuccess;

	ConfigClass config;
	try {
		parse_config(&config, "config.cfg");
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

	size_t num_wallets_gpu = config.cuda_grid * config.cuda_block;

	tools::clearFiles();
	//18,446,744,073,709,551,615
	uint64_t number_of_addresses = 0;
	int count_save_data_in_file = 0;

	std::cout << "\nNUM WALLETS IN ROUND GPU: " << tools::formatWithCommas(num_wallets_gpu) << std::endl << std::endl;
#ifndef TEST_MODE
	std::cout << "Max value: 18,000,000,000,000,000,000 (18000000000000000000)" << std::endl;
	std::cout << "Enter number of generate mnemonic: ";
	std::cin >> number_of_addresses;
	number_of_addresses = (((number_of_addresses - 1) / (num_wallets_gpu)+1) * (num_wallets_gpu));

	std::cout << "Enter num rounds save data in file: ";
	std::cin >> count_save_data_in_file;

	std::cout << "!!!FOR TEST!!! Enter num bytes for check 6...8: ";
	std::cin >> num_bytes;
	if (num_bytes != 0)
		if ((num_bytes < 6) || (num_bytes > 8)) {
			std::cout << "Error num bytes. Won't be used!" << std::endl;
			num_bytes = 0;
		}


#else
	number_of_addresses = num_wallets_gpu*1;
	num_bytes = 5;
	count_save_data_in_file = 1;
#endif //TEST_MODE
	data_class* Board = new data_class();
	stride_class* Stride = new stride_class(Board);

	int err = tools::readAllTables(Board->host.tables, config.folder_database, "");
	if (err == -1) {
		std::cout << "Error get_all_tables segwit!" << std::endl;
		goto Error;
	}


	if (Board->malloc(config.cuda_grid, config.cuda_block, count_save_data_in_file == 0 ? false : true) != 0) {
		std::cout << "Error Board->Malloc()!" << std::endl;
		goto Error;
	}

	if (Stride->init() != 0) {
		printf("Error INIT!!\n");
		goto Error;
	}

	Board->host.freeTableBuffers();

	std::cout << "START GENERATE ADDRESSES!" << std::endl;
	std::cout << "\nGENERATE " << tools::formatWithCommas(number_of_addresses) << " MNEMONICS. " << tools::formatWithCommas(number_of_addresses * NUM_ALL_CHILDS) << " ADDRESSES. PACKET " << tools::formatWithCommas(Board->wallets_in_round_gpu) << ". WAIT...\n\n";

	tools::generateRandomWordsIndex(Board->host.words_index, WORDS_MNEMONIC);

	if (cudaMemcpyToSymbol(num_bytes_find, &num_bytes, 4, 0, cudaMemcpyHostToDevice) != cudaSuccess)
	{
		fprintf(stderr, "cudaMemcpyToSymbol to num_bytes_find failed!");
		goto Error;
	}


	static int start_save = 0;
	for (uint64_t step = 0; step < number_of_addresses / (Board->wallets_in_round_gpu); step++)
	{
		tools::start_time();

		number_of_addresses_generate = (step + 1) * (Board->wallets_in_round_gpu);
		if (start_save < count_save_data_in_file) {
			if (Stride->start_for_save(config.cuda_grid, config.cuda_block) != 0) {
				printf("Error START!!\n");
				goto Error;
			}
		}
		else
		{
			if (Stride->start(config.cuda_grid, config.cuda_block) != 0) {
				printf("Error START!!\n");
				goto Error;
			}
		}


		tools::generateRandomWordsIndex(Board->host.words_index, WORDS_MNEMONIC);
		if (start_save < count_save_data_in_file) {
			if (Stride->end_for_save() != 0) {
				printf("Error END!!\n");
				goto Error;
			}
		}
		else
		{
			if (Stride->end() != 0) {
				printf("Error END!!\n");
				goto Error;
			}
		}

		if (start_save < count_save_data_in_file) {
			start_save++;
			tools::saveResult((char*)Board->host.mnemonic, (uint8_t*)Board->host.hash160, Board->wallets_in_round_gpu);
		}

		tools::checkResult(Board->host.ret);

		float delay;
		tools::stop_time_and_calc(&delay);
		std::cout << "\rSPEED: " << std::setw(8) << std::fixed << tools::formatWithCommas((float)Board->wallets_in_round_gpu / (delay / 1000.0f)) << " MNEMONICS/SECOND AND "
			<< tools::formatWithCommas(((float)Board->wallets_in_round_gpu * NUM_ALL_CHILDS) / (delay / 1000.0f)) << " ADDRESSES/SECOND, ROUND: " << step;
	}

	std::cout << "\n\nEND!" << std::endl;

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
	// cudaDeviceReset must be called before exiting in order for profiling and
	// tracing tools such as Nsight and Visual Profiler to show complete traces.
	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
		return -1;
	}

	return -1;
}







