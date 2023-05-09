/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V2.0.0
  * @date		9-May-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */
#pragma once

#include <stdint.h>
#include "stdafx.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

__global__ void gl_bruteforce_mnemonic(
	const uint16_t* __restrict__ words_index,
	const tableStruct* __restrict__ tables,
	retStruct* __restrict__ ret
);

__global__ void gl_bruteforce_mnemonic_for_save(
	const uint16_t* __restrict__ words_index,
	const tableStruct* __restrict__ tables,
	retStruct* __restrict__ ret,
	uint8_t* __restrict__ mnemonic_ret,
	uint32_t* __restrict__ hash160_ret
);


extern __constant__ uint32_t dev_num_bytes_find[];
extern __constant__ uint32_t dev_generate_path[];
extern __constant__ uint32_t dev_num_childs[];
extern __constant__ uint32_t dev_num_paths[];
extern __constant__ int16_t dev_static_words_indices[];
