#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#define MAX_CMD_LENGTH 20											// max no of arguments that can be passed through a command
#define MAX_NO_OF_CMDS 4

#define DEFAULT_INODE_COUNT 20
#define MAX_DB_COUNT 81920

#define MAX_BLOCK_SIZE 1024
#define MIN_BLOCK_SIZE 256
#define MAX_FILESYSTEM_SIZE 20
#define MIN_FILESYSTEM_SIZE 1

#define MAX_MOUNTS 5
#define DEFAULT_OSFILENAME_LENGTH 40

// structure to pass commands in different functions
typedef struct{
	int noOfArgs;													// stores the no of arguments in a command, max = MAX_NO_OF_CMDS
	char str[MAX_NO_OF_CMDS][MAX_CMD_LENGTH];						// stores the arguments in form of array of strings
}returnCmd;

// structure of super block
typedef struct{
	unsigned long int fs_size;					// 8
	unsigned int block_size;					// 4
	unsigned int inode_count;					// 4
	unsigned int db_count;						// 4
	unsigned int free_inode_count;				// 4
	unsigned int free_db_count;					// 4
	unsigned int db_start_loc;					// 4
	char inode_bm[DEFAULT_INODE_COUNT]; 		// 20
	char db_bm[MAX_DB_COUNT];					// 50000
}superBlock;

// structure of inode
typedef struct{
	unsigned char filename[20];
	unsigned int db_count;
	unsigned int db_start_loc;
	unsigned long int filesize;
}inode;

// structure of mounted files
typedef struct{
	unsigned char os_filename[DEFAULT_OSFILENAME_LENGTH];
	unsigned char drive_letter;
	void *fp;
	unsigned long int fs_size;
}mounts;
mounts mount_list[MAX_MOUNTS];
int last_mount_index;

// enumerable type for predicting and printing errors
enum errors{
	MKFS_ERR, USE_ERR, LS_ERR, CP_ERR, MV_ERR, RM_ERR, UM_ERR, DET_ERR,
	OSFL_OPERR, OSFL_RERR, OSFL_WERR
};

void printError(int errorNo)
{
	switch(errorNo){
		case MKFS_ERR:
			fprintf(stderr, "\t myfs: Wrong input for mkfs\n" );
			fprintf(stderr, "\t USAGE: mkfs [filename] [size of block in B] [size of file in MB]\n");
			break;
		case USE_ERR:
			fprintf(stderr, "\t myfs: Wrong input for use\n" );
			fprintf(stderr, "\t USAGE: use [src] [partition name]\n");
			break;
		case LS_ERR:
			fprintf(stderr, "\t myfs: Wrong input for ls\n" );
			fprintf(stderr, "\t USAGE: ls [path]\n");
			break;
		case CP_ERR:
			fprintf(stderr, "\t myfs: Wrong input for cp\n" );
			fprintf(stderr, "\t USAGE: cp [src] [destination]\n");
			break;
		case MV_ERR:
			fprintf(stderr, "\t myfs: Wrong input for mv\n" );
			fprintf(stderr, "\t USAGE: mv [src] [destination]\n");
			break;
		case RM_ERR:
			fprintf(stderr, "\t myfs: Wrong input for rm\n" );
			fprintf(stderr, "\t USAGE: rm [path]\n");
		case UM_ERR:
			fprintf(stderr, "\t myfs: Wrong input for umount\n" );
			fprintf(stderr, "\t USAGE: umount\n");
		case DET_ERR:
			fprintf(stderr, "\t myfs: Wrong input for det\n" );
			fprintf(stderr, "\t USAGE: det [mount_point]\n");
		case OSFL_OPERR:
			fprintf(stderr, "Failed in opening the file\n");
		case OSFL_RERR:
			fprintf(stderr, "Failed in opening the file for reading\n");
		case OSFL_WERR:
			fprintf(stderr, "Failed in opening the file for writing\n");

			break;

	}
}
returnCmd prepareCommand(char *buffer)												// returns the buffer after splitting the words and counting it
{
	int length = strlen(buffer);													// store length of buffer
	int i = 0;																		// track of index
	while(buffer[i] == ' ')															// traverse until first char is found, removes leading spaces before command
		i++;		

	returnCmd args;																	// structure to store return values
	int lenOfArg = 0;																// length of each argument													
	args.noOfArgs = 0;																// counts no of arguments entered by user
	int startPosOfArgs = 0;

	for (; i < length; ++i){														// traverse through all characters in buffer[]
		if(buffer[i] != ' ' && i<length-1){
			startPosOfArgs = i;
			lenOfArg = 1;
			i++;
			while(buffer[i] != ' ' && i<length-1){
				// count length of the argument
				i++;
				lenOfArg++;
			}
			if(args.noOfArgs == 4 || lenOfArg >= 20 ){
				// wrong input by user
				args.noOfArgs = -1;
				return args;
			}
			else{
				strncpy(args.str[args.noOfArgs], buffer+startPosOfArgs, lenOfArg);
				args.str[args.noOfArgs][lenOfArg] = '\0';
				args.noOfArgs++;
			}
		}
	}
	return args;
}

