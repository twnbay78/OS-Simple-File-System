#ifndef _File_H_
#define _File_H_
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define BLOCK_SIZE 4096
#define MAX_PATH 16000
typedef struct _metaData
{
		char *rootDir;
}metaData;

#endif 
