#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "structures.h"
#define SetBit(A,k)     ( A[(k/32)] |= (1 << (k%32)) )
#define ClearBit(A,k)   ( A[(k/32)] &= ~(1 << (k%32)) )
#define TestBit(A,k)    ( A[(k/32)] & (1 << (k%32)) )
int main(void){
	printf("sizeof inode: %d\n", sizeof(inode));
	int array[1024];
	int i;
	for(i = 0; i < 1024; ++i){
		array[i] = 0;
	}
	for(i = 0; i < 1024; ++i){
		printf("array vals: %d\n", array[i]);
	}
	printf("size of bitmap: %d\n", sizeof(array));

	if(TestBit(array, 1))
		printf("BIT 1 SET\n");
	SetBit(array, 1);
	if(TestBit(array, 1))
		printf("BIT 1 SET AFTER SET\n");
	ClearBit(array, 1);
	if(!TestBit(array, 0))
		printf("BIT 1 NOT SET AFTER CLEAR\n");
}