int checkWrongInput(returnCmd arg, int noOfArgs)									// checks if no of arguments is same as expected for corresponding command
{
	if(noOfArgs != arg.noOfArgs){													// if not then print error message
		fprintf(stderr, "\t myfs: Wrong input for %s \n", arg.str[0] );
		return -1;
	}
	else
		return 0;
}
void print_sblock( superBlock sblock)												// print all of the super block fields of the passed 'sblock'
{
	fprintf(stderr, " *************** SUPER BLOCK DETAILS ***************\n");
	fprintf(stderr, "         File System Size: %ld\n", sblock.fs_size);
	fprintf(stderr, "               Block Size: %d\n", sblock.block_size );
	fprintf(stderr, "         Super Block Size: %ld\n", sizeof(superBlock));
	fprintf(stderr, "               Inode Size: %ld\n", sizeof(inode));
	fprintf(stderr, "              Inode Count: %d\n", sblock.inode_count);
	fprintf(stderr, "         Data Block Count: %d\n", sblock.db_count);
	fprintf(stderr, "Data Block Start Location: %d\n", sblock.db_start_loc);
	fprintf(stderr, "         Free Inode count: %d\n", sblock.free_inode_count);
	fprintf(stderr, "            Free db count: %d\n", sblock.free_db_count);
	fprintf(stderr, " ***************************************************\n");

}
int mkfs(returnCmd arg)
{
	char *filename = arg.str[1];													// take inputs
	int blockSize, fileSize;
	sscanf(arg.str[2], "%d", &blockSize);
	sscanf(arg.str[3], "%d", &fileSize);

	// check constraints
	if(blockSize < MIN_BLOCK_SIZE || blockSize > MAX_BLOCK_SIZE){
		fprintf(stderr, "Block size not satisfied. Constraint: %d <= blockSize <= %d \n", MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
		return -1;
	}
	if(fileSize < MIN_FILESYSTEM_SIZE || fileSize > MAX_FILESYSTEM_SIZE){
		fprintf(stderr, "File size not satisfied . Constraint: %d <= fileSize <= %d\n", MIN_FILESYSTEM_SIZE, MAX_FILESYSTEM_SIZE );
		return -1;
	}

	// super block fields
	superBlock sblock;
	sblock.fs_size = fileSize*1024*1024;											// store file system size							
	sblock.block_size = blockSize;													// store block size
 	sblock.inode_count = DEFAULT_INODE_COUNT;										// store no of inodes as default value 
	sblock.db_count = (sblock.fs_size - (sizeof(superBlock) + sblock.inode_count*sizeof(inode))) / sblock.block_size;// store no of data blocks
	sblock.free_inode_count = sblock.inode_count;									// store free no of free inodes
	sblock.free_db_count = sblock.db_count;											// store no of free data blocks
	sblock.db_start_loc = sizeof(superBlock) + sizeof(inode)*sblock.inode_count;	// store offset value of starting loaction of data blocks
	for(int i=0; i<sblock.inode_count; i++)											// store byte map of free inodes
		sblock.inode_bm[i] = '0';
	for(int i=0; i<sblock.db_count; i++)											// store byte map of free data blocks
		sblock.db_bm[i] = '0';

	int fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0777);						// create os file
	if(fd == -1){																	// if creation failed then print error and return -1
		printError(OSFL_OPERR);
		return -1;
	}
	fprintf(stderr, "File %s opened for creation .\n", filename );

	fprintf(stderr, "%ld Bytes of Super Block written into file %s\n", write(fd,  &sblock, sizeof(superBlock)), filename);// write super block into the file
	print_sblock(sblock);
	close(fd);																		// close file
	if(truncate(filename, sblock.fs_size ) == -1){									// truncate the file to the total size of the file system provided by the user
		fprintf(stderr, "Failed in Truncate operation. File %s may lead to unknown failures.\n", filename);
	}																
	fprintf(stderr, "File %s Truncated to %ld Bytes.\n", filename, sblock.fs_size);
	return 0;
}
int unmount_all()
{
	// writes back in memory copy of opened file systems to corresponding osfiles
	// deletes exiting osfile and creates a new osfile of same name and writes into that osfile
	long int status;
	for(int i=0; i<last_mount_index; i++){
		status = unlink(mount_list[i].os_filename);
		int fd = open(mount_list[i].os_filename, O_WRONLY | O_CREAT | O_EXCL, 0777);
		status = write(fd, mount_list[i].fp, mount_list[i].fs_size);
		close(fd);
		memset( mount_list[i].os_filename, 0, strlen(mount_list[i].os_filename));
		mount_list[i].drive_letter = '\0';
		mount_list[i].fp = NULL;
		mount_list[i].fs_size = 0;
	}
	last_mount_index = 0;
	return 0;
}
int use(returnCmd arg)																// this function is used for mounting
{
	if(last_mount_index == MAX_MOUNTS - 1){											// check if max allowed files as mounted
		fprintf(stderr, "Max no of files are already mounted.\n" );
		return -1;																	// if yes then return -1
	}

	char *os_filename = arg.str[1];													// take inputs
	char drive_letter = toupper(arg.str[3][0]);

	
	for(int i=0; i<MAX_MOUNTS; i++){												// check if entered mount point is already used
		if(mount_list[i].drive_letter == drive_letter){
			fprintf(stderr, "Mount Point %c is already mounted with file %s\n", mount_list[i].drive_letter, mount_list[i].os_filename);
			return -1;																// if yes then print a message and return -1
		}
	}

	int fd = open(os_filename, O_RDONLY);											// open file to be mounted
	if(fd == -1){
		fprintf(stderr, "Error in opening : %s\n", os_filename);
		return -1;																	// if error then print message and return -1
	}
	
	superBlock sblock;																// read super block of file to be opened
	read(fd, &sblock, sizeof(superBlock));
	
	strcpy(mount_list[last_mount_index].os_filename, os_filename);					// store filename 
	mount_list[last_mount_index].drive_letter = drive_letter;						// store mount point
	mount_list[last_mount_index].fp = malloc(sblock.fs_size);						// allocate memory of size same as file system size
	memset(mount_list[last_mount_index].fp, 0, sblock.fs_size);						// set memory space to 0's
	lseek(fd, 0, SEEK_SET);															// in fd, seek to start position
	read(fd, mount_list[last_mount_index].fp, sblock.fs_size);						// copy whole os file in allocated memory pointed by fp
	mount_list[last_mount_index].fs_size = sblock.fs_size;							// store file system size that is mounted
	close(fd);

	fprintf(stderr, "%s of size %ld bytes is mounted at Mount Point %c \n", os_filename, sblock.fs_size, drive_letter);
	last_mount_index++;																// increase mount_list[] array index
}

