#include <oxyale.h>
#include <palreg.h>

uint32_t PAL_REG_CRC_MASK_[] = {
    0xebe19b94, 0x7604de74, 0xe3f9d651, 0x604fd612, 0xe8897c2c, 0xadc40920, 0x37ecdfb7, 0x334989ed,
    0x2834c33b, 0x8bd2fe15, 0xcbf001a7, 0xbd96b9d6, 0x315e2ce0, 0x4f167884, 0xa489b1b6, 0xa51c7a62,
    0x54622636, 0x0bc016fc, 0x68de2d22, 0x3c9d304c, 0x44fd06fb, 0xbbb3f772, 0xd637e099, 0x849aa9f9,
    0x5f240988, 0xf8373bb7, 0x30379087, 0xc7722864, 0xb0a2a643, 0xe3316071, 0x956fed7c, 0x966f937d,
    0x9945ae16, 0xf0b237ce, 0x223479a0, 0xd8359782, 0x05ae1b89, 0xe3653292, 0xc34eea0d, 0x2691dfc2,
    0xe9145f51, 0xd9aa7f35, 0xc7c4344e, 0x4370eba1, 0x1e43833e, 0x634bcf18, 0x0c50e26b, 0x06492118,
    0xf78b8bfe, 0x5f2bb95c, 0xa3eb54a6, 0x1e15a2f0, 0x6cc01887, 0xde4e7405, 0x1c1d7374, 0x85757feb,
    0xe372517e, 0x9b9979c7, 0xf37807e8, 0x18f97235, 0x645a149b, 0x9556c6cf, 0xf389119e, 0x1d6cbf85,
    0xa9760ce5, 0xa985c5ff, 0x5f4db574, 0x13176cac, 0x2f14aa85, 0xf520832c, 0xd21ee917, 0x6f307a5b,
    0xc1fb01c6, 0x19415378, 0x797fa2c3, 0x24f42481, 0x4f652c30, 0x39bc02ed, 0x11eda1d7, 0x8c79a136,
    0x6bd37a86, 0x80b354ee, 0xc424e066, 0xaae16427, 0x6bd3be12, 0x868d8e37, 0xd1d43c54, 0x4d62081f,
    0x433056d7, 0xf2e4cb02, 0x043fc5a2, 0x9da58ca4, 0x1ed63321, 0x20679f26, 0xb38a4758, 0x846419f7,
    0x6bdc6352, 0xabf2c24d, 0x40ac386c, 0x27588588, 0x5e1ab2e5, 0x76bdead4, 0x71444d32, 0x02fc6084,
    0x92db41fb, 0xef86baeb, 0xf7d8572a, 0xb75aeabf, 0x84dc5c93, 0xcbc13881, 0x641d6e73, 0x0cb27a99,
    0xded369a6, 0x617e5dfa, 0x248bd13e, 0xb8596d66, 0x9b36a9fa, 0x52edaf1c, 0x3c659784, 0x146df599,
    0x109fcae8, 0xc9ed4841, 0xbf593f49, 0xc94a6e73, 0x5afa0d2f, 0xb2035002, 0xcab31104, 0x7c4f5a82,
    0xeac93638, 0x63fc5385, 0xdf0cae06, 0x26e55be3, 0x2921b9b8, 0xb80b3408, 0x917e137d, 0x127a48bc,
    0xe031858a, 0x722213d7, 0x2dbc96fa, 0x5359f112, 0xab256019, 0x6e2a756e, 0x4dc62f76, 0x268832de,
    0x5980e578, 0xd338b668, 0xeee2e4d7, 0x1fff8fc6, 0x9b17ed10, 0xf3e6be0f, 0xc1ba9d78, 0xbb8693c5,
    0x24d57ec0, 0x5d640aed, 0xee87979b, 0x96323e11, 0xccbc1601, 0x0e83f43b, 0x2c2f7495, 0x5f150b2a,
    0x710a77e2, 0x281b51dc, 0x2385d03c, 0x67239bff, 0xa719e8f9, 0x21c3b9de, 0x26489c22, 0x0de68989,
    0xca758f0d, 0x417e8cd2, 0x67ed61f8, 0xd15fc001, 0x3ba2f272, 0x57e2f7a9, 0xe723b883, 0x914e43e1,
    0x71aa5b97, 0xfceb1be1, 0x7ffa4fd9, 0x67a0b494, 0x5e1c741e, 0xc8c2a5e6, 0xe13ba068, 0x24525548,
    0x397a9cf6, 0x3dddd4d6, 0xb626234c, 0x39e7b04d, 0x36ca279f, 0x89aea387, 0xcfe93789, 0x04e1761b,
    0x9d620edc, 0x6e9df1e7, 0x4a15dfa6, 0xd44641ac, 0x39796769, 0x6d062637, 0xf967af35, 0xddb4a233,
    0x48407280, 0xa9f22e7e, 0xd9878f67, 0xa05b3bc1, 0xe8c9237a, 0x81cec53e, 0x4be53e70, 0x60308e5e,
    0xf03de922, 0xa712af7b, 0xbb6168b4, 0xcc6c15b5, 0x2f202775, 0x304527e3, 0xd32bc1e6, 0xba958058,
    0xa01f7214, 0xc6e8d190, 0xab96f14b, 0x18669984, 0x4f93a385, 0x403b5b40, 0x580755f1, 0x59de50e8,
    0xf746729f, 0xff6f7d47, 0x8022ea34, 0xb24b0bcd, 0xf687a7cc, 0x7e95bab3, 0x8dc1583d, 0x0b443fe9,
    0xe6e45618, 0x224d746f, 0xf30624bb, 0xb7427258, 0xc78e19bf, 0xd1ee98a6, 0x66be7d3a, 0x791e342f,
    0x68cbaab0, 0xbbb5355d, 0x8dda9081, 0xdc2736dc, 0x573355ad, 0xc3ffec65, 0xe97f0270, 0xc6a265e8,
    0xd9d49152, 0x4bb35bdb, 0xa1c7bbe6, 0x15a3699a, 0xe69e1eb5, 0x7cdda410, 0x488609df, 0xd19678d3
};

