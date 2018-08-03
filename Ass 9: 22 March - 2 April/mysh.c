#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define PROMPT "mysh_>"
#define MAX_CMD_LENGTH 30
#define MAX_NO_ARGS 15
#define DEFAULT_ENVIRONMENT 0
#define debug_print 0
#define HIGHLIGHT "\033[44;1;4m"
#define RESET "\033[0m"

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

	char *env[] = {"/bin", NULL};
	int status;
	if(DEFAULT_ENVIRONMENT)
		status = execve(buffer[0], buffer, env);								// call execve(), if not executed properly then get return status as 'status'
	else
		status = execve(buffer[0], buffer, NULL);
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
	int status =  chdir(path);												 		// return return status of chdir()
	if(status != 0)
		perror("cd");
	return status;
}

bool if_connector(char *str)
{
	if(strncmp(str, "||", 2) == 0)
		return true;
	else if( strncmp(str, "&&", 2) == 0)
		return true;
	else
		return false;
}

bool if_internal_cmd( char *str)
{
	if(strncmp(str, "pwd", 3) == 0)
		return true;
	else if( strncmp(str, "cd", 2) == 0)
		return true;
	else if( strncmp( str, "clear", 5) == 0)
		return true;
	else
		return false;
}
int process_commands_l2(commands arg, int si, int ei)
{
	bool if_pipe_found = false;
	for(int i=si; i<=ei; i++){
		if(strcmp(arg.cmds[i], "|") == 0){
			if_pipe_found = true;
		}
	}
	int status;
	bool if_internal_cmd_flag = if_internal_cmd(arg.cmds[si]);
	if(debug_print)
		fprintf(stderr, "cmd: '%s' type: %d\n", arg.cmds[si], if_internal_cmd_flag);
	if(if_internal_cmd_flag){
		if(strncmp(arg.cmds[si], "pwd", 3) == 0){
			char *path = pwd();
			if(path == NULL)
				status = -1;
			else{
				fprintf(stdout, "Working Dir: %s\n", path);
				status = 0;
			}
		}
		if( strncmp(arg.cmds[si], "cd", 2) == 0){
			status = cd(arg.cmds[si + 1]);
		}
		if(strcmp(arg.cmds[si], "clear") == 0){
			fprintf(stderr, "\033[H\033[J" );
		}
	}
	else{
		pid_t pid = fork();
		if(pid == 0){
			// child process
			do_childish_task(arg, si, ei);
		}
		else{
			waitpid(pid, &status, 0);
			if(debug_print)
				fprintf(stderr, "Status in parent : %d\n", status);
		}
	}
	return status;
}
int process_commands(commands arg, int si, int ei)
{
	int start_index = si, end_index;
	int no_of_cmds = end_index - start_index + 1;
	bool last_argument = false;
	bool if_to_be_executed = true;
	bool if_semicolon_found = false;
	int last_logical_connector = -1;
	int status;

	for(int i=si; i<=ei; i++){
		if( strncmp( arg.cmds[i], ";", 1) == 0)
			if_semicolon_found = true;
		if(i == arg.no_of_cmds)
			last_argument = true;
		if( if_semicolon_found && (strncmp(arg.cmds[i], ";", 1) == 0 || last_argument)){
			if(last_argument)
				end_index = i;
			else
				end_index = i-1;
			process_commands(arg, start_index, end_index);
			start_index = i+1;
		}
	}
	if(if_semicolon_found)
		return 0;
	if(debug_print){
		fprintf(stderr, "SI: %d EI: %d :In recursion: ", si, ei );
		for(int j = si; j<=ei; j++)
			fprintf(stderr, "Current arg: '%s' ", arg.cmds[j]);
		fprintf(stderr, "\n" );
	}

	start_index = si;
	last_argument = false;
	for( int i=si; i<=ei; i++){
		if( i == ei)
			last_argument = true;
		if(debug_print)
			fprintf(stderr, "in for, i: %d. last_argument: %d\n",i, last_argument );
		if(if_connector(arg.cmds[i]) || last_argument){

			if(!if_to_be_executed){
				if(debug_print)
					fprintf(stderr, "skipping '%s'\n", arg.cmds[start_index]);
				if( strncmp(arg.cmds[i], "||", 2) == 0 && last_logical_connector == 0)
					if_to_be_executed = false;
				else if( strncmp(arg.cmds[i], "&&", 2) == 0 && last_logical_connector == 1)
					if_to_be_executed = false;
				else
					if_to_be_executed = true;
				if( strncmp(arg.cmds[i], "||", 2) == 0 )
					last_logical_connector = 0;
				else if( strncmp(arg.cmds[i], "&&", 2) == 0 )
					last_logical_connector = 1;
				start_index = i+1;
				continue;
			}
			if(last_argument)
				end_index = i;
			else 
				end_index = i-1;

			status = process_commands_l2(arg, start_index, end_index);
			
			if(status == 0){
				if( strncmp(arg.cmds[i], "||", 2) == 0)
					if_to_be_executed = false;
				else if( strncmp(arg.cmds[i], "&&", 2) == 0)
					if_to_be_executed = true;
			}
			else{
				if( strncmp(arg.cmds[i], "||", 2) == 0)
					if_to_be_executed = true;
				else if( strncmp(arg.cmds[i], "&&", 2) == 0)
					if_to_be_executed = false;
			}
			if(debug_print)
				fprintf(stderr, "cmd: '%s' Status: %d, to be executed %d\n", arg.cmds[start_index], status, if_to_be_executed );
			start_index = i+1;
			if( strncmp(arg.cmds[i], "||", 2) == 0)
				last_logical_connector = 0;
			else if( strncmp(arg.cmds[i], "&&", 2) == 0)
				last_logical_connector = 1;
		}
	}
}
int main(int argc, char const *argv[])
{
	char buffer[100];
	commands arg;
	while(true){
		fprintf(stderr,"\n%s%s/%s%s ", HIGHLIGHT, pwd(), PROMPT, RESET );

		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);

		arg = prepare_commands(buffer);
		if(arg.no_of_cmds == -1)
			continue;
		if(strcmp(arg.cmds[0] , "exit") == 0){
			fprintf(stdout, "Exiting ... \n" );
			exit(0);
		}

		process_commands(arg, 0, arg.no_of_cmds);
	}
	
	return 0;
}