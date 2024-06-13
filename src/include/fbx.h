#pragma once

#include <stdint.h>
#include <zlib.h>

static const uint32_t K3_FBX_KEYWORD_LEN = 21;
static const char* K3_FBX_KEYWORD = "Kaydara FBX Binary  ";
static const uint16_t K3_FBX_KEYWORD_TAIL = 0x001A;

static const uint32_t K3_FBX_NODE_RECORD_LENGTH = 13;
struct k3fbxNodeRecord {
    uint32_t end_offset;
    uint32_t num_properties;
    uint32_t property_list_len;
    uint8_t name_len;
};

static const char K3_FBX_TYPECODE_SINT16 = 'Y';
static const char K3_FBX_TYPECODE_BOOL = 'C';
static const char K3_FBX_TYPECODE_SINT32 = 'I';
static const char K3_FBX_TYPECODE_FLOAT = 'F';
static const char K3_FBX_TYPECODE_DOUBLE = 'D';
static const char K3_FBX_TYPECODE_SINT64 = 'L';
static const char K3_FBX_TYPECODE_FLOAT_ARRAY = 'f';
static const char K3_FBX_TYPECODE_DOUBLE_ARRAY = 'd';
static const char K3_FBX_TYPECODE_SINT64_ARRAY = 'l';
static const char K3_FBX_TYPECODE_SINT32_ARRAY = 'i';
static const char K3_FBX_TYPECODE_BOOL_ARRAY = 'b';
static const char K3_FBX_TYPECODE_STRING = 'S';
static const char K3_FBX_TYPECODE_RAW = 'R';

static const uint64_t K3_FBX_TICKS_PER_MSEC = 46186158ULL;
static const uint32_t K3_FBX_MAX_ANIM_NAME_LENGTH = 64;

static const uint32_t K3_FBX_ARRAY_PROPERTY_LENGTH = 12;
struct k3fbxArrayProperty {
    uint32_t array_length;
    uint32_t encoding;
    uint32_t compressed_length;
};