int get_mount_index(char ch)														// returns index of mounted drive in mount_list[], specified by char ch 
{
	int dest_drive = -1;
	for(int i=0; i<MAX_MOUNTS; i++){
		if(mount_list[i].drive_letter == ch )
			dest_drive = i;
	}
	return dest_drive;
}

int get_db_start_loc_index(superBlock dest_sb, superBlock src_sb)					// returns staring data block of first free continuous space 
{
	int db_required = ceil(src_sb.fs_size / dest_sb.block_size);					// no of data blocks to store the source file in destined file system
	fprintf(stderr, "DB required: %d\n", db_required);
	fprintf(stderr, "Dest free db %d\n", dest_sb.free_db_count);
	if(db_required > dest_sb.free_db_count)											// if not enough free space in destined file system then return -1
		return -1;
	if(dest_sb.free_inode_count < 1)												// if no free inode in destined file system then return -1  	
		return -1;
	int count = 0;
	int db_start_loc_index = 0;
	for(int i=0; i<dest_sb.db_count; i++){											// check if db_required no of continuous data blocks are free or not
		if(dest_sb.db_bm[i] == '0'){												// if yes, then return the starting location of the first continuous free space
			if(count == 0)															// if no, then return -1 
				db_start_loc_index = i;
			count++;
			if(count == db_required)
				return db_start_loc_index;
		}
		else
			count = 0;
	}
	return -1;
}
int get_target_inode(int drive, char filename[])									// returns index of that inode which contains data blocks pointer 
{																					// 		of the specified file by 'filename' on mounted drive (specified by 'drive')
	superBlock sblock;
	inode inode_obj;
	int offset;
	memcpy( &sblock, mount_list[drive].fp, sizeof(superBlock));						// copy super block of file system to sblock 
	for (int i = 0; i < sblock.inode_count; i++){									// sequentially check inode byte-map 
		if(sblock.inode_bm[i] == '1'){												// 		if any inode is engaged and file pointed by that inode is same as 'filename'
			offset = sizeof(superBlock) + sizeof(inode)*i;							// calculate offset as inode location to be checked
			memcpy( &inode_obj, mount_list[drive].fp + offset, sizeof(inode));		// copy the inode to be checked to inodde_obj
			if( strcmp(inode_obj.filename, filename) == 0)							// Then return that inode index or return -1
				return i;
		}
	}
	return -1;																		// 6. if inode of 'filename' is not found then return -1
}

