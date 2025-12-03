# Quinnipiac File System (QFS)
QFS (Quinnipiac File System) is a simplified educational file system designed to demonstrate core concepts of filesystem design, including superblocks, directory entries, file allocation, and block-based storage. It is suitable for small disk images up to **120 MB** and emphasizes clarity over complexity.

# Disk Layout Overview
A QFS disk is divided into three main regions:

- Superblock – Global filesystem metadata

- Directory Entry Table – Fixed-size table with up to 255 entries

- Data Blocks – Actual file data storage

# Superblock Structure
The superblock is stored at byte offset 0 and contains essential metadata.

| Field                | Type        | Description                              |
|----------------------|-------------|------------------------------------------|
| fstype               | uint8_t     | Magic number (`0x51` = 'Q')              |
| totalblocks          | uint16_t    | Total blocks in the file system          |
| availableblocks      | uint16_t    | Free block count                          |
| bytesperblock        | uint16_t    | Size of each block                        |
| totaldirentries      | uint8_t     | Always 255                                |
| availabledirentries  | uint8_t     | Number of free directory entries          |
| reserved             | uint8_t[8]  | Must be 0                                 |
| label                | char[15]    | Optional volume label                     |

Total size: 32 bytes

# Directory Entry Structure
Each directory entry is 32 bytes and contains all metadata needed to locate a file.

| Field          | Type        | Description                      |
|----------------|-------------|----------------------------------|
| filename       | char[23]    | NULL-terminated name             |
| permissions    | uint8_t     | User/Group/World + File type bits|
| ownerid        | uint8_t     | Owner ID (0–255)                 |
| groupid        | uint8_t     | Group ID (0–255)                 |
| startingblock  | uint16_t    | First block of the file          |
| filesize       | uint32_t    | File size in bytes               |

A directory entry is free if the first byte of filename is '\0'.

# Block Structure
Blocks begin immediately after the directory table at offset 8192 bytes.
The block size depends on the disk size:

| Disk Size Range                 | Block Size |
|----------------------------------|------------|
| ≤ 30 MB                          | 512 bytes  |
| 30–60 MB                         | 1024 bytes |
| 60–120 MB                        | 2048 bytes |


Each block contains:
| Field     | Type       | Description                               |
|-----------|------------|-------------------------------------------|
| isbusy    | uint8_t    | `1` = free, `0` = allocated               |
| data      | uint8_t[]  | Payload data (blocksize − 3 bytes)        |
| nextblock | uint16_t   | Block number of the next block (`0` = none) |


Files may span multiple blocks; every block (except last) stores a pointer to the next.

# File Storage Model
Files are stored across one or more blocks. Example for a 1256-byte file using 512-byte blocks:

| Block Number | Data Stored (Bytes) | Pointer to Next Block |
|--------------|----------------------|------------------------|
| 15           | 509 bytes            | 95                     |
| 95           | 509 bytes            | 23                     |
| 23           | 238 bytes            | (end)                  |

# Features Demonstrated
- Superblock-based filesystem metadata
  
- Fixed directory entry table
  
- Block allocation with chaining
  
- Simple free/busy tracking per block
  
- Support for up to 255 files
  
- UNIX-style user, group, world permissions

# Intended Educational Concepts
This filesystem helps demonstrate:

- How OS structures file system metadata
  
- Block-based allocation and linked-block files
  
- Directory and file lookup mechanisms
  
- How files span multiple blocks
  
- How a minimal filesystem can be implemented without complexity
  
