# OS---Simple-File-System
Implementing a very simple file system in USERspace using the FUSE library

REQUIREMENTS:
- 16MB MAX files
- 128 files indexable

STRUCTURE OF FS

blocks:
- can specify block size 

superblocks:
- store file descriptors
- store tables

FUNCTIONS

You should implement the following system calls at minimum:

0. int create(char* path)
- Allocate disk space 
	- check disk quotas
	- check perms
- Create a file descriptor for the file 
	- name
	- location on disk
	- filename
	- permissions
	- size
	- (filetype)
1. int delete(char* path)
	- find directory containing the file
	- Free the disk blocks used by the file
	- remove the file descriptor from the directory
	- adjust structures
2. int stat(char* path, struct stat* buf)
	- obtain information of file (read file descriptor) 
	- write info to buf
3. int open(char* path)
	- Check if file is already open by another proc
		NOT
		- find file
		- copy file descriptor into the system-wide open file table
	- check perms
	- increment open count
	- create entry in proc's file table pointing to the entry in the system wide file table
	- initialize file ptr to the start of the file
4. int close(int fileID)
	- remove the entry for the file in the proc's file table
	- decriment open count
	- if opencount == 0, remove the entry in the system-wide file table
5. int read(int fileID, void* buffer, int bytes)
	RANDOM
	- OS reads (size" bytes fsrom file position "from" into "bufAddress"
		for(i = from; i < from + size; i++){
			bufAddress[i-from] = file[i];
		}
	SEQUENTIAL
	- OS reads "size bytes" from current fp into bufAddress and increments the current file position by size
		for(i = 0; i < size; i++){
			bufAddress[i] = file[fp + i];
		}
		fp +- size;
6. int write(int fileID, void* buffer, int bytes)
	- like reads, but copies from buffer to the file
7. struct dirent* readdir(int directoryID)
FUSE
8. init()


9. destroy()

ADDITIONAL MECHANISMS
- single and double indirection pointers
10. int opendir(char* path)
11. int closedir(int directoryID)
12. int mkdir(char* path)
13. int rmdir(char* path)

STRUCTURES

File Descriptor Table (stores all file descriptors on disk) (iNode)
- metadata:
	- filename
	- size
	- permissions


Active Page Table (used for open files - in mem): table for all open files with ptr to proc file table
- loaded in on open

Proc Page Table: table for all files owned by procs

Free space list:
- basic approach -> bitmap
	- 1 bit for every block in the FS
	- Used or not used
	- take 32 bits, compare it to 0, will tell you if there is a free page or not 

- LL of free blocks 
	- better because bitmaps can become quite large





-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-


INODE STRUCTURE
- number
- 12 data ptrs
- 1 single indirection ptr
- 1 double indirection ptr

IMPLEMENTATIONS

Contiguous Allocation: Allocate the start chunk and # of blocks

+ Simple
+ Number of seeks is 1 - good seek time for sequential and random access (just START + OFFSET for block_i)
+/- Access time is standard (depends on where the drive arm is - can optimize)
-- If filesize changes, well.... you can't change it
-- Fragmentation is BADDDD

LL Implementation: LL of block ptrs

+ No fragmentation bc data is not sequential
+ file size can grow
- sequential and random read/write take n seeks and n accesses

Indexed files: array of block ptrs

+ Good random time because you don't have to traverse previous blocks
+ still not much fragmentation
+/- specify file size
- still n seek times and n access times b/c blocks are not contiguous
- if filesize grows, might have to copy large amounts of metadata
- large files = large metadata (file descriptors) -> need max filesize

Multilevel Index: ptrs to ptrs to data (single indirection) and ptr to ptr to ptrs of data (double indirection)
+ Because of indirection, you don't need a max file type
+ Saves metadata space because you are only storing indexies you need (metadata grows with filesize)
+/- Seek time still n
+/- Access time still n
- expensive to access data in large files
- still lots of seeks because data is not contiguous

-> typically 14 block ptrs
	- 12 ptrs to data
	- 13th ptr pots to 1024 ptrs of data blocks (one direction)
	- 14th ptr pointrs to block of ptrs to indirect blocks
