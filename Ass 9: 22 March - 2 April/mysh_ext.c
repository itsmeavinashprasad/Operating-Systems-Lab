#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PROMPT "mysh_>"															// prompt
#define MAX_CMD_LENGTH 30														// max length of the command
#define MAX_NO_ARGS 15															// max no of arguments in the input string
#define debug_print 0															// set 1 to print all debug messages
#define HIGHLIGHT "\033[44;1;4m"												// escape seq for background highlighting
#define RESET "\033[0m"															// escape seq for resetting formatting
#define READ_END 0																// read end of the pipe descriptor
#define WRITE_END 1																// write end of the pipe descriptor


typedef struct 																	// structure to return given string in a array of strings
{
	int no_of_cmds;																// no of elements in that array
	char cmds[MAX_NO_ARGS][MAX_CMD_LENGTH];										// array of strings
}commands;

commands prepare_commands(char *buffer)											// eliminates extra spaces and packs given strings into
{																				// array of strings in 'command' structure
	commands arg;
	int cmd_cnt = -1;
	int i = 0, j = 0;
	while(i < strlen(buffer) -1 ){												// traverse to last character of the input given
		if(buffer[i] != ' '){
			cmd_cnt ++;
			while(buffer[i] != ' ' && i<strlen(buffer)-1 ){
				arg.cmds[cmd_cnt][j] = buffer[i];
				i++;
				j++;
			}
			arg.cmds[cmd_cnt][j] = '\0';
			j=0;
		}
		i++;
	}
	arg.no_of_cmds = cmd_cnt;
	return arg;
}

int do_childish_task( commands arg, int start_index, int end_index)				// packs the strings given from 'start index' to 'end index'
{																				// in a array of strings named 'buffer', then passes tat to execve()
	int no_of_cmds = end_index - start_index + 2;								// no of commands to be passed to execve()
	char **buffer;																// pointer to pointer of characters
	buffer = (char **)malloc( no_of_cmds * sizeof(char*) );						// initialize size
	for(int i=0; i<no_of_cmds; i++){											// initialize and pack
		buffer[i] = (char*) malloc(strlen( arg.cmds[ start_index+i ]));
		strcpy(buffer[i] , arg.cmds[ start_index + i] );
	}
	buffer[no_of_cmds-1] = NULL;												// enter last element as NULL

	int status = execvp(buffer[0], buffer);
	int err_no = errno;															// store errno set by the execve() system call		
	if(debug_print)
		fprintf(stderr, "External Executable not Executed Properly %d, ERRNO: %d\n", status , err_no);
	perror(arg.cmds[start_index]);
	exit(err_no);																// return error no for further considerations
}

char* pwd()																		// function corresponding to pwd command
{																				// returns path of working directory as character pointer
	char *str = NULL;
	str = getcwd(str, 0);														// char *getcwd(char *buf, size_t size), system call 
	if(str == NULL)
		perror("pwd");
	return str;
}

int cd(char *str)																// function corresponding to cd command
{
	char path[strlen(str)+1];													
	strncpy(path, str, strlen(str));											
	path[strlen(str)] = '\0';													// int chdir(const char *path), returns -1 on failure
	int status =  chdir(path);												 	// return return status of chdir()
	if(status != 0)
		perror("cd");
	return status;
}

bool is_this(char *str1, char *str2)											// wrapper function for string comparison
{
	if(strcmp(str1, str2) == 0)
		return true;
	else
		return false;
}
bool if_connector(char *str)													// returns true if the given string is a connector
{																				// i.e. if this is || or &&
	if( is_this(str , "||"))
		return true;
	else if( is_this(str, "&&") )
		return true;
	else
		return false;
}

bool if_internal_cmd( char *str)												// returns true if the given input is a internal command
{
	if(is_this(str, "pwd"))
		return true;
	else if( is_this(str, "cd") )
		return true;
	else if( is_this( str, "clear") )
		return true;
	else
		return false;
}

