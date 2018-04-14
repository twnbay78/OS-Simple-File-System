#include<"File.h">
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


void U_Fullpath(char filePath[MAX_PATH], const char* path)
{
	strcpy(filePath,inode->rootDir);
	strncat(filePath,path,MAX_PATH);
}

int U_getattr(const char *path, struct stat *statS)
{
	//int return_status; check to make sure it is written to a log
	char filePath[MAX_PATH];
	//maybe log something

 	U_Fullpath(filePath,path);

 	return 0;
}
int U_readdir(const char *path, void *buf, fuse_fill_dirt filler, 
	off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *Directory;

	dp = (DIR*) (uintptr_t) fi->fh;

	Directory = readdir(dp);
		if(Directory == 0)
		{
			return -1;
		}

	do{
		if (filler(buf,Directory->d_name,NULL,0) != 0)
		{
			return -1;
		}

	}while((Directory = readdir(dp)) != NULL);

}