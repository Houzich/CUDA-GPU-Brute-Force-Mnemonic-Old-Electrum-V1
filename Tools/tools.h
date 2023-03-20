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
#include <vector>
#include <string>
#include "../BruteForceMnemonic/stdafx.h"
namespace tools {

	void generateRandomWordsIndex(uint16_t* buff, size_t len);
	int pushToMemory(uint8_t* addr_buff, std::vector<std::string>& lines, int max_len);
	int readAllTables(tableStruct* tables, std::string path, std::string prefix);
	void clearFiles(void);
	void saveResult(char* mnemonic, uint8_t* hash160, size_t num_wallets);
	void addFoundMnemonicInFile(std::string mnemonic, const char* address);
	void addInFileTest(std::string& mnemonic, std::string& hash160, std::string& hash160_in_table, std::string& addr, std::string& addr_in_table);
	int checkResult(retStruct* ret);
}