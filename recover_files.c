// Author - Billy Godoy
// Date - 12/11/25

// Description - This file scans a QSF disk image and recovers JPG files 
// by searching raw data blocks for JPG start and end signatures. 
// the recovered files are written to the current directory and are written as "recovred_file_N.jpg".


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "qfs.h"


static int is_jpg_start(uint8_t a, uint8_t b) {
    return (a == 0xFF && b == 0xD8);
}

static int is_jpg_end(uint8_t a, uint8_t b) {
    return (a == 0xFF && b == 0xD9);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filesystem_image>\n", argv[0]);
        return 1;
    }

    const char *image = argv[1];

    FILE *fp = fopen(image, "rb");
    if (!fp) {
        perror("fopen");
        return 2;
    }

    // reads the superblock to get block size and number of blocks
    superblock_t sb;
    if (fread(&sb, sizeof(sb), 1, fp) != 1) {
        fprintf(stderr, "Failed to read superblock\n");
        fclose(fp);
        return 3;
    }

    uint16_t block_size = sb.bytes_per_block;

    // data blocks begin right after the superblock and directory table
    long data_offset = sizeof(superblock_t) +
                       (sb.total_direntries * sizeof(direntry_t));

    // buffer to hold one full block at a atime while scanning
    uint8_t *buffer = malloc(block_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return 4;
    }

    // In QFS each block has a 1-btye status at the start and then a 2-byte 
    // pointer at the end
    //so the "real data" area is in the middle
    uint32_t payload_start = 1;
    uint32_t payload_len = block_size - 3;

    int recovered_count = 0;

    // scan every block ont he disk for the JPG signituares 
    for (uint16_t b = 0; b < sb.total_blocks; b++) {

        long block_offset = data_offset + (long)b * block_size;
        fseek(fp, block_offset, SEEK_SET);

        fread(buffer, 1, block_size, fp);

        for (uint32_t i = 0; i + 1 < payload_len; i++) {

            if (!is_jpg_start(buffer[payload_start + i],
                              buffer[payload_start + i + 1])) {
                continue;
            }

            char outname[64];
            sprintf(outname, "recovered_file_%d.jpg", recovered_count);

            FILE *out = fopen(outname, "wb");
            if (!out) {
                perror("fopen output");
                free(buffer);
                fclose(fp);
                return 5;
            }

            int found_end = 0;

            uint8_t prev = 0;

            int has_prev = 0;

            // keep wiritng bytes until we hit FF D9 when we find a JPG start
            for (uint16_t cur_block = b;
                 cur_block < sb.total_blocks && !found_end;
                 cur_block++) {

                if (cur_block != b) {
                    long off = data_offset + (long)cur_block * block_size;
                    fseek(fp, off, SEEK_SET);
                    fread(buffer, 1, block_size, fp);
                }

                uint32_t start_index = (cur_block == b) ? i : 0;

                for (uint32_t j = start_index; j < payload_len; j++) {
                    uint8_t byte = buffer[payload_start + j];
                    fputc(byte, out);

                    if (has_prev && is_jpg_end(prev, byte)) {
                        found_end = 1;
                        b = cur_block;
                        break;
                    }

                    prev = byte;

                    has_prev = 1;
                }
            }

            // recover file is done now move to the next one
            fclose(out);

            recovered_count++;

            // continuing scanning after this recovered file
            break;
        }
    }

    free(buffer);

    fclose(fp);
    return 0;
}
