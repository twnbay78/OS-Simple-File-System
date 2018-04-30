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

/* HELPER FUNCTIONS - inode functions
 *
 *
 *
 */

/*
 * Takes in an inode table and writes the table to the disk. The only error checking the inode table does is with disk_write,
 * and will log messages if the disk could not be written to
 */
void write_inode_table(inode* inode_table, superblock* superblock_t) {
	
	// writing inode table to fs file
	int i = 0;
	int q = 0;
	for(i = superblock_t->start_of_inode_table; i < superblock_t->start_of_data_block; ++i){
		for(q = 0; q < 32; ++q){
			int j = disk_write(i, inode_table[q]);
			if (j < 1){
				log_msg("error trying to write inode table to block");
			}
		}	
	}
	free(inode_table);
	log_msg("inode table is loaded");
}

/*
 * This function will return the inode table currently written to disk
 * The only error checking done is with disk_read. Log messages will be written if
 * the blocks read are empty or if the blocks cannot be read.
 */
inode* load_inode_table(superblock* superblock_t) {
	
	inode* inode_table = (inode*)malloc(sizeof(inode) * superblock_t->num_of_inodes);

	// load inode table into disk file
	int i = 0;
	int q = 0;
	for(i = superblock_t->start_of_inode_table; i < superblock_t->start_of_data_block; ++i){
		inode* temp = (inode*)malloc(sizeof(inode) * 32);
		int j = disk_read(i, temp);
		if (j < 0){
			log_msg("error trying to write inode table to block");
		}else if (j == 0){
			log_msg("while reading inode table block, blocks are empty");
		}
		for(q = 0; q < 32; ++q){
			inode_table[q + ((i*32)-1)] = temp[q]; 
		}		
		free(temp);
	}
	
	log_msg("inode table is loaded");
	return inode_table;
}

/*
 * This function searches the inode_bitmap and returns an index number of the first found free inode
 * If a free inode cannot be found, whether there are no more free inodes or undefined behavior occurs, the function will return -1. 
 */
int find_inode(superblock* superblock_t){
	
	inode* inode_table = load_inode_table(superblock_t);
	
	// load in bitmap
	int* inode_bitmap;
	block_read(superblock_t->inode_bitmap, inode_bitmap);

	return -1;
}	

/*
 * This function searches the block bitmap for a free block. The block number is returned.
 * If a free block cannot be found, whether there are no more free blocks or undefined behavior occurs, the function will return -1.
 */
int find_block(

}

/* HELPER FUNCTIONS - Bitmap Functions
 *
 *
 *
 */




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
	inode* inode_table = (inode*)malloc(sizeof(inode)*256);

        // root dir
	memcpy(inode_table[0].filename, "/", 1);
        inode_table[0].num = 0;
        inode_table[0].type = IS_DIR;
        inode_table[0].isOpen = IS_CLOSED;
        inode_table[0].size = 0;


	
	// fulling inode table 
        for(i = 1; i < 256; ++i){
                inode_table[i].num = i;
                inode_table[0].isOpen = IS_CLOSED;
        }

	disk_open(disk);

	write_inode_table(inode_table, superblock_t);
	
	free(superblock_t);
	free(inode_bitmap);
	free(block_bitmap);
	free(inode_table);
	
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
}

