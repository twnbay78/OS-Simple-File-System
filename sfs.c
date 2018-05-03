/*
   Simple File System
   This code is derived from function prototypes found /usr/include/fuse/fuse.h
   Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
   His code is licensed under the LGPLv2.
*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "structures.h"

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

#define SetBit(A,k)     ( A[(k/32)] |= (1 << (k%32)) )
#define ClearBit(A,k)   ( A[(k/32)] &= ~(1 << (k%32)) )
#define TestBit(A,k)    ( A[(k/32)] & (1 << (k%32)) )

inode* inode_table;
/* HELPER FUNCTIONS - inode table functions
 *
 *
 *
 */

/*
 * Takes in an inode table and writes the table to the disk. The only error checking the inode table does is with disk_write,
 * and will log messages if the disk could not be written to
 */
int write_inode_table(inode* inode_table, superblock* superblock_t) {


	// writing inode table to fs file
	int inode_start = superblock_t->start_of_inode_table;
	int i = 0;
	int q = 0;
	for(i = inode_start; i < superblock_t->start_of_data_block; ++i){
		inode* temp = (inode*)malloc(sizeof(inode)*32);
		for(q = 0; q < 32; ++q){
			log_msg("temp index: %d - inode_table index: %d\n", q, q + ((i - inode_start)*32));
			temp[q]  = inode_table[q + ((i-inode_start)*32)];
		}	
		int j = block_write(i, (const void*)temp);
		if (j < 1){
			log_msg("error trying to write inode table to block\n");
			return -1; 
		}
	}
	for(i = 0; i < superblock_t->num_of_inodes; ++i){
		log_msg("parent of i: %d\n", inode_table[i].parent);
	}
	free(inode_table);
	log_msg("inode table is loaded\n");
	return 0;
}

/*
 * This function will return the inode table currently written to disk
 * The only error checking done is with disk_read. Log messages will be written if
 * the blocks read are empty or if the blocks cannot be read.
 */
inode* load_inode_table(superblock* superblock_t) {

	log_msg("in load_inode\n");
	inode* inode_table = (inode*)malloc(sizeof(inode) * superblock_t->num_of_inodes);
	char* buffer[4096];
	int inode_start = superblock_t->start_of_inode_table;

	// load inode table into disk file
	int i = 0;
	int q = 0;
	for(i = superblock_t->start_of_inode_table; i < superblock_t->start_of_data_block; ++i){
		inode* temp;
		int j = block_read(i, buffer);
		temp = (inode*)buffer;
		if (j < 0){
			log_msg("error trying to write inode table to block\n");
			return NULL;
		}
		else if (j == 0){
			log_msg("while reading inode table block, blocks are empty\n");
			return NULL;
		}
		log_msg("pass read\n");

		for(q = 0; q < 32; ++q){
			inode_table[q + ((i-inode_start)*32)] = temp[q]; 
		}		
	}
	/*
	for(i = 0; i < superblock_t->num_of_inodes; ++i){
		log_msg("parent of i: %d\n", inode_table[i].parent);
	}*/

	log_msg("inode table is loaded\n");
	return inode_table;
}


/* HELPER FUNCTIONS - Bitmap Functions
 *
 *
 *
 */

/*
 * This function searches the inode_bitmap and returns an index number of the first found free inode, also sets the bit
 * If a free inode cannot be found, whether there are no more free inodes or undefined behavior occurs, the function will return -1. 
 */
int find_inode(superblock* superblock_t){

	log_msg("in find_inode\n");
	// load in bitmap
	char buffer[4096];
	int j = block_read(superblock_t->start_of_inode_bitmap, buffer);
	int* inode_bitmap = (int*)buffer;
	if (j < 0){
		log_msg("error trying to write inode table to block\n");
		return -1;
	}
	else if (j == 0){
		log_msg("while reading inode table block, blocks are empty\n");
		return -1;
	}
	log_msg("num_of_inodes: %d\n", superblock_t->num_of_inodes);
	int i = 0;

	// find free inode in inode_bitmap
	log_msg("num of inodes: %d\n", superblock_t->num_of_inodes);
	for(i = 1; i < superblock_t->num_of_inodes; ++i){
		log_msg("testbit ret val: %d\n", TestBit(inode_bitmap, i));
		if(!TestBit(inode_bitmap, i)){
			log_msg("found a free inode\n");
			// set bit because it is no longer free
			SetBit(inode_bitmap, i);
			if(!TestBit(inode_bitmap, i))
				log_msg("WE HAVE A WINNNNNERRRR LEO AIN't A STREET BABY AHAHAHAHAHAHHAAHHAAHAHA\n");
			// write bitmap back to fs file, error check
			j = block_write(superblock_t->start_of_inode_bitmap, inode_bitmap);
			if (j < 1){
				log_msg("error trying to write inode table to block\n");
				return -1;
			}
			return i;	
		}
	}
	log_msg("could not find a free inode\n");
	return -1;
}	

/*
 * This function searches the block bitmap for a free block. The block number is returned, and the bit is set in the bitmap
 * If a free block cannot be found, whether there are no more free blocks or undefined behavior occurs, the function will return -1.
 */