int process_commands_last(commands arg, int si, int ei)							// last hierarchy function for processing commands 
{
	int status;
	bool if_internal_cmd_flag = if_internal_cmd(arg.cmds[si]);					// true if given input is a internal cmd
	if(debug_print)	
		fprintf(stderr, "In PCL. SI: %d EI: %d \n",si, ei );
	if(debug_print)
		fprintf(stderr, "cmd: '%s' type: %d\n", arg.cmds[si], if_internal_cmd_flag);
	if(if_internal_cmd_flag){													// enters whenever it is a internal command, executes in parent process
		if(strncmp(arg.cmds[si], "pwd", 3) == 0){								// if cmd is "pwd"
			char *path = pwd();
			if(path == NULL)
				status = -1;
			else{
				fprintf(stdout, "Working Dir: %s\n", path);
				status = 0;
			}
		}
		if( strncmp(arg.cmds[si], "cd", 2) == 0){								// if cmd is "cd"
			status = cd(arg.cmds[si + 1]);
		}
		if(strcmp(arg.cmds[si], "clear") == 0){									// if cmd is "clear"
			fprintf(stderr, "\033[H\033[J" );
		}
	}
	else{																		// enters whenever it is a external command
		pid_t pid = fork();														// creates a child process for execution and parents waits for child's termination
		if(pid == 0){
			// child process
			do_childish_task(arg, si, ei);										// executes child process's task
		}
		else{	
			waitpid(pid, &status, 0);											// parent waits for child's termination
			if(debug_print)
				fprintf(stderr, "Status in parent : %d\n", status);
		}
	}
	return status;																// return status of executed command
}


int process_commands_l4(commands arg, int si, int ei)							// level 4 hierarchy of process execution,
{																				// break into IO operations
	char *cur_arg;
	int status;
	int IO_count = 0;															// counts number of io operators
	int IO_type ; // 0. '>'  1. '2>'  2. '<'  									// type of io operator
	int IO_index;																// index of io operator
	for (int i = si; i <+ ei; i++){												// parse given input for setting variables declared
		cur_arg = arg.cmds[i];
		if(is_this(cur_arg, ">") ){
			IO_count++;
			IO_type = 0;
			IO_index = i;
		}
		else if( is_this(cur_arg, "2>")){
			IO_count++;
			IO_type = 1;
			IO_index = i;
		}
		else if( is_this(cur_arg, "<")){
			IO_count++;
			IO_type = 2;
			IO_index = i;
		}
	}

	if(IO_count == 0){															// if no io operator is present then jump to next hierarchy
		// no IO operator														
		status = process_commands_last(arg, si, ei);
		return status;															// return status from previous hierarchy
	}
	else if(IO_count > 1){														// if more than one io operators present in given input
		fprintf(stderr, "More than one IO operator not supported.\n" );			// then this program can not handle this, so returns -1
		return -1;
	}

	pid_t cpid ;
	cpid = fork();
	if(cpid == 0){
		// child
		char *filename = arg.cmds[ei];
		int fd;
		if(IO_type == 0 || IO_type == 1){										// true in case of output redirection
			fd = open( filename, O_WRONLY | O_CREAT | O_TRUNC, 0777 );		// open file, if not present then create
			if(fd == -1){														// if cant open then returns -1
				fprintf(stderr, "Error in opening file %s\n", filename);
				exit(EXIT_FAILURE);
			}
			if(IO_type == 0){													// if ">" operator is used then only replace STDOUT
				if( dup2( fd, STDOUT_FILENO) == -1){							// with the file descriptor
					fprintf(stderr, "dup2() failed. \n" );						// call dup2()
					exit(EXIT_FAILURE);
				}
			}
			else if( IO_type == 1){												// if "2>" operator is used then only replace STDERR
				if( dup2( fd, STDERR_FILENO) == -1){							// with file descriptor
					fprintf(stderr, "dup2() failed. \n" );						// call dup2()
					exit(EXIT_FAILURE);
				}	
			}
		}
		else if(IO_type == 2){													// true in case of input redirection
			fd = open(filename, O_RDONLY);										// open given file
			if(fd == -1){														// if open() fails or file not present, then return -1
				fprintf(stderr, "Error in opening file %s\n", filename);
				exit(EXIT_FAILURE);
			}
			if( lseek( fd, 0, SEEK_SET) == -1){									// seek to start of file
				fprintf(stderr, "Seek failure in file %s\n", filename);
				exit(EXIT_FAILURE);
			}
			if( dup2(fd, STDIN_FILENO) == -1){									// call dup2()
				fprintf(stderr, "dup2() failed\n" );
				exit(EXIT_FAILURE);
			}
		}
		status = process_commands_last(arg, si, IO_index-1);					// after this, go to next hierarchy of process commands
		exit(status);															// exit with return status
	}
	else{
		// parent
		waitpid(cpid, &status, 0);												// parent waits for child
		return status;															// returns status
	}
}