int cp(returnCmd arg)
{
	int alternative = -1;															// 3 types of possible valid combination of cp command
	if(arg.str[1][1] != ':' && arg.str[2][1] == ':')								// 0. src: osfile   & dest: testfile
		alternative = 0;
	else if(arg.str[1][1] == ':' && arg.str[2][1] == ':')							// 1. src: testfile & dest: testfile
		alternative = 1;
	else if(arg.str[1][1] == ':' && arg.str[2][1] != ':')							// 2. src: testfile & dest: osfile	
		alternative = 2;

	superBlock src_sb, dest_sb, temp_sblock;
	void *src_fp, *dest_fp;
	int src_fd, src_drive, src_inode_index, src_db_index, offset;
	int db_start_loc_index;
	int dest_drive;
	
	// Step 1. Bring the whole source file (whether osfile or testfile) to memory 
	// and keep it ready as a buffer for copying operation, in any alternative method
	// also read super block of source buffer and store it in src_sb
	if(alternative == 0){															// source is a osfile
		src_fd = open(arg.str[1], O_RDONLY);										// open source osfile
		if(src_fd == -1){
			fprintf(stderr, "Error in opening file %s.\n", arg.str[1]);
			return -1;
		}
		read(src_fd, &src_sb, sizeof(superBlock));									// read its super block
		src_fp = malloc(src_sb.fs_size);											// create a memory space of source file system size
		lseek(src_fd, 0, SEEK_SET);													// seek to starting location
		read(src_fd, src_fp, src_sb.fs_size);										// copy whole source osfile
		close(src_fd);																// close source osfile 
	}
	else if(alternative == 1 || alternative == 2){									// source is a testfile
		src_drive = get_mount_index(toupper(arg.str[1][0]));						// get source drive index in mount_list[]
		if(src_drive == -1){														// check if source drive is mounted or not 
			fprintf(stderr, "Source %s Not Found\n", arg.str[1]);					// if not then return -1
			return -1;
		}
		src_inode_index = get_target_inode(src_drive, arg.str[1]+2);				// get source file inode index in source drive
		if(src_inode_index == -1){													// check if source testfile is present in source drive or not
			fprintf(stderr, "Source %s Not Found\n", arg.str[1]);					// if not, then return -1	
			return -1;	
		}
		memcpy( &temp_sblock, mount_list[src_drive].fp, sizeof(superBlock));		// load super block of source drive
		inode inode_obj;															
		offset = sizeof(superBlock) + sizeof(inode)*src_inode_index;				// count offset to source inode in source drive
		memcpy( &inode_obj, mount_list[src_drive].fp + offset, sizeof(inode));		// store source inode in source drive	
		offset = temp_sblock.db_start_loc + inode_obj.db_start_loc*temp_sblock.block_size;// count offset to source data block in source drive
		src_fp = mount_list[src_drive].fp + offset;									// store src_fp as starting position of source data block in source drive
		memcpy( &src_sb, src_fp, sizeof(superBlock));								// copy source file super block
	}
	fprintf(stderr, "Source File System Size: %ld \n", src_sb.fs_size );

	// Step 2. 
	if(alternative == 0 || alternative == 1){										// destination is testfile
		dest_drive = get_mount_index(toupper(arg.str[2][0]));						// check if destination drive already mounted
		if(dest_drive == -1){
			fprintf(stderr, "Drive %c is not mounted.\n", toupper(arg.str[2][0]));	// if not then print error message and return -1
			return -1;
		}
		dest_fp = mount_list[dest_drive].fp;										// store mounted file system's pointer as dest_fp
		memcpy( &dest_sb,dest_fp, sizeof(superBlock) );								// load destination file system's super block
		fprintf(stderr, "Destination File System Size: %ld \n", dest_sb.fs_size );

		db_start_loc_index = get_db_start_loc_index(dest_sb, src_sb);				// if allocatable or not,
		if( db_start_loc_index == -1){												// if yes get starting data block index of destination 
			fprintf(stderr, "Source %s can not be copied into %s \n", arg.str[1], arg.str[2]);//from which src file can be copied to
			return -1;																// if not, then return -1
		}

		// on destination, modify super block
		dest_sb.free_inode_count --;												// decrease free inode count
		int db_required = ceil(src_sb.fs_size / dest_sb.block_size);				// calculate no of data blocks required to copy
		dest_sb.free_db_count -= db_required;										// decrease free data block count
		int target_inode_index = -1;												// denotes index of the inode to be modified
		for(int i=0; i<dest_sb.inode_count; i++){									// find first free inode
			if(dest_sb.inode_bm[i] == '0'){
				target_inode_index = i;
				break;
			}
		}
		dest_sb.inode_bm[target_inode_index] = '1';									// mark that inode as engaged
		for(int i = db_start_loc_index; i <= db_start_loc_index + db_required; i++)	// mark data blocks to be engaged
			dest_sb.db_bm[i] = '1';
		memcpy(dest_fp, (void*) &dest_sb, sizeof(superBlock));						// store destination file system's super block

		// on destination, modify 1st free inode
		inode target_inode;
		offset = sizeof(superBlock) + sizeof(inode)*target_inode_index;				// calculate offset to targeted inode
		memcpy( (void*) &target_inode, dest_fp+offset, sizeof(inode) );				// load targeted inode
		strcpy(target_inode.filename, arg.str[2]+2);								// store filename
		target_inode.db_count = db_required;										// store no od data blocks required
		target_inode.db_start_loc = db_start_loc_index;								// store starting data block index
		target_inode.filesize = src_sb.fs_size;										// store file size
		memcpy( dest_fp+offset, (void*) &target_inode, sizeof(inode));				// store targeted inode

		// on destination, modify data blocks
		offset = dest_sb.db_start_loc + dest_sb.block_size*db_start_loc_index;		// calculate offset to starting data block
		memcpy( dest_fp+offset, src_fp, src_sb.fs_size);							// store file to be copied from src_fp pointer
		fprintf(stderr, "Operation Done. %ld Bytes copied.\n", src_sb.fs_size );
	}
	else if(alternative == 2){														// destination is os file
		int fd = open(arg.str[2], O_WRONLY | O_CREAT | O_EXCL , 0777);				// create file to which intended to be copied
		if(fd == -1){																// if cannot create that file then return
			printError(OSFL_OPERR);
			return -1;
		}
		fprintf(stderr, "%ld Bytes Written to file %s Successfully\n", write(fd, src_fp, src_sb.fs_size), arg.str[2]);
		close(fd);
	}
	return 0;
}