int find_block(superblock* superblock_t){
	log_msg("in find_block\n");

	// load in bitmap
	char buffer[4096];
	int j = block_read(superblock_t->start_of_block_bitmap, buffer);
	int* block_bitmap = (int*)buffer;
	if (j < 0){
		log_msg("error trying to write inode table to block\n");
		return -1;
	}
	else if (j == 0){
		log_msg("while reading inode table block, blocks are empty\n");
		return -1;
	}


	// find free block in block_bitmap
	int i = 0;
	for(i = 1; i < superblock_t->num_of_data_blocks; ++i){
		int x = TestBit(block_bitmap, i);
		log_msg("testbit ret val: %d\n", x);
		if(!TestBit(block_bitmap, i)){
			log_msg("found a free block\n");
			// set bit because it is no longer free
			SetBit(block_bitmap, i);
			// write bitmap back to fs file, error check
			j = block_write(superblock_t->start_of_block_bitmap, block_bitmap);
			if (j < 1){
				log_msg("error trying to write inode table to block\n");
				return -1;
			}
			return i;	
		}

	}

	log_msg("could not find a free block\n");
	return -1;
}

/* HELPER FUNCTIONS - parser functions
 *
 *
 *
 */

//ALL YOUR SLASHING NEEDS:
//returns number of slashes found in given path
//returns -1 if consecutive slashes or no beginning slash or has an ending slash
int numslash(const char *path){
	int pathlen = strlen(path);
	int slashcount=0;
	int lastslashind=-1;//make sure no consecutive slashes
	int i;//loop
	if (path[pathlen-1]=='/'||path[0]!='/')//path must begin with a slash and NOT end with a slash
		return -1;
	for (i=0;i<pathlen;i++){
		if (path[i]=='/'){
			if (lastslashind==-1){
				lastslashind=i;
			}else{
				if (i-1==lastslashind){
					return -1;
				}
			}
			slashcount++;
			lastslashind=i;
		}	
	}
	return slashcount;
}

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn)
{
	fprintf(stderr, "in bb-init\n");
	log_msg("\nsfs_init()\n");
	struct sfs_state* state = SFS_DATA;
	char* disk=state->diskfile;
	log_conn(conn);
	log_fuse_context(fuse_get_context());

	// Create superblock struct
	superblock* superblock_t = (superblock*)malloc(sizeof(superblock));
	superblock_t->magic_num = 69;
	superblock_t->num_of_inodes = 256;
	superblock_t->num_of_data_blocks = 4085;
	superblock_t->max_num_of_files = 256;
	superblock_t->max_num_of_blocks = 87;
	superblock_t->block_size = 4096;
	superblock_t->start_of_inode_table = 3;
	superblock_t->start_of_block_bitmap = 2;
	superblock_t->start_of_inode_bitmap = 1;
	superblock_t->start_of_data_block = 11;


	// declare bitmaps and set all to 0 indicating open
	int* inode_bitmap = (int*)malloc(sizeof(int) * 1024);
	int i;
	for(i = 0; i < 1024; ++i){
		inode_bitmap[i] = 0;
	}
	int* block_bitmap = (int*)malloc(sizeof(int) * 1024);
	for(i = 0; i < 1024; ++i){
		block_bitmap[i] = 0;
	}

	// declare inode table
	inode_table = (inode*)malloc(sizeof(inode)*256);

	// root dir
	memcpy(inode_table[0].filename, "/", 1);
	inode_table[0].num = 0;
	inode_table[0].type = IS_DIR;
	inode_table[0].isOpen = IS_CLOSED;
	inode_table[0].size = 0;
	inode_table[0].parent = -2;
	int j = 0;
	for(j = 0; j < 15; ++j){
		inode_table[0].blocks[j] = -1;
	}


	// fulling inode table 
	for(i = 1; i < 256; ++i){
		inode_table[i].num = i;
		inode_table[0].isOpen = IS_CLOSED;
		for(j = 0; j < 15; ++j){
			inode_table[i].blocks[j] = -1;
		}
		inode_table[i].parent = -1;
	}

	disk_open(disk);

	// write superblock to disk
	j = block_write(0, superblock_t);
	if (j < 1){
		log_msg("error trying to write inode table to block\n");
		return;
	}
	
	// write inode bitmap
	j = block_write(superblock_t->start_of_inode_bitmap, inode_bitmap);
	if (j < 1){
		log_msg("error trying to write inode table to block\n");
		return;
	}
	
	// write block bitmap
	j = block_write(superblock_t->start_of_block_bitmap, block_bitmap);
	if (j < 1){
		log_msg("error trying to write inode table to block\n");
		return;
	}
	/*
	// write inode table
	if(write_inode_table(inode_table, superblock_t) == -1){
		log_msg("failed to write inode_table\n");
		return;
	}*/

	free(superblock_t);
	free(inode_bitmap);
	free(block_bitmap);

	return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
	log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
	disk_close();
}

