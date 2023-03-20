/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V1.0.0
  * @date		20-March-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */
#include "Config.hpp"
#include <tao/config.hpp>

int parse_config(ConfigClass* config, std::string path)
{
	try {
		const tao::config::value v = tao::config::from_file(path);

		config->folder_database = access(v, tao::config::key("folder_database")).get_string();
		config->cuda_grid = access(v, tao::config::key("cuda_grid")).get_unsigned();
		config->cuda_block = access(v, tao::config::key("cuda_block")).get_unsigned();
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error parse config.cfg file " << path << " : " << e.what() << '\n';
		throw std::logic_error("error parse config.cfg file");
	}
	catch (...) {
		std::cerr << "Error parse config.cfg file, unknown exception occured" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
	return 0;
}


