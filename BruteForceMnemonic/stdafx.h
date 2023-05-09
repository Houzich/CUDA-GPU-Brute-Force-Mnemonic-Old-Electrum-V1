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
#include <cstddef>
//compute_86, sm_86


//#define _CRT_SECURE_NO_WARNINGS
//#define TEST_MODE



#define NUM_WORDS_MNEMONIC					(12)
#define NUM_WORDS_MNEMONIC_FRAME			(111)
#define SIZE_WORDS_IDX_BUFF					(NUM_WORDS_MNEMONIC * sizeof(unsigned short) * NUM_WORDS_MNEMONIC_FRAME)
#define SIZE_WORDS_IDX_FRAME				(NUM_WORDS_MNEMONIC * sizeof(unsigned short))
#define SIZE_MNEMONIC_FRAME					(156)
#define SIZE_HASH160_FRAME					(20)
#define SIZE16_WORDS_IDX_FRAME				(SIZE_WORDS_IDX_FRAME / 2)
#define SIZE32_MNEMONIC_FRAME				(SIZE_MNEMONIC_FRAME / 4)
#define SIZE32_HASH160_FRAME			    (SIZE_HASH160_FRAME / 4)
#define SIZE64_MNEMONIC_FRAME				(SIZE_MNEMONIC_FRAME / 8)


#define NUM_PACKETS_SAVE_IN_FILE 8
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
	unsigned int* table = NULL;
	unsigned int size = 0;
};
#define MAX_FOUND_ADDRESSES 5


#pragma pack(push, 1)
struct foundInfoStruct {
	unsigned int hash160[SIZE32_HASH160_FRAME];
	unsigned int path;
	unsigned int child;
	unsigned short words_idx[NUM_WORDS_MNEMONIC];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct foundBytesInfoStruct {
	unsigned int hash160[SIZE32_HASH160_FRAME];
	unsigned int hash160_from_table[SIZE32_HASH160_FRAME];
	unsigned int path;
	unsigned int child;
	unsigned short words_idx[NUM_WORDS_MNEMONIC];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct foundStruct {
	foundInfoStruct found_info[MAX_FOUND_ADDRESSES];
	foundBytesInfoStruct found_bytes_info[MAX_FOUND_ADDRESSES];
	unsigned int count_found;
	unsigned int count_found_bytes;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct retStruct {
	foundStruct f[1];
};
#pragma pack(pop)