/** Get file attributes (metadata)
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
	char *filename=NULL;
	char tokenpath[PATH_MAX];
	tokenpath[PATH_MAX]='\0';
	int retstat = 0;
	log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
			path, statbuf);
	char buffers[4096];
	// read in superblock
	block_read(0, buffers);
	superblock* superblock_t = (superblock*) buffers;
	// read in inode_table
	//inode* inode_table = load_inode_table(superblock_t);
	if(inode_table == NULL){
		log_msg("inode table returned -1 in getattr\n");
		return -1;
	}
	inode root=inode_table[0];

	//If the inode is the root directory
	if ((strcmp(path,"/")==0)) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_size=root.size;
		statbuf->st_nlink = 1;
		statbuf->st_blocks=((root.size-1)+4096)/4096;
		statbuf->st_atime = time(NULL);
		statbuf->st_ctime = time(NULL);
		statbuf->st_mtime = time(NULL);
		/*
		// write changes to inode_table into the fs file
		if(write_inode_table(inode_table, superblock_t) == -1){
			log_msg("could not write inode_table to fs file\n");
			return -1;
		}*/
		
		log_msg("get attr returning\n");
		return retstat;
	}

	else {
		//Get the disk path
		char* disk=SFS_DATA->diskfile;
		//buffer to read into
		char buffer[4096];

		//Read in the root node
		inode rootDir=inode_table[0];
		int cp = rootDir.num;
		//Search through all the direct map ptrs
		int i=1;
		for(i;i<superblock_t->num_of_inodes;i++) {
			//block num referenced by ptr
			inode temp=inode_table[i];
			int parent = temp.parent;
			if(parent == cp) {
				// get filename, -1 for null terminator + -1 for root '/'
				
				strcpy(tokenpath, path);
				filename=strtok(tokenpath,"/");
				log_msg("tempgpath: %s - %s\n", temp.filename, filename);
				//Compares paths for match
				if(strcmp(temp.filename,filename)==0) {
					statbuf->st_nlink = 1;
					statbuf->st_mode = S_IFDIR | 0775;
					statbuf->st_size=temp.size;
					statbuf->st_blocks=((temp.size-1)+4096)/4096;
					statbuf->st_atime = time(NULL);
					statbuf->st_ctime = time(NULL);
					statbuf->st_mtime = time(NULL);
					if(temp.type == IS_FILE){
						log_msg("ISA FILE BITCH\n");
						statbuf->st_mode = S_IFREG | 0777;
					}
					/*
					// write changes to inode_table into the fs file
					if(write_inode_table(inode_table, superblock_t) == -1){
						log_msg("could not write inode_table to fs file\n");
						return -1;
					}*/
					return retstat;
				}
			}
		}
	}
	/*
	// write changes to inode_table into the fs file
	if(write_inode_table(inode_table, superblock_t) == -1){
		log_msg("could not write inode_table to fs file\n");
		return -1;
	}*/

	//free(inode_table);
	return -ENOENT;
}
/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	int retstat = 0;
	log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
			path, mode, fi);

	int i,j;
	int iposition;//bit index of inode bitmap
	int dposition;//bit index of data block bitmap
	int slashcount;
	char tokenpath[PATH_MAX];//can't modify path so copy 
	char *token=NULL;//will hold current token
	char *parname; //store file's parent (IMPORTANT)
	int parid;//parent id
	char **pathwords;//tokenized "words" in path
	int numdir;//number of directories in path input
	//inode *table;//will hold inode table read from disk

	//some path checking
	if ((slashcount = numslash(path))>=1){//checking numslash doesn't give -1
		numdir=slashcount-1;
	}else if (strcmp(path,"/")==0){//path shouldnt be "/"
		log_msg("failure in create with path: %s\n",path);
		return -1;
	}else{//either numslash returned -1, or 0
		log_msg("failure in create with bad path: %s\n",path);
		return -1;
	}

	//retrieve superblock
	char sbuf[4096];
	superblock* s;
	int z = block_read(0, sbuf);
	if (z < 0){
		log_msg("error trying to write inode table to block\n");
	}
	else if (z == 0){
		log_msg("while reading inode table block, blocks are empty\n");
	}
	s=(superblock*)sbuf;
	log_msg("num_of_inodes in create: %d\n", s->block_size);
	log_msg("num_of_inodes in create: %d\n", s->num_of_inodes);
	int it_start = 0;
	int it_end = s->max_num_of_files;

	iposition=find_inode(s);
	dposition=find_block(s);
	log_msg("inode returned: %d\n", iposition);
	log_msg("data block returned: %d\n", dposition);
	if (iposition==-1 || dposition==-1){
		log_msg("failure in create with invalid space left\n");
		return -1;
	}

	//tokenizing time!
	log_msg("path before strcpy: %s\n", path);
	tokenpath[PATH_MAX] = '\0';
	strcpy(tokenpath,path);
	token=strtok(tokenpath,"/");
	log_msg("path after strcpy: %s\n", path);

	if (slashcount==1){//root case: root is parent dir
		//inode_table=(inode *)malloc(it_end*sizeof(inode));
		//inode_table = load_inode_table(s);
		if(inode_table == NULL){
			log_msg("inode_table returned NULL in create\n");
			return -1;
		}
		//check if file exists
		for (i=it_start;i<it_end;i++){//iterate thru inode table block nums
			if (strcmp(inode_table[i].filename,token)==0 && inode_table[i].parent==0){//filenames match and rootdir is parent (inum=0?)
				//BADDD it EXISTS!!!
				log_msg("failure in create: FILE EXISTS\n");
				//free(table);
				return -1;
			}
		}
		//create file at iposition
		//modify superblock
		s->num_of_inodes++;
		s->num_of_data_blocks++;
		//modify bitmaps with indices: dposition, iposition

		//initialize new inode
		inode_table[iposition].type=IS_FILE;
		inode_table[iposition].isOpen=IS_OPEN;//open it 
		strcpy(inode_table[iposition].filename,token);
		log_msg("create is inserting filename %s into new inode\n",inode_table[iposition].filename);
		inode_table[iposition].parent=0;
		inode_table[iposition].size=0;
		inode_table[iposition].blocks[0]=dposition;
		inode_table[iposition].single_indir_ptr=NULL;
		inode_table[iposition].double_indir_ptr=NULL;

		//writing back to disk
		block_write(0,s);//write superblock back
		/*
		//write bitmaps back
		if(write_inode_table(table,s) == -1){
			log_msg("could not write inode table to disk in create\n");
			return -1;
		}//write back to disk
		*/
		//free(inode_table);
		log_msg("create is a success with path: %s\n",path);
		return 0;
	}


}

