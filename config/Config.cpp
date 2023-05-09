/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V2.0.0
  * @date		9-May-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */
#include "Config.hpp"
#include <tao/config.hpp>


int check_config(ConfigClass& config)
{
	int num_paths = 0;
	if (config.path_m0_x == "yes") {
		num_paths++;
		config.generate_path[0] = 1;
	}
	else if (config.path_m0_x != "no") {
		std::cerr << "Error parse path_m0_x. Please write \"yes\" or \"no\"" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
	if (config.path_m1_x == "yes") {
		num_paths++;
		config.generate_path[1] = 1;
	}
	else if (config.path_m1_x != "no") {
		std::cerr << "Error parse path_m1_x. Please write \"yes\" or \"no\"" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
	if (config.chech_equal_bytes_in_adresses == "yes") {
	}
	else if (config.chech_equal_bytes_in_adresses != "no") {
		std::cerr << "Error parse chech_equal_bytes_in_adresses. Please write \"yes\" or \"no\"" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
	if (config.save_generation_result_in_file == "yes") {
	}
	else if (config.save_generation_result_in_file != "no") {
		std::cerr << "Error parse save_generation_result_in_file. Please write \"yes\" or \"no\"" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}


	if (config.num_child_addresses > 10)
	{
		std::cerr << "Error \"num_child_addresses\". Please enter a number less than 10" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
	if (config.number_of_generated_mnemonics > 18000000000000000000)
	{
		std::cerr << "Error number_of_generated_mnemonics. Please enter a number less than 18,000,000,000,000,000,000" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}

	config.num_paths = num_paths;

	return 0;
}

int parse_config(ConfigClass* config, std::string path)
{
	try {
		const tao::config::value v = tao::config::from_file(path);

		config->folder_tables = access(v, tao::config::key("folder_tables")).get_string();

		config->number_of_generated_mnemonics = access(v, tao::config::key("number_of_generated_mnemonics")).get_unsigned();
		config->num_child_addresses = access(v, tao::config::key("num_child_addresses")).get_unsigned();

		config->path_m0_x = access(v, tao::config::key("path_m0_x")).get_string();
		config->path_m1_x = access(v, tao::config::key("path_m1_x")).get_string();

		config->static_words_generate_mnemonic = access(v, tao::config::key("static_words_generate_mnemonic")).get_string();
		config->chech_equal_bytes_in_adresses = access(v, tao::config::key("chech_equal_bytes_in_adresses")).get_string();
		config->save_generation_result_in_file = access(v, tao::config::key("save_generation_result_in_file")).get_string();

		config->cuda_grid = access(v, tao::config::key("cuda_grid")).get_unsigned();
		config->cuda_block = access(v, tao::config::key("cuda_block")).get_unsigned();

		return check_config(*config);
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error parse config.cfg file " << path << " : " << e.what() << '\n';
		throw std::logic_error("error parse config.cfg file");
	}
	catch (...) {
		std::cerr << "Error parse config.cfg file" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
	return 0;
}