int ls(returnCmd arg)																// prints files present in mounted file system 
{
	char mount_point = toupper( arg.str[1][0] );
	int dest_drive = get_mount_index(mount_point);
	if(dest_drive == -1){															// check if drive specified is mounted or not
		fprintf(stderr, "Drive %c is not Mounted.\n", toupper( arg.str[1][0] ) );	// if not, then print message and returns -1
		return -1;
	}

	superBlock sblock;
	inode inode_obj;
	int offset;
	memcpy( (void*) &sblock, mount_list[dest_drive].fp , sizeof(superBlock));		// copy super block of destined file system
	fprintf(stderr, "FILENAME \t SIZE \t\t Starting DB \t TOTAL DB\n");
	fprintf(stderr, "---------------------------------------------------------\n" );
	for(int i=0; i<sblock.inode_count; i++){										// sequentially check all inode in that file system
		if(sblock.inode_bm[i] == '1'){												// if that inode is engaged, them print the filename and its size
			offset = sizeof(superBlock) + sizeof(inode)*i;
			memcpy( (void*) &inode_obj, mount_list[dest_drive].fp+offset, sizeof(inode));
			fprintf(stderr, "%s \t\t %ld Bytes \t %d \t\t %d\n", inode_obj.filename, inode_obj.filesize, inode_obj.db_start_loc, inode_obj.db_count);
		}
	}
	return 0;
}

