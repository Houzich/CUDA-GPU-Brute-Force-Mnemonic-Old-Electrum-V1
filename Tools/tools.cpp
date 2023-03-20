/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V1.0.0
  * @date		20-March-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */
#include "main.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <set>
#include <random>
#include <fstream>
#include <filesystem>

#include "../BruteForceMnemonic/stdafx.h"
#include "tools.h"
#include "utils.h"
#include "base58.h"
#include "segwit_addr.h"




namespace tools {


	void generateRandomWordsIndex(uint16_t* buff, size_t len) {
		std::random_device rd;
		std::uniform_int_distribution<uint16_t> distr;
		std::mt19937 eng(rd());

		for (int i = 0; i < len; i++)
		{
			buff[i] = distr(eng) % 1626;
		}

	}

	int pushToMemory(uint8_t* addr_buff, std::vector<std::string>& lines, int max_len) {
		int err = 0;
		for (int x = 0; x < lines.size(); x++) {
			const std::string line = lines[x];
			err = hexStringToBytes(line, &addr_buff[max_len * x], max_len);
			if (err != 0) {
				std::cerr << "\n!!!ERROR HASH160 TO BYTES: " << line << std::endl;
				return err;
			}
		}
		return err;
	}

	int readAllTables(tableStruct* tables, std::string path, std::string prefix)
	{
		int ret = 0;
		std::string num_tables;
		size_t all_lines = 0;
#pragma omp parallel for 
		for (int x = 0; x < 256; x++) {

			std::string table_name = byteToHexString(x);

			std::string file_path = path + "\\" + prefix + table_name + ".csv";

			std::ifstream inFile(file_path);
			int64_t cnt_lines = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');
			inFile.close();
			if (cnt_lines != 0) {
				tables[x].table = (uint32_t*)malloc(cnt_lines * 20);
				if (tables[x].table == NULL) {
					printf("Error: malloc failed to allocate buffers.Size %llu. From file %s\n", (unsigned long long int)(cnt_lines * 20), file_path.c_str());
					inFile.close();
					ret = -1;
					break;
				}
				tables[x].size = (uint32_t)_msize((void*)tables[x].table);
				memset((uint8_t*)tables[x].table, 0, cnt_lines * 20);
				inFile.open(file_path, std::ifstream::in);
				if (inFile.is_open())
				{
					std::vector<std::string> lines;
					std::string line;
					while (getline(inFile, line)) {
						lines.push_back(line);
					}

					ret = pushToMemory((uint8_t*)tables[x].table, lines, 20);
					if (ret != 0) {
						std::cerr << "\n!!!ERROR push_to_memory, file: " << file_path << std::endl;
						ret = -1;
						inFile.close();
						break;
					}

					if (cnt_lines != lines.size()) {
						std::cout << "cnt_lines != lines.size(): cnt_lines = " << cnt_lines << " lines.size() = " << lines.size() << std::endl;
					}
					inFile.close();
				}
				else
				{
					std::cerr << "\n!!!ERROR open file: " << file_path << std::endl;
					ret = -1;
					break;
				}
#pragma omp critical 
				{
					all_lines += cnt_lines;
					std::cout << "PROCESSED " << cnt_lines << " ROWS IN FILE " << file_path << "\r";
				}
			}
			else {
#pragma omp critical 
				{
					std::cout << "!!! WORNING !!! COUNT LINES IS 0, FILE " << file_path << std::endl;
				}
			}

		}

#ifdef	USE_REVERSE_32
#pragma omp parallel for 
		for (int i = 0; i < 256; i++) {
			size_t addrs = tables[i].size / 20;
			for (int x = 0; x < addrs; x++) {
				if (tables[i].table != NULL)
					reverseHashUint32(&tables[i].table[x * 5], &tables[i].table[x * 5]);
			}

		}
#endif //USE_REVERSE
		std::cout << "\nALL ADDRESSES IN FILES " << all_lines << std::endl;
		std::cout << "TEMP MALLOC ALL RAM MEMORY SIZE (DATABASE): " << std::to_string((float)(all_lines * 20) / (1024.0f * 1024.0f * 1024.0f)) << " GB\n";
		return ret;
	}

