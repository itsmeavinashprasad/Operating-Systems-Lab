#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_CMD_LENGTH 20											// max no of arguments that can be passed through a command
#define MAX_NO_OF_CMDS 4

#define DEFAULT_INODE_COUNT 40
#define MAX_DB_COUNT 50000

#define MAX_BLOCK_SIZE 1024
#define MIN_BLOCK_SIZE 256
#define MAX_FILESYSTEM_SIZE 20
#define MIN_FILESYSTEM_SIZE 1

#define MAX_MOUNTS 5
#define DEFAULT_OSFILENAME_LENGTH 40

// structure of super block
typedef struct{
	long int fs_size;
	int block_size;
	int inode_count;
	int db_count;
	int free_inode_count;
	int free_db_count;
	int db_start_loc;
	bool inode_bm[DEFAULT_INODE_COUNT];
	bool db_bm[MAX_DB_COUNT];
}superBlock;

// structure of inode
typedef struct{
	char filename[20];
	int db_count;
	int db_start_loc;
}inode;
int main()
{
	superBlock sblock;
	int fd = open("os2", O_RDONLY);
	int offset = 0;
	read(fd, &sblock, sizeof(superBlock));
	fprintf(stderr, "File System Size: %ld\n", sblock.fs_size);
	fprintf(stderr, "      Block Size: %d\n", sblock.block_size );
	fprintf(stderr, "Super Block Size: %ld\n", sizeof(superBlock));
	fprintf(stderr, "      Inode Size: %ld\n", sizeof(inode));
	fprintf(stderr, "     Inode Count: %d\n", sblock.inode_count);
	fprintf(stderr, "Data Block Count: %d\n", sblock.db_count);

	offset = sizeof(superBlock) + sizeof(inode) * 0;
	fprintf(stderr, "Reading inode from offset %d \n", offset);
	inode obj;
	lseek(fd, offset, SEEK_SET);
	read(fd, &obj, sizeof(inode));
	fprintf(stderr, "Filename: %s\n", obj.filename);
	fprintf(stderr, "DB count: %d\n", obj.db_count);
	fprintf(stderr, "db start Loc: %d \n", obj.db_start_loc ); 

	offset = sblock.db_start_loc;
	fprintf(stderr, "Reading copied sblock from offset %d \n", offset);
	lseek(fd, offset, SEEK_SET);
	read(fd, &sblock, sizeof(superBlock));
	fprintf(stderr, "File System Size: %ld\n", sblock.fs_size);
	fprintf(stderr, "      Block Size: %d\n", sblock.block_size );
	fprintf(stderr, "Super Block Size: %ld\n", sizeof(superBlock));
	fprintf(stderr, "      Inode Size: %ld\n", sizeof(inode));
	fprintf(stderr, "     Inode Count: %d\n", sblock.inode_count);
	fprintf(stderr, "Data Block Count: %d\n", sblock.db_count);
}