int rm(returnCmd arg)																// remove file (testfile)
{
	int src_drive = get_mount_index(toupper(arg.str[1][0]));						// get source drive index in mount_list[]
	if(src_drive == -1){															// check if source drive is mounted or not 
		fprintf(stderr, "Source %s Not Found\n", arg.str[1]);						// if not then return -1
		return -1;
	}
	void *src_fp = mount_list[src_drive].fp;
	int src_inode_index = get_target_inode(src_drive, arg.str[1]+2);				// get source file inode index in source drive
	if(src_inode_index == -1){														// check if source testfile is present in source drive or not
		fprintf(stderr, "Source %s Not Found\n", arg.str[1]);						// if not, then return -1	
		return -1;	
	}
	superBlock sblock;
	memcpy( &sblock, src_fp, sizeof(superBlock));									// load super block of source drive
	inode inode_obj;															
	int offset = sizeof(superBlock) + sizeof(inode)*src_inode_index;				// count offset to source inode in source drive
	memcpy( &inode_obj, src_fp + offset, sizeof(inode));							// load source inode in source drive	
	sblock.free_inode_count++;														// increase free inode count by one
	sblock.free_db_count += inode_obj.db_count;										// increase free data block count by no of data block the targeted file was using
	sblock.inode_bm[src_inode_index] = '0';											// mark engaged inode as free
	for(int i=inode_obj.db_start_loc; i<=inode_obj.db_start_loc+inode_obj.db_count; i++)// mark engaged data blocks as free
		sblock.db_bm[i] = '0';
	memcpy( src_fp, &sblock, sizeof(superBlock));									// store super block
	fprintf(stderr, "%s Deleted. %ld Bytes Freed\n", arg.str[1], inode_obj.filesize);
	return 0;
}

