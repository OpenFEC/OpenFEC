
#define OF_USE_ENCODER
#define OF_USE_DECODER
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/rc4.h>

#include "../src/lib_common/of_openfec_api.h"
#include "../src/lib_common/of_debug.h"

#define CODEC_ID OF_CODEC_LDPC_STAIRCASE_STABLE

#define BUFFER_LENGTH (1<<20)
#define SYMBOL_TABLE_MAX_SIZE (1<<28)

void prng_fill(uint64_t seed, unsigned char *buffer, size_t length) {
    static RC4_KEY key;
    static bool setup = false;
    
    if (!setup) {
        setup = true;
        memset(&key, 0, sizeof(key));
        RC4_set_key(&key, sizeof(seed), (const unsigned char*)&seed);
    }
    
    int i;
    const unsigned char null_byte[] = {0x00};
    for (i=0; i<length;++i) {
        RC4(&key, 1, null_byte, (unsigned char *)buffer+i);
    }
}

int main(int argc, char **argv) {
    int ret;
    char *buffer;
    size_t length=0;
    
    uint32_t symbol_length;
    uint32_t source_symbols;
    uint32_t repair_symbols;
    uint64_t seed;
    uint8_t n1;
    
    char **symbol_table;
    
    int i;
    
    int offset=0;
    
    size_t header_size = sizeof(symbol_length) + sizeof(source_symbols) + sizeof(repair_symbols) + sizeof(seed) + sizeof(n1);
    
    buffer = malloc(header_size+1);
    if (buffer == NULL)
        return 0;
    
    while((ret = read(STDIN_FILENO, buffer+length, 1)) > 0) {
        length += ret;
        
        if (length > header_size) {
            return 0;
        }
    }
    
    if (length != header_size)
        return 0;
    
    memcpy(&symbol_length, buffer+offset, sizeof(symbol_length));
    offset += sizeof(symbol_length);
    
    memcpy(&source_symbols, buffer+offset, sizeof(source_symbols));
    offset += sizeof(source_symbols);
    
    memcpy(&repair_symbols, buffer+offset, sizeof(repair_symbols));
    offset += sizeof(repair_symbols);
    
    memcpy(&seed, buffer+offset, sizeof(seed));
    offset += sizeof(seed);
    
    memcpy(&n1, buffer+offset, sizeof(n1));
    offset += sizeof(n1);
    
    if (symbol_length > (SYMBOL_TABLE_MAX_SIZE) || source_symbols > (SYMBOL_TABLE_MAX_SIZE) || symbol_length * source_symbols > (SYMBOL_TABLE_MAX_SIZE))
        return 0;
    
    if (symbol_length > (SYMBOL_TABLE_MAX_SIZE) || repair_symbols > (SYMBOL_TABLE_MAX_SIZE) || symbol_length * repair_symbols > (SYMBOL_TABLE_MAX_SIZE))
        return 0;
    
    of_session_t *session;
    
    of_create_codec_instance(&session, CODEC_ID, OF_ENCODER_AND_DECODER, 2);
    
    of_ldpc_parameters_t ldpc_parameters;
    memset(&ldpc_parameters, 0, sizeof(ldpc_parameters));
    
    prng_fill(seed, (unsigned char *)&ldpc_parameters.prng_seed, sizeof(ldpc_parameters.prng_seed));
    ldpc_parameters.N1 = n1;
    ldpc_parameters.nb_source_symbols = source_symbols;
    ldpc_parameters.nb_repair_symbols = repair_symbols;
    ldpc_parameters.encoding_symbol_length = symbol_length;
    
    if (of_set_fec_parameters(session, (of_parameters_t*)&ldpc_parameters) != OF_STATUS_OK)
        return 0;
    
    symbol_table = calloc(source_symbols + repair_symbols, sizeof(char*));
    
    if (symbol_table == NULL)
        return 0;
    
    for (i = 0; i < source_symbols; ++i) {
        symbol_table[i] = malloc(symbol_length);
        if (symbol_table[i] == NULL)
            return 0;
        prng_fill(seed, (unsigned char *)symbol_table[i], symbol_length);
    }
    
    for (i = source_symbols; i < source_symbols + repair_symbols; ++i)
    {
        symbol_table[i] = malloc(symbol_length);
        if (symbol_table[i] == NULL)
            return 0;
        memset(symbol_table[i], 0, symbol_length);
        if (of_build_repair_symbol(session, (void**)symbol_table, i) != OF_STATUS_OK)
            return 0;
    }
    
    if (of_release_codec_instance(session) != OF_STATUS_OK)
        return 0;
    
    return 0;
}
