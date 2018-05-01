#ifndef STRUCTURES_H
#define STRUCTURES_H


// File descriptor (i-node)
enum inode_type {
        IS_FILE = 1,
        IS_DIR = 2
};

enum inode_open {
        IS_OPEN = 1,
        IS_CLOSED = 2
};

// File descriptor (i-node)
typedef struct _inode {
 	uint16_t num;
	uint16_t type;
	int16_t isOpen;
	char filename[32];
	uint32_t parent;
	uint32_t size;
	uint32_t blocks[15];
  	uint32_t** single_indir_ptr;
  	uint32_t*** double_indir_ptr;
}inode;

typedef struct _dirData {
        uint16_t num;
        char filename[16];
}dirData;

typedef struct _inodebitmap {
        int bitmap[1024];
}inode_bitmap;

typedef struct _blockbitmap {
        int bitmap[1024];
}block_bitmap;

// superblock struct
typedef struct _superblock {
	uint16_t magic_num;
	uint32_t num_of_inodes;
	uint32_t num_of_data_blocks;
	uint32_t max_num_of_files;
	uint32_t max_num_of_blocks;
	uint16_t block_size;
	uint16_t start_of_inode_table;
	uint16_t start_of_block_bitmap;
	uint16_t start_of_inode_bitmap;
	uint16_t start_of_data_block; // root node
} superblock;

#endif