int mv(returnCmd arg)
{
	superBlock src_sb, dest_sb, temp_sblock;										// super block variables 
	void *src_fp, *dest_fp, *temp_fp;												// memory pointers
	int offset;										
	int src_drive = get_mount_index(toupper(arg.str[1][0]));						// get source drive index in mount_list[]
	if(src_drive == -1){															// check if source drive is mounted or not 
		fprintf(stderr, "Source %s Not Found\n", arg.str[1]);						// if not then return -1
		return -1;
	}
	temp_fp = mount_list[src_drive].fp;
	int src_inode_index = get_target_inode(src_drive, arg.str[1]+2);				// get source file inode index in source drive
	if(src_inode_index == -1){														// check if source testfile is present in source drive or not
		fprintf(stderr, "Source %s Not Found\n", arg.str[1]);						// if not, then return -1	
		return -1;	
	}
	memcpy( &temp_sblock, temp_fp, sizeof(superBlock));								// load super block of source drive
	offset = sizeof(superBlock) + sizeof(inode)*src_inode_index;					// count offset to source inode in source drive
	inode inode_obj;															
	memcpy( &inode_obj, temp_fp + offset, sizeof(inode));							// store source inode in source drive	
	offset = temp_sblock.db_start_loc + inode_obj.db_start_loc*temp_sblock.block_size;// count offset to source data block in source drive
	src_fp = temp_fp + offset;														// store src_fp as starting position of source data block in source drive
	memcpy( &src_sb, src_fp, sizeof(superBlock));									// copy source file super block
	
	if(toupper(arg.str[1][0]) == toupper(arg.str[2][0]) ){							// mv operation within same file system
		memset( inode_obj.filename, 0, strlen(inode_obj.filename));					// set filename of targeted inode as blank
		strcpy( inode_obj.filename, arg.str[2]+2);									// rename filename of targeted inode
		memcpy( temp_fp+offset, &inode_obj, sizeof(inode));							// store targeted inode
		fprintf(stderr, "Operation Done. %ld Bytes Moved.\n", src_sb.fs_size );
	}
	else{																			// mv operation between two file systems
		int dest_drive = get_mount_index(toupper(arg.str[2][0]));					// check if destination drive already mounted
		if(dest_drive == -1){
			fprintf(stderr, "Drive %c is not mounted.\n", toupper(arg.str[2][0]));	// if not then print error message and return -1
			return -1;
		}
		void *dest_fp = mount_list[dest_drive].fp;									// store mounted file system's pointer as dest_fp
		memcpy( &dest_sb, dest_fp, sizeof(superBlock) );							// load destination file system's super block
		fprintf(stderr, "Destination File System Size: %ld \n", dest_sb.fs_size );

		int db_start_loc_index = get_db_start_loc_index(dest_sb, src_sb);			// if allocatable or not,
		if( db_start_loc_index == -1){												// if yes get starting data block index of destination 
			fprintf(stderr, "Source %s can not be copied into %s \n", arg.str[1], arg.str[2]);//from which src file can be copied to
			return -1;																// if not, then return -1
		}

		// on destination, modify super block
		dest_sb.free_inode_count --;												// decrease free inode count
		int db_required = ceil(src_sb.fs_size / dest_sb.block_size);				// calculate no of data blocks required to copy
		dest_sb.free_db_count -= db_required;										// decrease free data block count
		int target_inode_index = -1;												// denotes index of the inode to be modified
		for(int i=0; i<dest_sb.inode_count; i++){									// find first free inode
			if(dest_sb.inode_bm[i] == '0'){
				target_inode_index = i;
				break;
			}
		}
		dest_sb.inode_bm[target_inode_index] = '1';									// mark that inode as engaged
		for(int i = db_start_loc_index; i <= db_start_loc_index + db_required; i++)	// mark data blocks to be engaged
			dest_sb.db_bm[i] = '1';
		memcpy(dest_fp, (void*) &dest_sb, sizeof(superBlock));						// store destination file system's super block

		// on destination, modify 1st free inode
		inode target_inode;
		offset = sizeof(superBlock) + sizeof(inode)*target_inode_index;				// calculate offset to targeted inode
		memcpy( (void*) &target_inode, dest_fp+offset, sizeof(inode) );				// load targeted inode
		strcpy(target_inode.filename, arg.str[2]+2);								// store filename
		target_inode.db_count = db_required;										// store no od data blocks required
		target_inode.db_start_loc = db_start_loc_index;								// store starting data block index
		target_inode.filesize = src_sb.fs_size;										// store file size
		memcpy( dest_fp+offset, (void*) &target_inode, sizeof(inode));				// store targeted inode

		// on destination, modify data blocks
		offset = dest_sb.db_start_loc + dest_sb.block_size*db_start_loc_index;		// calculate offset to starting data block
		memcpy( dest_fp+offset, src_fp, src_sb.fs_size);							// store file to be copied from src_fp pointer
		fprintf(stderr, "Operation Done. %ld Bytes Moved.\n", src_sb.fs_size );
		temp_sblock.free_inode_count++;												// increase free inode count by one of source file system's super block
		temp_sblock.free_db_count += inode_obj.db_count;							// increase free data block count by no of data block the targeted file was using
		temp_sblock.inode_bm[src_inode_index] = '0';								// mark engaged inode as free
		for(int i=inode_obj.db_start_loc; i<=inode_obj.db_start_loc+inode_obj.db_count; i++)// mark engaged data blocks as free
			temp_sblock.db_bm[i] = '0';
		memcpy( temp_fp, &temp_sblock, sizeof(superBlock));							// store super block
	}
	return 0;
}