	void clearFiles() {
		std::ofstream out;
		out.open(FILE_PATH_RESULT);
		out.close();
	}
#define NUM_PACKETS_SAVE_IN_FILE 8
	void saveResult(char* mnemonic, uint8_t* hash160, size_t num_wallets) {
		std::ofstream out;
		out.open(FILE_PATH_RESULT, std::ios::app);
//#pragma omp parallel for 
		for (int x = 0; x < NUM_PACKETS_SAVE_IN_FILE; x++) {
			if (out.is_open())
			{
				for (int i = x * (int)num_wallets / NUM_PACKETS_SAVE_IN_FILE; i < (x * (int)num_wallets / NUM_PACKETS_SAVE_IN_FILE + (int)num_wallets / NUM_PACKETS_SAVE_IN_FILE); i++) {
					std::string addr;
					std::stringstream ss;
					ss << (const char*)&mnemonic[SIZE_MNEMONIC_FRAME * i];
					for (int ii = 0; ii < NUM_ALL_CHILDS; ii++) {
						uint8_t* hash = (uint8_t*)&hash160[(i * NUM_ALL_CHILDS + ii) * 20];
						encodeAddressBase58((const uint8_t*)hash, addr);
						ss << "," << addr;
					}
					ss << '\n';
//#pragma omp critical
//					{
						out << ss.str();
//					}
				}
			}
			else
			{
				printf("\n!!!ERROR create file %s!!!\n", FILE_PATH_RESULT);
			}
		}
		out.close();
	}
	void addFoundMnemonicInFile(std::string mnemonic, const char* address) {
		std::ofstream out;
		out.open(FILE_PATH_FOUND_ADDRESSES, std::ios::app);
		if (out.is_open())
		{
			std::time_t result = std::time(nullptr);
			out << mnemonic << "," << (const char*)address << "," << std::asctime(std::localtime(&result));
		}
		else
		{
			printf("\n!!!ERROR open file %s!!!\n", FILE_PATH_FOUND_ADDRESSES);
		}
		out.close();
	}

	void addInFileTest(std::string& mnemonic, std::string& hash160, std::string& hash160_in_table, std::string& addr, std::string& addr_in_table) {
		std::ofstream out;
		out.open(FILE_PATH_FOUND_BYTES, std::ios::app);
		if (out.is_open())
		{
			const std::time_t now = std::time(nullptr);
			out << mnemonic << "," << addr << "," << addr_in_table << "," << hash160 << "," << hash160_in_table << "," << std::asctime(std::localtime(&now));
		}
		else
		{
			printf("\n!!!ERROR open file %s!!!\n", FILE_PATH_FOUND_BYTES);
		}
		out.close();
	}

	int checkResult(retStruct* ret) {
		if (ret->flag_found == 1)
		{
			std::string mnemonic = wordsIndexToMnemonic(ret->words_index);
			std::string addr;

			tools::encodeAddressBase58((const uint8_t*)ret->hash160, addr);
			tools::addFoundMnemonicInFile(mnemonic, addr.c_str());
			std::cout << "!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n";
			std::cout << "!!!FOUND: " << mnemonic << ", " << addr << std::endl;
			std::cout << "!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n";

		}
		if (ret->flag_found_bytes == 2)
		{
			std::string hash160 = tools::bytesToHexString((const uint8_t*)ret->hash160_bytes, 20);
			uint32_t hash_reverse[5];
			tools::reverseHashUint32(ret->hash160_bytes_from_table, hash_reverse);
			std::string hash160_in_table = tools::bytesToHexString((const uint8_t*)hash_reverse, 20);
			std::string mnemonic = wordsIndexToMnemonic(ret->words_index_bytes);
			std::string addr;
			std::string addr_in_table;

			tools::encodeAddressBase58(hash160, addr);
			tools::encodeAddressBase58(hash160_in_table, addr_in_table);
			std::cout << "\n!!!FOUND BYTES: " << mnemonic << "," << addr << "," << addr_in_table << "," << hash160 << "," << hash160_in_table << " \n";
			tools::addInFileTest(mnemonic, hash160, hash160_in_table, addr, addr_in_table);
		}
		return 0;
	}

}
