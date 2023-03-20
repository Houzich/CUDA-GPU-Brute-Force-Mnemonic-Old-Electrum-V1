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
//compute_86, sm_86

#define NUM_CHILDS					(10)
#define NUM_ALL_CHILDS				(NUM_CHILDS*2)


//#define _CRT_SECURE_NO_WARNINGS
//#define TEST_MODE



#define WORDS_MNEMONIC						(12)
#define SIZE_WORDS_IDX_FRAME				(WORDS_MNEMONIC * sizeof(unsigned short))
#define SIZE_MNEMONIC_FRAME					(156)
#define SIZE_HASH160_FRAME					(20)
#define SIZE16_WORDS_IDX_FRAME				(SIZE_WORDS_IDX_FRAME / 2)
#define SIZE32_MNEMONIC_FRAME				(SIZE_MNEMONIC_FRAME / 4)
#define SIZE32_HASH160_FRAME			    (SIZE_HASH160_FRAME / 4)
#define SIZE64_MNEMONIC_FRAME				(SIZE_MNEMONIC_FRAME / 8)

#define USE_REVERSE_32

#define FILE_PATH_RESULT "Save_Addresses.csv"
#define FILE_PATH_FOUND_ADDRESSES "Found_Addresses.csv"
#define FILE_PATH_FOUND_BYTES "Found_Bytes.csv"


/* Four of six logical functions used in SHA-384 and SHA-512: */
#define REVERSE32_FOR_HASH(w,x)	{ \
	uint32_t tmp = (w); \
	tmp = (tmp >> 16) | (tmp << 16); \
	(x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
}

struct tableStruct {
	unsigned int* table;
	unsigned int size;
};
#pragma pack(push, 1)
struct retStruct {
	unsigned int flag_found;
	unsigned int flag_found_bytes;
	unsigned int hash160[SIZE32_HASH160_FRAME];
	unsigned int hash160_bytes[SIZE32_HASH160_FRAME];
	unsigned int hash160_bytes_from_table[SIZE32_HASH160_FRAME];
	unsigned short words_index[SIZE16_WORDS_IDX_FRAME];
	unsigned short words_index_bytes[SIZE16_WORDS_IDX_FRAME];
};
#pragma pack(pop)