int det(returnCmd arg)
{
	superBlock sblock;
	int dest_drive = get_mount_index(toupper(arg.str[1][0]));
	void *dest_fp = mount_list[dest_drive].fp;
	memcpy( &sblock, dest_fp, sizeof(superBlock));
	fprintf(stderr, " ***************************************************\n");
	fprintf(stderr, "       Super Block Details of Drive %c \n", toupper(arg.str[1][0]));
	print_sblock(sblock);
	return 0;
}

int main(int argc, char const *argv[])
{
	char buffer[100];
	returnCmd arg;
	while(1){
		// infinite loop until user decides to exit manually
		
		printf("\n- mkfs> ");
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);
		arg = prepareCommand(buffer);
		
		if(arg.noOfArgs == -1){														// wrong input entered by user
			fprintf(stderr, "\t myfs: ERROR: Wrong Input\n" );
			continue;
		}
		else if(strcmp(arg.str[0], "exit") == 0 && arg.noOfArgs == 1){				// user decided to exit
			unmount_all();
			exit(0);
		}
		else if( strcmp(arg.str[0], "mkfs") == 0 ){									// mkfs command
			if(arg.noOfArgs != 4){
				printError(MKFS_ERR);
				continue;
			}
			mkfs(arg);
		}
		else if( strcmp(arg.str[0], "use") == 0 ){									// use command
			if(arg.noOfArgs != 4){
				printError(USE_ERR);
				continue;
			}
			use(arg);
		}
		else if( strcmp(arg.str[0], "cp") == 0){									// copy command
			if(arg.noOfArgs != 3){
				printError(CP_ERR);
				continue;
			}
			cp(arg);
		}
		else if( strcmp(arg.str[0], "ls") == 0){									// ls command
			if(arg.noOfArgs != 2){
				printError(LS_ERR);
				continue;
			}
			ls(arg);
		}
		else if( strcmp(arg.str[0], "rm") == 0){									// remove command
			if(arg.noOfArgs != 2){
				printError(RM_ERR);
				continue;
			}
			rm(arg);
		}
		else if( strcmp(arg.str[0], "mv") == 0){									// move command
			if(arg.noOfArgs != 3){
				printError(MV_ERR);
				continue;
			}
			mv(arg);
		}
		else if( strcmp(arg.str[0], "umount") == 0){								// unmount command
			if(arg.noOfArgs != 1){
				printError(UM_ERR);
				continue;
			}
			unmount_all();
		}
		else if( strcmp(arg.str[0], "det") == 0){									// details command
			if(arg.noOfArgs != 2){
				printError(DET_ERR);
				continue;
			}
			det(arg);
		}
		else{
			fprintf(stderr, "\t myfs: COMMAND NOT FOUND: %s\n", arg.str[0]);
		}
	}
	return 0;
}