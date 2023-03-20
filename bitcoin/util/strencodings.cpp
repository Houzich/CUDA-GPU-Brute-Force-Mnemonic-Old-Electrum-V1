// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <main.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <tinyformat.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
    CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
    CHARS_ALPHA_NUM + "!*'();:@&=+$,/?#[]-_.~%", // SAFE_CHARS_URI
};

std::string SanitizeString(const std::string& str, int rule)
{
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++)
    {
        if (SAFE_CHARS[rule].find(str[i]) != std::string::npos)
            strResult.push_back(str[i]);
    }
    return strResult;
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char HexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}

bool IsHex(const std::string& str)
{
    for(std::string::const_iterator it(str.begin()); it != str.end(); ++it)
    {
        if (HexDigit(*it) < 0)
            return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}

bool IsHexNumber(const std::string& str)
{
    size_t starting_location = 0;
    if (str.size() > 2 && *str.begin() == '0' && *(str.begin()+1) == 'x') {
        starting_location = 2;
    }
    for (const char c : str.substr(starting_location)) {
        if (HexDigit(c) < 0) return false;
    }
    // Return false for empty string or "0x".
    return (str.size() > starting_location);
}

std::vector<unsigned char> ParseHex(const char* psz)
{
    // convert hex dump to vector
    std::vector<unsigned char> vch;
    while (true)
    {
        while (IsSpace(*psz))
            psz++;
        signed char c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        auto n{uint8_t(c << 4)};
        c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

std::vector<unsigned char> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}

void SplitHostPort(std::string in, uint16_t& portOut, std::string& hostOut)
{
    size_t colon = in.find_last_of(':');
    // if a : is found, and it either follows a [...], or no other : is in the string, treat it as port separator
    bool fHaveColon = colon != in.npos;
    bool fBracketed = fHaveColon && (in[0] == '[' && in[colon - 1] == ']'); // if there is a colon, and in[0]=='[', colon is not 0, so in[colon-1] is safe
    bool fMultiColon{fHaveColon && colon != 0 && (in.find_last_of(':', colon - 1) != in.npos)};
    if (fHaveColon && (colon == 0 || fBracketed || !fMultiColon)) {
        uint16_t n;
        if (ParseUInt16(in.substr(colon + 1), &n)) {
            in = in.substr(0, colon);
            portOut = n;
        }
    }
    if (in.size() > 0 && in[0] == '[' && in[in.size() - 1] == ']') {
        hostOut = in.substr(1, in.size() - 2);
    } else {
        hostOut = in;
    }
}

std::string EncodeBase64(Span<const unsigned char> input)
{
    static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string str;
    str.reserve(((input.size() + 2) / 3) * 4);
    ConvertBits<8, 6, true>([&](int v) { str += pbase64[v]; }, input.begin(), input.end());
    while (str.size() % 4) str += '=';
    return str;
}

std::vector<unsigned char> DecodeBase64(const char* p, bool* pf_invalid)
{
    static const int8_t decode64_table[256]{
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    const char* e = p;
    std::vector<uint8_t> val;
    val.reserve(strlen(p));
    while (*p != 0) {
        int x = decode64_table[(unsigned char)*p];
        if (x == -1) break;
        val.push_back(uint8_t(x));
        ++p;
    }

    std::vector<unsigned char> ret;
    ret.reserve((val.size() * 3) / 4);
    bool valid = ConvertBits<6, 8, false>([&](unsigned char c) { ret.push_back(c); }, val.begin(), val.end());

    const char* q = p;
    while (valid && *p != 0) {
        if (*p != '=') {
            valid = false;
            break;
        }
        ++p;
    }
    valid = valid && (p - e) % 4 == 0 && p - q < 4;
    if (pf_invalid) *pf_invalid = !valid;

    return ret;
}

std::string DecodeBase64(const std::string& str, bool* pf_invalid)
{
    if (!ValidAsCString(str)) {
        if (pf_invalid) {
            *pf_invalid = true;
        }
        return {};
    }
    std::vector<unsigned char> vchRet = DecodeBase64(str.c_str(), pf_invalid);
    return std::string((const char*)vchRet.data(), vchRet.size());
}

std::string EncodeBase32(Span<const unsigned char> input, bool pad)
{
    static const char *pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

    std::string str;
    str.reserve(((input.size() + 4) / 5) * 8);
    ConvertBits<8, 5, true>([&](int v) { str += pbase32[v]; }, input.begin(), input.end());
    if (pad) {
        while (str.size() % 8) {
            str += '=';
        }
    }
    return str;
}

std::string EncodeBase32(const std::string& str, bool pad)
{
    return EncodeBase32(MakeUCharSpan(str), pad);
}

std::vector<unsigned char> DecodeBase32(const char* p, bool* pf_invalid)
{
    static const int8_t decode32_table[256]{
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1,  0,  1,  2,
         3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    const char* e = p;
    std::vector<uint8_t> val;
    val.reserve(strlen(p));
    while (*p != 0) {
        int x = decode32_table[(unsigned char)*p];
        if (x == -1) break;
        val.push_back(uint8_t(x));
        ++p;
    }

    std::vector<unsigned char> ret;
    ret.reserve((val.size() * 5) / 8);
    bool valid = ConvertBits<5, 8, false>([&](unsigned char c) { ret.push_back(c); }, val.begin(), val.end());

    const char* q = p;
    while (valid && *p != 0) {
        if (*p != '=') {
            valid = false;
            break;
        }
        ++p;
    }
    valid = valid && (p - e) % 8 == 0 && p - q < 8;
    if (pf_invalid) *pf_invalid = !valid;

    return ret;
}

std::string DecodeBase32(const std::string& str, bool* pf_invalid)
{
    if (!ValidAsCString(str)) {
        if (pf_invalid) {
            *pf_invalid = true;
        }
        return {};
    }
    std::vector<unsigned char> vchRet = DecodeBase32(str.c_str(), pf_invalid);
    return std::string((const char*)vchRet.data(), vchRet.size());
}

namespace {
template <typename T>
bool ParseIntegral(const std::string& str, T* out)
{
    static_assert(std::is_integral<T>::value);
    // Replicate the exact behavior of strtol/strtoll/strtoul/strtoull when
    // handling leading +/- for backwards compatibility.
    if (str.length() >= 2 && str[0] == '+' && str[1] == '-') {
        return false;
    }
    const std::optional<T> opt_int = ToIntegral<T>((!str.empty() && str[0] == '+') ? str.substr(1) : str);
    if (!opt_int) {
        return false;
    }
    if (out != nullptr) {
        *out = *opt_int;
    }
    return true;
}
}; // namespace

bool ParseInt32(const std::string& str, int32_t* out)
{
    return ParseIntegral<int32_t>(str, out);
}

bool ParseInt64(const std::string& str, int64_t* out)
{
    return ParseIntegral<int64_t>(str, out);
}

bool ParseUInt8(const std::string& str, uint8_t* out)
{
    return ParseIntegral<uint8_t>(str, out);
}

bool ParseUInt16(const std::string& str, uint16_t* out)
{
    return ParseIntegral<uint16_t>(str, out);
}

bool ParseUInt32(const std::string& str, uint32_t* out)
{
    return ParseIntegral<uint32_t>(str, out);
}

bool ParseUInt64(const std::string& str, uint64_t* out)
{
    return ParseIntegral<uint64_t>(str, out);
}

std::string FormatParagraph(const std::string& in, size_t width, size_t indent)
{
    assert(width >= indent);
    std::stringstream out;
    size_t ptr = 0;
    size_t indented = 0;
    while (ptr < in.size())
    {
        size_t lineend = in.find_first_of('\n', ptr);
        if (lineend == std::string::npos) {
            lineend = in.size();
        }
        const size_t linelen = lineend - ptr;
        const size_t rem_width = width - indented;
        if (linelen <= rem_width) {
            out << in.substr(ptr, linelen + 1);
            ptr = lineend + 1;
            indented = 0;
        } else {
            size_t finalspace = in.find_last_of(" \n", ptr + rem_width);
            if (finalspace == std::string::npos || finalspace < ptr) {
                // No place to break; just include the entire word and move on
                finalspace = in.find_first_of("\n ", ptr);
                if (finalspace == std::string::npos) {
                    // End of the string, just add it and break
                    out << in.substr(ptr);
                    break;
                }
            }
            out << in.substr(ptr, finalspace - ptr) << "\n";
            if (in[finalspace] == '\n') {
                indented = 0;
            } else if (indent) {
                out << std::string(indent, ' ');
                indented = indent;
            }
            ptr = finalspace + 1;
        }
    }
    return out.str();
}

/** Upper bound for mantissa.
 * 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
 * Larger integers cannot consist of arbitrary combinations of 0-9:
 *
 *   999999999999999999  1^18-1
 *  9223372036854775807  (1<<63)-1  (max int64_t)
 *  9999999999999999999  1^19-1     (would overflow)
 */
static const int64_t UPPER_BOUND = 1000000000000000000LL - 1LL;

/** Helper function for ParseFixedPoint */
static inline bool ProcessMantissaDigit(char ch, int64_t &mantissa, int &mantissa_tzeros)
{
    if(ch == '0')
        ++mantissa_tzeros;
    else {
        for (int i=0; i<=mantissa_tzeros; ++i) {
            if (mantissa > (UPPER_BOUND / 10LL))
                return false; /* overflow */
            mantissa *= 10;
        }
        mantissa += ch - '0';
        mantissa_tzeros = 0;
    }
    return true;
}

bool ParseFixedPoint(const std::string &val, int decimals, int64_t *amount_out)
{
    int64_t mantissa = 0;
    int64_t exponent = 0;
    int mantissa_tzeros = 0;
    bool mantissa_sign = false;
    bool exponent_sign = false;
    int ptr = 0;
    int end = (int)val.size();
    int point_ofs = 0;

    if (ptr < end && val[ptr] == '-') {
        mantissa_sign = true;
        ++ptr;
    }
    if (ptr < end)
    {
        if (val[ptr] == '0') {
            /* pass single 0 */
            ++ptr;
        } else if (val[ptr] >= '1' && val[ptr] <= '9') {
            while (ptr < end && IsDigit(val[ptr])) {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
            }
        } else return false; /* missing expected digit */
    } else return false; /* empty string or loose '-' */
    if (ptr < end && val[ptr] == '.')
    {
        ++ptr;
        if (ptr < end && IsDigit(val[ptr]))
        {
            while (ptr < end && IsDigit(val[ptr])) {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
                ++point_ofs;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr < end && (val[ptr] == 'e' || val[ptr] == 'E'))
    {
        ++ptr;
        if (ptr < end && val[ptr] == '+')
            ++ptr;
        else if (ptr < end && val[ptr] == '-') {
            exponent_sign = true;
            ++ptr;
        }
        if (ptr < end && IsDigit(val[ptr])) {
            while (ptr < end && IsDigit(val[ptr])) {
                if (exponent > (UPPER_BOUND / 10LL))
                    return false; /* overflow */
                exponent = exponent * 10 + val[ptr] - '0';
                ++ptr;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr != end)
        return false; /* trailing garbage */

    /* finalize exponent */
    if (exponent_sign)
        exponent = -exponent;
    exponent = exponent - point_ofs + mantissa_tzeros;

    /* finalize mantissa */
    if (mantissa_sign)
        mantissa = -mantissa;

    /* convert to one 64-bit fixed-point value */
    exponent += decimals;
    if (exponent < 0)
        return false; /* cannot represent values smaller than 10^-decimals */
    if (exponent >= 18)
        return false; /* cannot represent values larger than or equal to 10^(18-decimals) */

    for (int i=0; i < exponent; ++i) {
        if (mantissa > (UPPER_BOUND / 10LL) || mantissa < -(UPPER_BOUND / 10LL))
            return false; /* overflow */
        mantissa *= 10;
    }
    if (mantissa > UPPER_BOUND || mantissa < -UPPER_BOUND)
        return false; /* overflow */

    if (amount_out)
        *amount_out = mantissa;

    return true;
}

std::string ToLower(const std::string& str)
{
    std::string r;
    for (auto ch : str) r += ToLower(ch);
    return r;
}

std::string ToUpper(const std::string& str)
{
    std::string r;
    for (auto ch : str) r += ToUpper(ch);
    return r;
}

std::string Capitalize(std::string str)
{
    if (str.empty()) return str;
    str[0] = ToUpper(str.front());
    return str;
}

std::string HexStr(const Span<const uint8_t> s)
{
    std::string rv(s.size() * 2, '\0');
    static constexpr char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    auto it = rv.begin();
    for (uint8_t v : s) {
        *it++ = hexmap[v >> 4];
        *it++ = hexmap[v & 15];
    }
    assert(it == rv.end());
    return rv;
}

std::optional<uint64_t> ParseByteUnits(const std::string& str, ByteUnit default_multiplier)
{
    if (str.empty()) {
        return std::nullopt;
    }
    auto multiplier = default_multiplier;
    char unit = str.back();
    switch (unit) {
    case 'k':
        multiplier = ByteUnit::k;
        break;
    case 'K':
        multiplier = ByteUnit::K;
        break;
    case 'm':
        multiplier = ByteUnit::m;
        break;
    case 'M':
        multiplier = ByteUnit::M;
        break;
    case 'g':
        multiplier = ByteUnit::g;
        break;
    case 'G':
        multiplier = ByteUnit::G;
        break;
    case 't':
        multiplier = ByteUnit::t;
        break;
    case 'T':
        multiplier = ByteUnit::T;
        break;
    default:
        unit = 0;
        break;
    }

    uint64_t unit_amount = static_cast<uint64_t>(multiplier);
    auto parsed_num = ToIntegral<uint64_t>(unit ? str.substr(0, str.size() - 1) : str);
    if (!parsed_num || parsed_num > std::numeric_limits<uint64_t>::max() / unit_amount) { // check overflow
        return std::nullopt;
    }
    return *parsed_num * unit_amount;
}
