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
#include <string>



struct ConfigClass
{
public:
	std::string folder_database = "";
	uint64_t cuda_grid = 0;
	uint64_t cuda_block = 0;
public:
	ConfigClass()
	{
	}
	~ConfigClass()
	{
	}
};


int parse_config(ConfigClass* config, std::string path);

