/*
 * Authors: Joshua Molin
 * Date: Wednesday December 10th 2025
 * 
 * This program reads a requested qfs disk image and lists its details and directore entries
 * 
 * Usage: list_information <disk image file>
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk image file>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 2;
    }

#ifdef DEBUG
    printf("Opened disk image: %s\n", argv[1]);
#endif

        // read the superblock from the disk image
    superblock_t sb;
    if (fread(&sb, sizeof(sb), 1, fp) != 1) {
        fprintf(stderr, "Failed to read superblock from: %s\n", argv[1]);
        fclose(fp);
        return 3;
    }

    // print superblock information using header details
    printf("Block size: %u bytes\n", sb.bytes_per_block);
    printf("Total number of blocks: %u\n", sb.total_blocks);
    printf("Number of free blocks: %u\n", sb.available_blocks);
    printf("Total number of directory entries: %u\n", sb.total_direntries);
    printf("Number of free directory entries: %u\n", sb.available_direntries);
    printf("\nDirectory contents:\n");

    // seek to the entries
    if (fseek(fp, (long)sizeof(superblock_t), SEEK_SET) != 0) {
        perror("fseek directory");
        fclose(fp);
        return 4;
    }

    // read and print directory entries
    direntry_t entry;
    int files_found = 0;
    for (uint8_t i = 0; i < sb.total_direntries; i++) {
        if (fread(&entry, sizeof(entry), 1, fp) != 1) {
            fprintf(stderr, "Failed to read directory entry %u\n", i);
            fclose(fp);
            return 5;
        }

        if (entry.filename[0] == '\0') {
            continue;
        }

        char filename[sizeof(entry.filename) + 1];
        memcpy(filename, entry.filename, sizeof(entry.filename));
        filename[sizeof(entry.filename)] = '\0';

        printf("File: %s, Size: %u bytes, Starting block: %u\n",
               filename, (unsigned)entry.file_size, entry.starting_block);
        files_found = 1;
    }

    // handle empty directory
    if (!files_found) {
        printf("(no files found)\n");
    }

    fclose(fp);
    return 0;
}