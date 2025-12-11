#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "qfs.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <disk image file> <file to read> <output file>\n", argv[0]);
        return 1;
    }
    
    const char *diskImage = argv[1];
    const char *fileName = argv[2];
    const char *curDirectory = argv[3];
   
    FILE *fp = fopen(argv[1], "rb");
    if (!fp){
        perror("fopen");
        return 2;
    }

    superblock_t sb;
    if (fread(&sb, sizeof(sb), 1, fp) != 1){
        fprintf(stderr, "Failed to read superblock from %s\n", diskImage);
        fclose(fp);
        return 3;
    }

    long dir_offset = (long)sizeof(superblock_t);
    if (fseek(fp, dir_offset, SEEK_SET) != 0){
        perror("fseek directory");
        fclose(fp);
        return 4;
    }

    direntry_t entry;
    direntry_t target_entry;
    int found = 0;

    for (uint8_t i = 0; i < sb.total_direntries; i++){
        if (fread(&entry, sizeof(entry), 1, fp) != 1){
            fprintf(stderr, "Failed to read directory entry %u\n", i);
            fclose(fp);
            return 5;
        }

        if (entry.filename[0] == '\0') continue;

        if (strncmp(entry.filename, fileName, sizeof(entry.filename)) == 0){
            target_entry = entry;
            found = 1;
            break;
        }
    }

    if (!found){
        printf("File \"%s\" not found on disk image.\n", fileName);
        fclose(fp);
        return 0;
    }

    FILE *out = fopen(curDirectory, "wb");
    if (!out){
        perror("fopen output");
        fclose(fp);
        return 6;
    }

    uint16_t block_size = sb.bytes_per_block;
    uint32_t remaining = target_entry.file_size;
    uint16_t current_block = target_entry.starting_block;

    long data_region_offset = (long)sizeof(superblock_t) +
                              (long)sb.total_direntries * (long)sizeof(direntry_t);

    uint8_t *buffer = malloc(block_size);
    uint32_t payload_per_block = block_size - 3;

    if (!buffer){
        fprintf(stderr, "Failed to allocated %u-byte buffer\n", (unsigned)block_size);
        fclose(out);
        fclose(fp);
        return 7;
    }

    while (remaining > 0){
        if (current_block >= sb.total_blocks){
            fprintf(stderr, "Invalid block number %u encountered.\n", (unsigned)current_block);
            free(buffer);
            fclose(out);
            fclose(fp);
            return 8;
        }

        long block_offset = data_region_offset + (long)current_block * (long)block_size;

        if (fseek(fp, block_offset, SEEK_SET) != 0){
            perror("fseek data block");
            free(buffer);
            fclose(out);
            fclose(fp);
            return 9;
        }

        if (fread(buffer, 1, block_size, fp) != block_size){
            fprintf(stderr, "Short read on block %u\n", (unsigned)current_block);
            free(buffer);
            fclose(out);
            fclose(fp);
            return 10;
        }

        uint16_t next_block;
        memcpy(&next_block, buffer + (block_size - sizeof(uint16_t)), sizeof(uint16_t));

        size_t to_write = payload_per_block;
        if (remaining < payload_per_block) to_write = remaining;

        if (fwrite(buffer + 1, 1, to_write, out) != to_write){
            perror("fwrite");
            free(buffer);
            fclose(out);
            fclose(fp);
            return 11;
        }

        remaining -= (uint32_t)to_write;
        current_block = next_block;
    }

    free(buffer);
    fclose(out);
    fclose(fp);

    return 0;
}
