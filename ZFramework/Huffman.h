#pragma once

/* Header files */

#include <stdint.h>


/* Node identifiers, might change to enumeration */

#define INTERNAL_NODE 0
#define BYTE_NODE 1

#define HEADER_BASE_SIZE 10 /* Size of the header with no bytes stored */

#define MAX_CODE_LEN 16 /* The longest any encoded representation is allowed to be */

/* Huffman Tree node */

typedef struct huffman_node_t {
    size_t freq;
    union {
        struct huffman_node_t* child[2];
        uint8_t c;
    };
} huffman_node_t;

/* Lookup table used for encoding and decoding */

typedef struct huffman_coding_table_t {
    union {
        uint16_t code;
        uint8_t symbol;
    };
    uint8_t length;
} huffman_coding_table_t;

/* Interface Functions */

bool huffman_decode(uint8_t* input, uint8_t** output, uint32_t& decompressed_output_length);
bool huffman_encode(uint8_t* input, uint8_t** output, uint32_t decompressed_length, uint32_t& compressed_output_length);

/* Internal Decoding Functions */

uint16_t peek_buffer(uint8_t* input, size_t bit_pos);

/* Internal Encoding Functions */

int create_huffman_tree(size_t* freq, huffman_node_t** head_node);
int node_compare(const void* first_node, const void* second_node);
huffman_node_t* create_byte_node(uint8_t c, size_t freq);
huffman_node_t* create_internal_node(huffman_node_t* first_child, huffman_node_t* second_child);
void create_encoding_table(huffman_node_t* node, huffman_coding_table_t huffman_array[256], uint8_t bits_set);
void destroy_huffman_tree(huffman_node_t* node);
void write_k_bits(uint8_t* buffer, uint16_t value, size_t* byte_pos, uint8_t bits);

