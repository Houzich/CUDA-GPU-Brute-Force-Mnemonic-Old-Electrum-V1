/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V1.0.0
  * @date		20-March-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
 //
#include "../BruteForceMnemonic/stdafx.h"
#include "segwit_addr.h"
namespace tools {
    uint32_t bech32_polymod_step(uint32_t pre) {
        uint8_t b = pre >> 25;
        return ((pre & 0x1FFFFFF) << 5) ^
            (-((b >> 0) & 1) & 0x3b6a57b2UL) ^
            (-((b >> 1) & 1) & 0x26508e6dUL) ^
            (-((b >> 2) & 1) & 0x1ea119faUL) ^
            (-((b >> 3) & 1) & 0x3d4233ddUL) ^
            (-((b >> 4) & 1) & 0x2a1462b3UL);
    }

    static const char* charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    static const int8_t charset_rev[128] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
                -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
                1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
                -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
                1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
    };

    int bech32_encode(char* output, const char* hrp, const uint8_t* data, size_t data_len) {
        uint32_t chk = 1;
        size_t i = 0;
        while (hrp[i] != 0) {
            int ch = hrp[i];
            if (ch < 33 || ch > 126) {
                return 0;
            }

            if (ch >= 'A' && ch <= 'Z') return 0;
            chk = bech32_polymod_step(chk) ^ (ch >> 5);
            ++i;
        }
        if (i + 7 + data_len > 90) return 0;
        chk = bech32_polymod_step(chk);
        while (*hrp != 0) {
            chk = bech32_polymod_step(chk) ^ (*hrp & 0x1f);
            *(output++) = *(hrp++);
        }
        *(output++) = '1';
        for (i = 0; i < data_len; ++i) {
            if (*data >> 5) return 0;
            chk = bech32_polymod_step(chk) ^ (*data);
            *(output++) = charset[*(data++)];
        }
        for (i = 0; i < 6; ++i) {
            chk = bech32_polymod_step(chk);
        }
        chk ^= 1;
        for (i = 0; i < 6; ++i) {
            *(output++) = charset[(chk >> ((5 - i) * 5)) & 0x1f];
        }
        *output = 0;
        return 1;
    }

    static int convert_bits(uint8_t* out, size_t* outlen, int outbits, const uint8_t* in, size_t inlen, int inbits, int pad) {
        uint32_t val = 0;
        int bits = 0;
        uint32_t maxv = (((uint32_t)1) << outbits) - 1;
        while (inlen--) {
            val = (val << inbits) | *(in++);
            bits += inbits;
            while (bits >= outbits) {
                bits -= outbits;
                out[(*outlen)++] = (val >> bits) & maxv;
            }
        }
        if (pad) {
            if (bits) {
                out[(*outlen)++] = (val << (outbits - bits)) & maxv;
            }
        }
        else if (((val << (outbits - bits)) & maxv) || bits >= inbits) {
            return 0;
        }
        return 1;
    }

    int segwit_addr_encode(char* output, const char* hrp, int witver, const uint8_t* witprog, size_t witprog_len) {
        uint8_t data[65] = { 0 };
        size_t datalen = 0;
        if (witver > 16) return 0;
        if (witver == 0 && witprog_len != 20 && witprog_len != 32) return 0;
        if (witprog_len < 2 || witprog_len > 40) return 0;
        data[0] = witver;
        convert_bits(data + 1, &datalen, 5, witprog, witprog_len, 8, 1);
        ++datalen;
        return bech32_encode(output, hrp, data, datalen);
    }


    int bech32_decode_nocheck(uint8_t* data, size_t* data_len, const char* input)
    {

        uint8_t acc = 0;
        uint8_t acc_len = 8;
        size_t out_len = 0;

        size_t input_len = strlen(input);
        for (int i = 0; i < input_len; i++) {

            if (input[i] & 0x80)
                return false;

            int8_t c = charset_rev[tolower(input[i])];
            if (c < 0)
                return false;

            if (acc_len >= 5) {
                acc |= c << (acc_len - 5);
                acc_len -= 5;
            }
            else {
                int shift = 5 - acc_len;
                data[out_len++] = acc | (c >> shift);
                acc_len = 8 - shift;
                acc = c << acc_len;
            }

        }

        data[out_len++] = acc;
        *data_len = out_len;

        return true;

    }

    int bech32_decode(char* hrp, uint8_t* data, size_t* data_len, const char* input)
    {
        uint32_t chk = 1;
        size_t i;
        size_t input_len = strlen(input);
        size_t hrp_len;
        int have_lower = 0, have_upper = 0;
        if (input_len < 8 || input_len > 90) {
            return 0;
        }
        *data_len = 0;
        while (*data_len < input_len && input[(input_len - 1) - *data_len] != '1') {
            ++(*data_len);
        }
        hrp_len = input_len - (1 + *data_len);
        if (1 + *data_len >= input_len || *data_len < 6) {
            return 0;
        }
        *(data_len) -= 6;
        for (i = 0; i < hrp_len; ++i) {
            int ch = input[i];
            if (ch < 33 || ch > 126) {
                return 0;
            }
            if (ch >= 'a' && ch <= 'z') {
                have_lower = 1;
            }
            else if (ch >= 'A' && ch <= 'Z') {
                have_upper = 1;
                ch = (ch - 'A') + 'a';
            }
            hrp[i] = ch;
            chk = bech32_polymod_step(chk) ^ (ch >> 5);
        }
        hrp[i] = 0;
        chk = bech32_polymod_step(chk);
        for (i = 0; i < hrp_len; ++i) {
            chk = bech32_polymod_step(chk) ^ (input[i] & 0x1f);
        }
        ++i;
        while (i < input_len) {
            int v = (input[i] & 0x80) ? -1 : charset_rev[(int)input[i]];
            if (input[i] >= 'a' && input[i] <= 'z')
                have_lower = 1;
            if (input[i] >= 'A' && input[i] <= 'Z')
                have_upper = 1;
            if (v == -1) {
                return 0;
            }
            chk = bech32_polymod_step(chk) ^ v;
            if (i + 6 < input_len) {
                data[i - (1 + hrp_len)] = v;
            }
            ++i;
        }
        if (have_lower && have_upper) {
            return 0;
        }
        return chk == 1;
    }


    int segwit_addr_decode(int* witver, uint8_t* witdata, size_t* witdata_len, const char* hrp, const char* addr)
    {
        uint8_t data[84];
        char hrp_actual[84];
        size_t data_len;
        if (!bech32_decode(hrp_actual, data, &data_len, addr))
            return 0;
        if (data_len == 0 || data_len > 65)
            return 0;
        if (strncmp(hrp, hrp_actual, 84) != 0)
            return 0;
        if (data[0] > 16)
            return 0;
        *witdata_len = 0;
        if (!convert_bits(witdata, witdata_len, 8, data + 1, data_len - 1, 5, 0))
            return 0;
        if (*witdata_len < 2 || *witdata_len > 40)
            return 0;
        if (data[0] == 0 && *witdata_len != 20 && *witdata_len != 32)
            return 0;
        *witver = data[0];
        return 1;
    }

}