/** Remove a file */
int sfs_unlink(const char *path){
	int retstat = 0;
	log_msg("sfs_unlink(path=\"%s\")\n", path);

	int slashcount;//return value of numslash 
	int i=0;//loop var
	int j=0;//loop var
	char tokenpath[PATH_MAX];//can't modify path so copy 
	char *token=NULL;//will hold current token
	char **pathwords;//tokenized "words" in path
	int numdir;//number of directories in path input
	//inode *table;

	//path checking
	if ((slashcount = numslash(path))>=1){//checking numslash doesn't give -1
		numdir=slashcount-1;
	}else if (strcmp(path,"/")==0){//path shouldnt be "/"
		log_msg("failure in unlink with path: %s\n",path);
		return -1;
	}else{//either numslash returned -1, or 0
		log_msg("failure in unlink with bad path\n");
		return -1;
	}

	//load in superblock
	char sbuf[BLOCK_SIZE];
	superblock *s = (superblock *)malloc(sizeof(superblock));
	block_read(0,sbuf);
	s=(superblock *)sbuf;
	if(s == NULL){
		log_msg("superblock returned NULL in unlink\n");
		return -1;
	}
	//load in inode table
	//inode_table=(inode *)malloc(s->num_of_inodes*sizeof(inode));
	//inode_table = load_inode_table(s);
	//load in block bitmap
	int* block_bitmap = (int*)malloc(sizeof(int) * 1024);
	block_read(s->start_of_block_bitmap, block_bitmap);
	//load in inode bitmap
	int* inode_bitmap = (int*)malloc(sizeof(int) * 1024);
	block_read(s->start_of_inode_bitmap, inode_bitmap);

	//check table and bitmap mallocs
	if(inode_table == NULL){
		log_msg("inode_table returned NULL in open\n");
		return -1;
	}else if(inode_bitmap == NULL){
		log_msg("inode_bitmap returned NULL in open\n");
		return -1;
	}else if(block_bitmap == NULL){
		log_msg("block_bitmap returned NULL in open\n");
		return -1;
	}

	//tokenizing time!
	strcpy(tokenpath,path);
	token=strtok(tokenpath,"/");

	int blocksfreed=0; 

	if (slashcount==1){//root parent case
		for (i=0;i<s->num_of_inodes;i++){//iterate thru inode table block nums
			if (strcmp(inode_table[i].filename,token)==0 && inode_table[i].parent==0){//filenames match and rootdir is parent (inum=0?)
				//update inode
				inode_table[i].isOpen=IS_CLOSED;//close it 
				inode_table[i].parent=-1;//reset parent 
				inode_table[i].size=0;//reset size
				//clear filename
				for (j=0;j<32;j++){
					inode_table[i].filename[j]='\0';
				}
				//clear each block and reset their bits
				for (j=0;j<15;j++){
					if (inode_table[i].blocks[j]!=-1){//something is there
						ClearBit(block_bitmap,j);//reset block bit
						inode_table[i].blocks[j]=-1;//reset block 
						blocksfreed++;
					}	
				}

				//reset inode bit
				ClearBit(inode_bitmap,i);
				//update superblock
				s->num_of_inodes--;
				s->num_of_data_blocks-=blocksfreed;

				//write back bitmaps, superblock, inode table
				if(block_write(0,s) == -1){//superblock
					log_msg("couldn't write superblock back");
					return -1;
				}else if(block_write(s->start_of_inode_bitmap, inode_bitmap) == -1){//inode bitmap
					log_msg("couldn't write inode bitmap back");
					return -1;
				}else if(block_write(s->start_of_block_bitmap,block_bitmap) == -1){//block bitmap
					log_msg("couldn't write block bitmap back");
					return -1;
				}/*
				else if(write_inode_table(table,s) == -1){//inode table
					log_msg("couldn't write inode table back");
					return -1;
				}*/
				//free superblock and return
				//free(table);
				free(block_bitmap);
				free(inode_bitmap);
				log_msg("unlink is successful with path: %s\n",path);
				return 0;
			}
		}
		//free(table);
		free(block_bitmap);
		free(inode_bitmap);
		log_msg("unlink is a failure with path: %s\n",path);
		return -1;
	}
	//free(table);
	free(block_bitmap);
	free(inode_bitmap);
	log_msg("unlink is a failure with path: %s\n",path);
	return -1;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi){

	int retstat = 0;
	log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
			path, fi);
	int slashcount;//return value of numslash 
	int i=0;//loop var
	int j=0;//loop var
	char tokenpath[PATH_MAX];//can't modify path so copy 
	char *token=NULL;//will hold current token
	char *parname; //store file's parent (IMPORTANT)
	int parid;//parent id
	char **pathwords;//tokenized "words" in path
	int numdir;//number of directories in path input
	//inode *table;

	if ((slashcount = numslash(path))>=1){//checking numslash doesn't give -1
		numdir=slashcount-1;
	}else if (strcmp(path,"/")==0){//path shouldnt be "/"
		log_msg("failure in open with path: %s\n",path);
		return -1;
	}else{//either numslash returned -1, or 0
		log_msg("failure in open with bad path\n");
		return -1;
	}

	//retrieve superblock
	char sbuf[BLOCK_SIZE];
	superblock *s;
	block_read(0,sbuf);
	s=(superblock *)sbuf;
	int it_start = 0;
	int it_end = s->max_num_of_files;

	//tokenizing time!
	strcpy(tokenpath,path);
	token=strtok(tokenpath,"/");
	if (slashcount==1){//root parent case
		//table=(inode *)malloc(it_end*sizeof(inode));
		//table = load_inode_table(s);
		if(inode_table == NULL){
			log_msg("inode_table returned NULL in open\n");
		}
		for (i=it_start;i<it_end;i++){//iterate thru inode table block nums
			if (strcmp(inode_table[i].filename,token)==0 && inode_table[i].parent==0){//filenames match and rootdir is parent (inum=0?)
				inode_table[i].isOpen=IS_OPEN;//open it 
				/*
				//write back
				if(write_inode_table(table,s) == -1){
					return -1;
				}*/
				log_msg("open is successful with path: %s\n",path);
				return 0;
			}
		}
		//free(table);
		log_msg("open is a failure with path: %s\n",path);
		return -1;
	}else{//othercases that are deeper than rootdir
		/*
		   pathwords=(char **)malloc(slashcount*sizeof(char *));
		   for (j=0;j<slashcount;j++){
		   pathwords[j]=(char *)malloc(16*sizeof(char));
		   }
		   i=0;
		   while (token!=NULL){
		   printf("token is currently: %s\n",token);
		   pathwords[i]=token;
		//do stuff before next token
		if (i==numdir-1){//store parent of file
		parname=token;
		}
		//store stuff?
		//end do stuff
		token = strtok(NULL, "/");//next token
		i++;
		}
		*/
		//ALGO
		//iterate thru ALL files
		//fetch current inode
		//see if filename matches token && see if parent inode = root inode (from superblock) 
		//if so, set to open, free shit and return 0
		//[after loop]: file does not exist, free shit, return -1
		//if not, then create and set its parent to "/"
		return -1;
	}
	/*
	// write inode table back to fs file
	if(write_inode_table(table, s) == -1){
		log_msg("could not write inode_table to disk\n");
		return -1;
	}*/

}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
			path, fi);

	if(strcmp(path, "/") == 0){
		log_msg("you done goofed, you tried to file release the root dir!\n");
		return -1;
	}
	// CASE FOR ONLY FILES IN ROOT DIR
	int i = numslash(path);
	if (i == 1){
		log_msg("only 1 slash in the path\n");
		// load in superblock
		superblock* superblock_t = (superblock*)malloc(sizeof(superblock));
		log_msg("works so far\n");
		int z = block_read(0, superblock_t);
		if (z < 0){
			log_msg("error trying to write inode table to block\n");
			return -1;
		}
		else if (z == 0){
			log_msg("while reading inode table block, blocks are empty\n");
			return -1;
		}
		
		log_msg("wrote superblock\n");

		// get filename, -1 for null terminator + -1 for root '/'
		char token[32];
		strcpy(token, path);
		char filename[sizeof(token)-1];
		memcpy(filename, &token[1], (sizeof(token)-1));
		log_msg("filename to be searched for in release: %s\n", filename);
		// load in inode table
		//inode* inode_table = load_inode_table(superblock_t);
		if(inode_table == NULL){
			log_msg("inode_table returned NULL in release\n");
			return -1;
		}
		// look for file in inode table
		for(i = 1; i < superblock_t->num_of_inodes; ++i){
			// if file exists and is in the root dir
			if((strcmp(filename, inode_table[i].filename) == 0) && (inode_table->parent == -1)){
				// change isOpen field to IS_OPEN 
				log_msg("found file to close in releasedir\n");
				inode_table[i].isOpen = IS_CLOSED;
				log_msg("release just closed file\n");
				break;
			}
		}
	}
	else {
		log_msg("There is a consecutive slash or no beginning slash or has an ending slash in the path when closing");
		return -1;
	}
	log_msg("release ended");
	return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
			path, buf, size, offset, fi);
	char bufferss[4096];
	int z = block_read(0, bufferss);
	if (z < 0)
		log_msg("error trying to write inode table to block\n");
	else if (z == 0)
		log_msg("while reading inode table block, blocks are empty\n");
	superblock* superblock_t = (superblock*) bufferss;
	// read in inode_table
	//inode* inode_table = load_inode_table(superblock_t);
	if(inode_table == NULL){
		log_msg("inode table returned -1 in read\n");
		return -1;
	}
	inode r = inode_table[0];
	int x = 1;
	int amountRead = 0; 
	for (; x <superblock_t->num_of_inodes ; x++)
	{
		inode r = inode_table[x];

		if(strcmp(r.filename,path+1) == 0)
		{
			int numBlocksToRead = ((offset%4096+size)-1+4096)/4096;

			int firstBlock = offset/4096;

			int lastBlock = firstBlock+numBlocksToRead;

			int i = firstBlock;

			for(i;i<=lastBlock;i++) {

				if(i<15) {
					char buffer3[4096];

					//initialize
					if(r.blocks[i]==-1) {
						return amountRead;
					}

					block_read(r.blocks[i], buffer3);

					if(i==firstBlock) {

						memcpy(buf+amountRead,buffer3+offset%4096,4096-offset%4096);
						amountRead+=4096-offset%4096;
					} 
					else if (i==lastBlock) {
						memcpy(buf+amountRead,buffer3,size-amountRead);
						amountRead+=size-amountRead;
					}
					else {
						memcpy(buf+amountRead,buffer3,4096);
						amountRead+=4096-offset%4096;
					}
				}

			}
		}
	}

	return amountRead;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
			path, buf, size, offset, fi);
	int u=0;
	char buffers[4096];
	// read in superblock
	block_read(0, buffers);
	superblock* superblock_t = (superblock*) buffers;
	// read in inode_table
	//inode* inode_table = load_inode_table(superblock_t);
	inode root = inode_table[0];
	int cp = root.num;	
	for(u;u<superblock_t->num_of_inodes;u++)
	{
		inode temp = inode_table[u];

		if(strcmp(temp.filename,path+1)==0)
		{
			root.size = root.size+offset+size-temp.size;		

			int numBlockToWrite=((offset%4096+size)-1+4096)/4096;

			int firstBlock = offset/4096;

			int lastBlock = firstBlock+numBlockToWrite;

			int i = firstBlock;

			int amountWrite = 0;

			for(i;i<=lastBlock;i++)
			{

				if(i<15)
				{
					char buffers[4096];
					//init
					if(temp.blocks[i]==-1)
					{
						int j = superblock_t->start_of_data_block + superblock_t->num_of_data_blocks;
						superblock_t->num_of_data_blocks+=1;
						temp.blocks[i] = j+1;
						break;
					}
					block_read(temp.blocks[i],buffers);
					if(i == firstBlock)
					{
						int writeSize = 4096-offset%4096;
						if(writeSize>size)
						{
							writeSize = size;					
						}				

						memcpy(buffers+offset%4096,buf + amountWrite,writeSize);
						amountWrite+=writeSize;
					}
					else {
						int writeSize=4096;
						if(amountWrite+writeSize>size) {
							writeSize=size-amountWrite;
						}
						memcpy(buffers,buf + amountWrite,writeSize);
						amountWrite+=writeSize;
					}	
					block_write(temp.blocks[i],buffers);	
				}
			}	
		}
	}
	return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
			path, fi);
	int i=0;
	int found=0;
	char buffer[4096];
	//load root node into root
	char buffers[4096];
	// read in superblock
	block_read(0, buffers);
	superblock* superblock_t = (superblock*) buffers;
	// read in inode_table

	//inode* inode_table = load_inode_table(superblock_t);

	inode root = inode_table[0];
	int cp = root.num;
	if(strcmp("/", path) == 0){
		//root.isOpen = 1;
		return 0;
	}
	//search through all of the direct mapped pointers in root
	for(;i < superblock_t->num_of_inodes; i++ ){
		inode temp = inode_table[i];
		if(i == cp){
			if(strcmp(path+1,temp.filename)==0 && root.isOpen==0) {
				found = 1;
				//root.isOpen = 1;
			}
		}
	}
	if (found == 0)
	{ 
		return  0;	
	}
	else {
		return retstat;	
	}
}
/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{

	// read in superblock

	int retstat = 0;
	log_msg("\nreaddir has begun\n");
	if(strcmp(path,"/") != 0 )
	{
		return -1;
	}

	filler(buf,".",NULL,0);
	filler(buf,"..",NULL,0);

	char* disk = SFS_DATA->diskfile;
	char buffers[4096];
	// read in superblock
	block_read(0, buffers);
	superblock* superblock_t = (superblock*) buffers;
	// read in inode_table
	//inode* inode_table = load_inode_table(superblock_t);
	if(inode_table == NULL){
		log_msg("inode table returned -1 in readdir\n");
		return -1;
	}
	inode root = inode_table[0];
	int cp = 0;
	//Search through all the direct map ptrs
	int i=0;
	for(i;i < superblock_t->num_of_inodes;i++) {
		//log_msg("\nreaddir: inside first for loop %d\n",i);
		//block num referenced by ptr
		inode temp = inode_table[i];
		//log_msg("%s\n",temp.filename);
		//if valid ptr
		if(temp.parent==cp) {
			log_msg("entered 2nd for loop");
			//read in inode
			log_msg("\npath=\"%s\"\n",temp.filename);
			//Compares paths for match

			if (filler(buf, temp.filename, NULL, 0) != 0){
				log_msg("\nerror returned: ENOMEM. When inserting file with path:\"%s\"\n",temp.filename);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_releasedir(path=\"%s\", fi=0x%08x)\n",
			path, fi);

	return retstat;
}

/** Remove a directory */
int sfs_rmdir(const char *path)
{
	int retstat = 0;
	log_msg("sfs_rmdir(path=\"%s\")\n",
			path);

	int slashcount;//return value of numslash 
	int i=0;//loop var
	int j=0;//loop var
	char tokenpath[PATH_MAX];//can't modify path so copy 
	char *token=NULL;//will hold current token
	char **pathwords;//tokenized "words" in path
	int numdir;//number of directories in path input
	//inode *table;

	//path checking
	if ((slashcount = numslash(path))>=1){//checking numslash doesn't give -1
		numdir=slashcount-1;
	}else if (strcmp(path,"/")==0){//path shouldnt be "/"
		log_msg("failure in rmdir with path: %s\n",path);
		return -1;
	}else{//either numslash returned -1, or 0
		log_msg("failure in rmdir with bad path\n");
		return -1;
	}

	//load in superblock
	char sbuf[BLOCK_SIZE];
	superblock *s = (superblock *)malloc(sizeof(superblock));
	block_read(0,sbuf);
	s=(superblock *)sbuf;
	if(s == NULL){
		log_msg("superblock returned NULL in unlink\n");
		return -1;
	}
	//load in inode table
	//inode_table=(inode *)malloc(s->num_of_inodes*sizeof(inode));
	//inode_table = load_inode_table(s);
	//load in block bitmap
	int* block_bitmap = (int*)malloc(sizeof(int) * 1024);
	block_read(s->start_of_block_bitmap, block_bitmap);
	//load in inode bitmap
	int* inode_bitmap = (int*)malloc(sizeof(int) * 1024);
	block_read(s->start_of_inode_bitmap, inode_bitmap);

	//check table and bitmap mallocs
	if(inode_table == NULL){
		log_msg("inode_table returned NULL in open\n");
		return -1;
	}else if(inode_bitmap == NULL){
		log_msg("inode_bitmap returned NULL in open\n");
		return -1;
	}else if(block_bitmap == NULL){
		log_msg("block_bitmap returned NULL in open\n");
		return -1;
	}

	//tokenizing time!
	strcpy(tokenpath,path);
	token=strtok(tokenpath,"/");

	int blocksfreed=0; 

	if (slashcount==1){//root parent case
		for (i=0;i<s->num_of_inodes;i++){//iterate thru inode table block nums
			if (strcmp(inode_table[i].filename,token)==0 && inode_table[i].parent==0){//filenames match and rootdir is parent (inum=0?)
				//update inode
				inode_table[i].isOpen=IS_CLOSED;//close it 
				inode_table[i].parent=-1;//reset parent 
				inode_table[i].size=0;//reset size
				//clear filename
				for (j=0;j<32;j++){
					inode_table[i].filename[j]='\0';
				}
				//clear each block and reset their bits
				for (j=0;j<15;j++){
					if (inode_table[i].blocks[j]!=-1){//something is there
						ClearBit(block_bitmap,j);//reset block bit
						inode_table[i].blocks[j]=-1;//reset block 
						blocksfreed++;
					}	
				}

				//reset inode bit
				ClearBit(inode_bitmap,i);
				//update superblock
				s->num_of_inodes--;
				s->num_of_data_blocks-=blocksfreed;

				//write back bitmaps, superblock, inode table
				if(block_write(0,s) == -1){//superblock
					log_msg("couldn't write superblock back");
					return -1;
				}else if(block_write(s->start_of_inode_bitmap, inode_bitmap) == -1){//inode bitmap
					log_msg("couldn't write inode bitmap back");
					return -1;
				}else if(block_write(s->start_of_block_bitmap,block_bitmap) == -1){//block bitmap
					log_msg("couldn't write block bitmap back");
					return -1;
				}/*
				else if(write_inode_table(table,s) == -1){//inode table
					log_msg("couldn't write inode table back");
					return -1;
				}*/
				//free superblock and return
				//free(table);
				free(block_bitmap);
				free(inode_bitmap);
				log_msg("rmdir is successful with path: %s\n",path);
				return 0;
			}
		}
		//free(table);
		free(block_bitmap);
		free(inode_bitmap);
		log_msg("rmdir is a failure with path: %s\n",path);
		return -1;
	}
	//free(table);
	free(block_bitmap);
	free(inode_bitmap);
	log_msg("rmdir is a failure with path: %s\n",path);

	return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
	int retstat = 0;
	log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
			path, mode);

	int i,j;
	int iposition;//bit index of inode bitmap
	int dposition;//bit index of data block bitmap
	int slashcount;
	char tokenpath[PATH_MAX];//can't modify path so copy 
	char *token=NULL;//will hold current token
	char *parname; //store file's parent (IMPORTANT)
	int parid;//parent id
	char **pathwords;//tokenized "words" in path
	int numdir;//number of directories in path input
	//inode *table;//will hold inode table read from disk

	//some path checking
	if ((slashcount = numslash(path))>=1){//checking numslash doesn't give -1
		numdir=slashcount-1;
	}else if (strcmp(path,"/")==0){//path shouldnt be "/"
		log_msg("failure in mkdir with path: %s\n",path);
		return -1;
	}else{//either numslash returned -1, or 0
		log_msg("failure in mkdir with bad path: %s\n",path);
		return -1;
	}

	//retrieve superblock
	char sbuf[4096];
	superblock* s = (superblock*)malloc(sizeof(superblock));
	int z = block_read(0, s);
	if (z < 0){
		log_msg("error trying to write inode table to block\n");
	}
	else if (z == 0){
		log_msg("while reading inode table block, blocks are empty\n");
	}
	int it_start = 0;
	int it_end = s->max_num_of_files;

	iposition=find_inode(s);
	dposition=find_block(s);
	if (iposition==-1 || dposition==-1){
		log_msg("failure in mkdir with invalid space left\n");
		return -1;
	}

	//tokenizing time!
	log_msg("path before strcpy: %s\n", path);
	tokenpath[strlen(path)] = '\0';
	strcpy(tokenpath,path);
	token=strtok(tokenpath,"/");
	log_msg("path after strcpy: %s\n", path);

	if (slashcount==1){//root case: root is parent dir
		//inode_table=(inode *)malloc(it_end*sizeof(inode));
		//inode_table = load_inode_table(s);
		if(inode_table == NULL){
			log_msg("inode_table returned NULL in mkdir\n");
			return -1;
		}
		//check if file exists
		for (i=it_start;i<it_end;i++){//iterate thru inode table block nums
			if (strcmp(inode_table[i].filename,token)==0 && inode_table[i].parent==0){//filenames match and rootdir is parent (inum=0?)
				//BADDD it EXISTS!!!
				log_msg("failure in mkdir: FILE EXISTS\n");
				//free(table);
				return -1;
			}
		}
		//create file at iposition
		//modify superblock
		s->num_of_inodes++;
		s->num_of_data_blocks++;
		//modify bitmaps with indices: dposition, iposition

		//initialize new inode
		inode_table[iposition].type=IS_DIR;
		inode_table[iposition].isOpen=IS_OPEN;//open it 
		strcpy(inode_table[iposition].filename,token);
		inode_table[iposition].parent=0;
		inode_table[iposition].size=0;
		inode_table[iposition].blocks[0]=dposition;
		inode_table[iposition].single_indir_ptr=NULL;
		inode_table[iposition].double_indir_ptr=NULL;

		//writing back to disk
		block_write(0,s);//write superblock back
		/*
		//write bitmaps back
		if(write_inode_table(table,s) == -1){
			log_msg("could not write inode table to disk in create\n");
			return -1;
		}//write back to disk
		*/
		//free(inode_table);
		log_msg("create is a success with path: %s\n",path);
		return 0;
	}
		


	return retstat;
}

struct fuse_operations sfs_oper = {
	.init = sfs_init,
	.destroy = sfs_destroy,

	.getattr = sfs_getattr,
	.create = sfs_create,
	.unlink = sfs_unlink,
	.open = sfs_open,
	.release = sfs_release,
	.read = sfs_read,
	.write = sfs_write,

	.rmdir = sfs_rmdir,
	.mkdir = sfs_mkdir,

	.opendir = sfs_opendir,
	.readdir = sfs_readdir,
	.releasedir = sfs_releasedir
};

void sfs_usage()
{
	fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
	abort();
}

int main(int argc, char *argv[])
{
	int fuse_stat;
	struct sfs_state *sfs_data;

	// sanity checking on the command line
	if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
		sfs_usage();

	sfs_data = malloc(sizeof(struct sfs_state));
	if (sfs_data == NULL) {
		perror("main calloc");
		abort();
	}

	// Pull the diskfile and save it in internal data
	sfs_data->diskfile = argv[argc-2];
	argv[argc-2] = argv[argc-1];
	argv[argc-1] = NULL;
	argc--;

	sfs_data->logfile = log_open();

	// turn over control to fuse
	fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
	fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
	fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

	return fuse_stat;
}