/** Get file attributes (metadata)
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
			path, statbuf);
 	
 	char buffer[512];
 	
 	
 	block_read(14,buffer);
    inode* root=(inode*)buffer;
    //If the inode is the root directory

    if ((strcmp(path,"/")==0)) {
    	statbuf->st_mode = S_IFDIR | 0755;
    	statbuf->st_size=root->size;
    	statbuf->st_nlink = 1;
    	statbuf->st_blocks=((root->size-1)+512)/512;
    	return retstat;
    }

    else {
    	//Get the disk path
    	char* disk=SFS_DATA->diskfile;
    	//buffer to read into
    	char buffer[512];

    	//Read in the root node
    	block_read(14,buffer);
    	inode* rootDir=(inode*)buffer;

    	//Search through all the direct map ptrs
    	int i=1;
    	for(i;i<15;i++) {
    		//block num referenced by ptr
    		int blocknum=rootDir->blocks[i];

    		//if valid ptr
    		if(blocknum>0) {

    			//read in inode
    			char buffer2[512];
    			block_read(blocknum,buffer2);
    			inode* tempNode=(inode*)buffer2;

    			//Compares paths for match
    			if(strcmp(tempNode->filename,path+1)==0) {
			    	statbuf->st_nlink = 2;
			    	statbuf->st_mode = S_IFDIR | 0777;
			    	statbuf->st_size=tempNode->size;
			    	statbuf->st_blocks=((tempNode->size-1)+512)/512;
			    	return retstat;
    			}
    		}
    	}
	}
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
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
			path, mode, fi);


	return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
	int retstat = 0;
	log_msg("sfs_unlink(path=\"%s\")\n", path);


	return retstat;
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
int sfs_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
			path, fi);


	return retstat;
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
/*
	int x = 1;
	int amountReadIN = 0; 
	for (; x <15 ; x++)
	{
		char buffer[512];
		block_read(x,buffer);
		inode* currentBlock = (inode*) buffer;

		if(strcmp(current->path,path+1) == 0)
		{
			int numBlocksToRead = ((offset%512+size)-1+512)/512;
			
			int firstBlock = offset/512;

			int lastBlock = firstBlock+numBlocksToRead;

			int i = firstBlock;

			 for(i;i<=lastBlock;i++) {

                if(i<15) {
                    char buffering[512];

                    //initialize
                    if(current->blocks[i]==-1) {
                        return amountRead;
                    }

                    block_read(current->dblocks[i], buffering);

                    if(i==firstBlock) {

                        memcpy(buf+amountRead,buffer3+offset%512,512-offset%512);
                        amountRead+=512-offset%512;
                    } else if (i==lastBlock) {
                        memcpy(buf+amountRead,buffer3,size-amountRead);
                        amountRead+=size-amountRead;
                    }
                    else {
                        memcpy(buf+amountRead,buffer3,512);
                        amountRead+=512-offset%512;
                     }
                }

		}
	}
	}
*/
	return -1;
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


	return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
	int retstat = 0;
	log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
			path, mode);


	return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
	int retstat = 0;
	log_msg("sfs_rmdir(path=\"%s\")\n",
			path);


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
		char buffer[512];
		//load root node into root
 		block_read(14,buffer);
 			inode* root=(inode*)buffer;
    	if(strcmp(root->filename, path) == 0){
			//root->isOpen = 1;
			return 0;
		}
		//search through all of the direct mapped pointers in root
		for(;i < 15; i++ ){
			if(root->blocks[i] > 0){
				if(strcmp(path+1,root->filename)==0 && root->isOpen==0) {
					found = 1;
					root->isOpen = 1;
					block_write(14, root);
				}
			}
		}
		if (found==0)
		{
			return  -1;	
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
	int retstat = 0;
	log_msg("\nreaddir has begun\n");
	if(strcmp(path,"/") != 0 )
	{
		return -1;
	}

	filler(buf,".",NULL,0);
	filler(buf,"..",NULL,0);

	char* disk = SFS_DATA->diskfile;
	char buffer[512];

	block_read(14,buffer);
	inode* rootDir = (inode*)buffer;
    //Search through all the direct map ptrs
    	int i=0;
    	for(i;i<15;i++) {
			log_msg("\nreaddir: inside first for loop %d\n",i);
    		//block num referenced by ptr
    		int blocknum = 0;
    		 blocknum = rootDir->blocks[i];
			log_msg("%d\n",blocknum);
    		//if valid ptr
    		if(blocknum > 14) {
			log_msg("entered 2nd for loop");
 	   			//read in inode
    			char buffer2[512];
    			block_read(blocknum,buffer2);
    			inode* tempNode=(inode*)buffer2;
			log_msg("\npath=\"%s\"\n",tempNode->filename);
    			//Compares paths for match

    				if (filler(buf, tempNode->filename, NULL, 0) != 0){
					log_msg("\nerror returned: ENOMEM. When inserting file with path:\"%s\"\n",tempNode->filename);
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
	char buffer[512];
	inode * root = (inode *) buffer;
	block_read(14,root);
	if(strcmp(path, root->filename) == 0){
		block_write(14,root);		
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