int process_commands_l3(commands arg, int si, int ei)							// level 3 hierarchy of process execution,
{																				// break into "|" operator
	int start_index = si;
	int status;
	char *cur_arg;

	int pipe_count = 0;															// stores no of | operators
	if(debug_print)
		fprintf(stderr, "In PC3. SI: %d EI: %d \n",si, ei );
	for (int i = si; i <= ei; ++i){
		cur_arg = arg.cmds[i];
		if( is_this( cur_arg, "|") )
			pipe_count++;														
	}

	if(pipe_count == 0){														// if no pipe found then jump to next hierarchy
		// no pipes found
		if(debug_print)
			fprintf(stderr, "No Piped Found.\n" );
		status = process_commands_l4(arg, si, ei);								// call next level of hierarchy
		return status;															// return status
	}		

	if(debug_print)
		fprintf(stderr, "Pipes found: %d\n", pipe_count);
	int pipefd[pipe_count][2];													// declare pipe descriptors
	for (int i = 0; i < pipe_count; ++i){
		if( pipe(pipefd[i]) == -1){												// implement pipe() call
			fprintf(stderr, "pipe() call failed. Terminating.\n" );
			exit(EXIT_FAILURE);
		}
	}
	int pipe_index = 0;
	pid_t cpid;
	for(int i = si; i <= ei; i++){
		cur_arg = arg.cmds[i];
		if(debug_print)
			fprintf(stderr, "Current Arg: %s\n", cur_arg);
		if( is_this(cur_arg, "|")){												// whenever | found executes until previous argument
			if(pipe_index == 0){												// if first pipe then cmd reads from STDIN and writes to pipe's write end
				cpid = fork();
				if(cpid == 0){
					close( pipefd[0][READ_END]);								// close 0-th pipe's read end
					if( dup2( pipefd[0][WRITE_END], STDOUT_FILENO) == -1){		// pipe[i][write-end] <- stdout
						fprintf(stderr, "dup2() failed for pipe index %d\n", pipe_index);
						exit(EXIT_FAILURE);
					}
					status = process_commands_l4(arg, start_index, i-1);		// execute sub cmd
					exit(status);												// exit with return status
				}
				else{
					close(pipefd[0][WRITE_END]);								// parent closes its copy of 0-h pipe's
					waitpid(cpid, &status, 0);									// parent waits for its child
				}
			}

			if( pipe_index != 0){												// if not first | operator, then read from previous 
				cpid = fork();													// indexed pipe, and write to current indexed pipe
				if(cpid == 0){
					if( dup2( pipefd[pipe_index-1][READ_END], STDIN_FILENO) == -1){// pipe[i-1][read-end] <- stdin
						fprintf(stderr, "dup2() failed for pipe index %d\n", pipe_index);
						exit(EXIT_FAILURE);
					}
					if( dup2( pipefd[pipe_index][WRITE_END], STDOUT_FILENO) == -1){// pipe[i][write-end] <- stdout
						fprintf(stderr, "dup2() failed for pipe index %d\n", pipe_index);
						exit(EXIT_FAILURE);
					}
					status = process_commands_l4(arg, start_index, i-1);
					exit(status);
				}
				else{
					close(pipefd[pipe_index-1][READ_END]);						// close unused pipe ends	
					close(pipefd[pipe_index][WRITE_END]);
					waitpid(cpid, &status, 0);									// parent waits
				}
			}
			pipe_index ++;
			start_index = i+1;
		}
		else if(i == ei){														// true when it is the last sub command
			cpid = fork();
			if(cpid == 0){
				close(pipefd[pipe_count-1][WRITE_END]);
				if( dup2( pipefd[pipe_count-1][READ_END], STDIN_FILENO) == -1){	// pipe[last][read-end] <- stdin
					fprintf(stderr, "dup2() failed for pipe index %d\n", pipe_index);
					exit(EXIT_FAILURE);
				}
				status = process_commands_l4(arg, start_index, i);				// call next level hierarchy
				exit(status);
			}
			else{
				close(pipefd[pipe_count-1][READ_END]);
				waitpid(cpid, &status, 0);										// parent waits
			}
		}
	}
	return status;
}
int process_commands_l2(commands arg, int si, int ei)							// level 3 hierarchy of process execution,
{																				// break into logical connectors
	int start_index = si;		
	int end_index;
	int status;																	// stores return status of last executes sub command
	bool if_to_be_executed = true;												// stores flag if next command should be executed or not
	char *cur_arg;
	int last_connector = -1;													// stores the type of the last logical connector; 0. "||"; 1."&&"
	if(debug_print)
		fprintf(stderr, "In PC2. SI: %d EI: %d \n",si, ei );
	for(int i = si; i<=ei; i++){												// parse all arguments
		cur_arg = arg.cmds[i];													
		if(debug_print)
			fprintf(stderr, "Current Arg: %s\n", cur_arg);
		if( if_connector(cur_arg) || i == ei){									// enters whenever current argument is a connector or last argument

			if(!if_to_be_executed){												// enters whenever this sub command 
				if(debug_print)
					fprintf(stderr, "Skipping: %s \n", arg.cmds[start_index]);

				if(last_connector == 0 && is_this(cur_arg,"||"))				// if last connector is ||, and this cmd should not be executed(last 
					if_to_be_executed = false;									// status was TRUE), then again
				else if( last_connector == 1 && is_this(cur_arg, "&&"))			// if this connector is also || then also dont execute next cmd
					if_to_be_executed = false;									// else if this connector is && then execute next cmd
				else
					if_to_be_executed = true;

				if( is_this(cur_arg, "||"))										// store last_connector type accordingly
					last_connector = 0;
				else if( is_this(cur_arg, "&&"))
					last_connector = 1;
				start_index = i+1;
				continue;														// also skip this sub command
			}

			if( i == ei)
				end_index = i;
			else
				end_index = i-1;
			status = process_commands_l3(arg, start_index, end_index);			// go to next level hierarchy
			if(debug_print)
				fprintf(stderr, "Status: %d for cmd: %s\n", status, arg.cmds[start_index]);
			if( is_this(cur_arg, "||")){										// if current connector is || , 
				last_connector = 0;												// then store last_connector
				if(status == 0)													// if status of last sub cmd is 0, then next sub cmd should be skipped
					if_to_be_executed = false;
			}
			else if( is_this( cur_arg, "&&") ){									// if current connector is &&
				last_connector = 1;												// then store last_connector
				if(status != 0)													// if status of last sub cmd is not 0, them next sub cmd should be skipped
					if_to_be_executed = false;
			}
			start_index = i+1;
		}
	}
	return status;

}
int process_commands_l1(commands arg, int si, int ei)							// level 1 hierarchy of process commands 			
{																				// break into finding ;
	int start_index = si;
	int end_index;
	int status;
	char *cur_arg;
	if(debug_print)
		fprintf(stderr, "In PC1. SI: %d EI: %d \n",si, ei );
	for(int i = si; i<=ei; i++){												// parse all arguments
		cur_arg = arg.cmds[i];
		if(debug_print)
			fprintf(stderr, "Current Arg: %s\n", cur_arg);
		if( is_this(cur_arg, ";") || i == ei){									// if current argument is ; or last argument 
			if(i == ei)
				end_index = i;
			else
				end_index = i-1;

			status = process_commands_l2(arg, start_index, end_index);			// go to next level hierarchy
			if(debug_print)
				fprintf(stderr, "Status: %d for cmd: %s\n", status, arg.cmds[start_index]);
			start_index = i+1;
		}
	}
	return status;
}

int main(int argc, char const *argv[])
{
	char buffer[100];
	commands arg;
	while(true){
		fprintf(stderr,"\n%s%s/%s%s ", HIGHLIGHT, pwd(), PROMPT, RESET );

		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);

		arg = prepare_commands(buffer);											// break buffer into array of strings
		if(arg.no_of_cmds == -1)												// if buffer is empty, just continue
			continue;

		if(strcmp(arg.cmds[0] , "exit") == 0){									// if exit is entered, then simply exit
			fprintf(stdout, "Exiting ... \n" );
			exit(0);
		}

		process_commands_l1(arg, 0, arg.no_of_cmds);							// for all other commands, go to process_commands() 
	}	
	return 0;
}