/* Test code: 9YAT-C8MM-GJVZL */

static inline uint32_t OXLPalRegComputeLicenseCRC(const uint32_t seed)
{
    uint32_t s, crc = PAL_REG_CRC_MAGIC;
    /*
     if ( ox_env_is_big_endian() )
	s = ox_uint32_swap_endian( seed );
    else
     */
    s = seed;
    do {
        crc = ((crc << 1) | (((crc & 0x80000000) == 0) ? 0 : 1)) ^ PAL_REG_CRC_MASK_[s & 0xff];
    } while (s >>= 8);
    return crc;
}

static inline uint32_t OXLPalRegComputeLicenseCounter(const uint32_t seed, const uint32_t crc)
{
    return (seed ^ PAL_REG_MAGIC_LONG) ^ crc;
}

void OXLPalRegGenerate(OXLPalReg *reg)
{
    uint32_t seed = (uint32_t)time(NULL);
    uint32_t crc = OXLPalRegComputeLicenseCRC(seed);
    reg->crc = crc;
    reg->counter = OXLPalRegComputeLicenseCounter(seed, crc);
}

int32_t OXLPalRegTrimStringLen(const char *str)
{
    size_t i, j, len = strnlen(str, PAL_REG_STR_LEN);
    uint32_t k = 0;
    for (i = len; i--; )
	for (j = PAL_REG_TO_ASCII_LUT_LEN; j--; )
            k += (str[i] == PAL_REG_UPPER_CODE_TO_ASCII_LUT[j] |
                  str[i] == PAL_REG_LOWER_CODE_TO_ASCII_LUT[j]);
    return k;
}

void OXLPalRegTrimString(const char *regString, char *trimmedRegString)
{
    size_t i, j, k = 0, z = strnlen(regString, PAL_REG_STR_LEN);
    int8_t isValid;
    for (i = 0; i < z; ++i) {
        isValid = 0;
        for (j = PAL_REG_TO_ASCII_LUT_LEN; j--; ) {
            isValid += (regString[i] == PAL_REG_UPPER_CODE_TO_ASCII_LUT[j] |
                        regString[i] == PAL_REG_LOWER_CODE_TO_ASCII_LUT[j]);
        }
        if (isValid) {
            trimmedRegString[k++] = regString[i];
        }
    } trimmedRegString[k] = '\0';
}

static inline uint32_t OXLPalRegConvertASCIIToCode(const char ch)
{
    uint32_t code = -1;
    uint32_t i;
    for (i = PAL_REG_TO_ASCII_LUT_LEN; i--; ) {
        if (PAL_REG_UPPER_CODE_TO_ASCII_LUT[i] == ch ||
            PAL_REG_LOWER_CODE_TO_ASCII_LUT[i] == ch) {
            code = i;
        }
    }
    return code;
}

void OXLPalRegStringToReg(const char *trimmedRegString, OXLPalReg *reg)
{
    size_t pos = 0, ocnt = 0, nbits = 64, i = 0;
    int16_t sn = 0, mask = 0x0080;
    int32_t si[2];
    byte *sip = ( byte * )&si;
    const char *t = trimmedRegString;
    while (nbits--) {
        if (ocnt == 0) {
            sn = OXLPalRegConvertASCIIToCode(t[i++]);
            ocnt = 5;
        }
        if (sn & 0x10) {
            sip[pos] = (byte)(sip[pos] | mask);
        }
        sn <<= 1;
        --ocnt;
        mask >>= 1;
        if (mask == 0) {
            mask = 0x80;
            sip[++pos] = 0;
        }
    }
    reg->crc = si[0];
    reg->counter = si[1];
}

void OXLPalPrintReg(const OXLPalReg *regcode)
{
    fprintf(stderr, "regcode->counter: %u\n", regcode->counter);
    fprintf(stderr, "regcode->CRC: %u\n", regcode->crc);
}
