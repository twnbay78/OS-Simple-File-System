#ifndef STRUCTURES_H
#define STRUCTURES_H

// File descriptor (i-node)
typedef struct _inode {
	short int num;
	char filename[16];
	uint32_t size;
	char permissions[3];
	uint32_t blocks[15];
	/* - indirection ptrs, worry about it later
 * 	void* single_indir_ptr;
 * 	void* double_indir_ptr;
 * 	*/
}inode;

// LL for free blocks
typedef struct _free_list{
	inode* i_node;
	struct _free_list* next;
}free_list;

// file table open FD's
// keep pid so 
typedef struct _open_fd {
	inode* i_node;
	void* pid;
}open_fd;

// file table for all files

// superblock struct
typedef struct _superblock {
	uint32_t magic_num;
	uint32_t num_of_inodes;
	uint32_t num_of_data_blocks;
	uint32_t max_num_of_files;
	uint32_t max_num_of_blocks;
	uint32_t block_size;
	uint32_t start_of_inode_table;
	uint32_t start_of_block_bitmap;
	uint32_t start_of_inode_bitmap;
	uint32_t start_of_data_block; // root node
} superblock;
#endif
