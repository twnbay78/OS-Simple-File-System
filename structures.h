#ifndef STRUCTURES_H
#define STRUCTURES_H

// File descriptor (i-node)
typedef struct _inode {
	short int num;
	char filename[16];
	int size;
	char permissions[3];
	void* block_ptr1;
	void* block_ptr2;
	void* block_ptr3;
	void* block_ptr4;
	void* block_ptr5;
	void* block_ptr6;
	void* block_ptr7;
	void* block_ptr8;
	void* block_ptr9;
	void* block_ptr10;
	void* block_ptr11;
	void* block_ptr12;
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
#endif
