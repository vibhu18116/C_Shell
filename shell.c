#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

char all_piped[30][1024];
int pipeCount = 0;

void executeCommand(int index, int further_flag);

void readline(char* buffer){
	int count = 0;
	char c;
	c = getchar();
	while (c != '\n'){
		buffer[count] = c;
		count++;
		c = getchar();
	}
	buffer[count] = '\0';
}

void pipe_splits(char* input){

	int inp_count = 0;
	int internal = 0;

	while (input[inp_count]!='\0'){
		char readChar = input[inp_count];
		if (readChar == '|'){
			all_piped[pipeCount][internal] = '\0';
			internal = 0;
			pipeCount+=1;
			inp_count++;
			continue;
		}
		all_piped[pipeCount][internal] = readChar;
		internal++;
		inp_count++;
	}
	all_piped[pipeCount][internal] = '\0';
	pipeCount++;
}


void copyValidStuff(int* internal, int* c, int* valid, int* w, char** writing, char* buffer, char* command_name){
	buffer[*w] = '\0';
	if (*internal == 0 && *valid){
		writing[*internal] = malloc(1024*sizeof(char));
		strcpy(command_name, buffer);
		strcpy(writing[*internal], command_name);
	}else{
		if (*valid){
			writing[*internal] = malloc(1024*sizeof(char));
			strcpy(writing[*internal], buffer);
			buffer = "";
			*w  = 0;
		}
	}
	*valid = 0;
	(*internal)++;
}

int setSymbols(char* command, char* command_name, char** command_args){

	int c = 0;
	int internal = 0;
	int w = 0;
	char buffer[1024];
	int valid = 0;
	while (command[c]!='\0'){
		char read = command[c];
		if (read == '<'){
			copyValidStuff(&internal, &c, &valid, &w, command_args, buffer, command_name);
			char filename[80];
			int d = 0;
			c++;
			if (command[c] == ' '){
				c++;
			}
			while (command[c]!=' ' && command[c]!='\0'){
				filename[d] = command[c];
				c++;
				d++;
			}
			filename[d] = '\0';
			close(0);
			int c = open(filename, O_RDONLY);
			if (c == -1){
				fprintf(stderr, "shell: %s: No such file or directory\n", filename, 80);
				return -1;
			}

		}else if (read == '>'){
			copyValidStuff(&internal, &c, &valid, &w, command_args, buffer, command_name);
			if (command[c-1] == '1'){
				internal--;
			}
			if (command[c+1] == '>'){
				char filename[80];
				int d = 0;
				c+=2;
				if (command[c] == ' '){
					c++;
				}
				while (command[c]!=' ' && command[c]!='\0'){
					filename[d] = command[c];
					c++;
					d++;
				}
				filename[d] = '\0';
				close(1);
				open(filename, O_APPEND|O_CREAT|O_WRONLY);

			}else{
				if (command[c-1] == '2' && command[c+1] == '&' && command[c+2] == '1'){
					internal--;
					close(2);
					dup(1);
					c+=3;
				}else if (command[c-1] == '2'){
					internal--;
					// printf("here\n");
					char filename[80];
					int d = 0;
					c++;
					if (command[c] == ' '){
						c++;
					}
					while (command[c]!=' ' && command[c]!='\0'){
						filename[d] = command[c];
						c++;
						d++;
					}
					filename[d] = '\0';
					close(2);
					creat(filename, 0666);
				}else {
					char filename[80];
					int d = 0;
					c++;
					if (command[c] == ' '){
						c++;
					}
					while (command[c]!=' ' && command[c]!='\0'){
						filename[d] = command[c];
						c++;
						d++;
					}
					filename[d] = '\0';
					close(1);
					creat(filename, 0666);
				}
			}
		}else if (read == ' '){
			buffer[w] = '\0';
			if (internal == 0 && valid){
				command_args[internal] = malloc(1024*sizeof(char));
				strcpy(command_name, buffer);
				strcpy(command_args[internal],command_name);
				strcpy(buffer, "");
				internal++;
			}else{
				if (valid){
					command_args[internal] = malloc(1024*sizeof(char));
					strcpy(command_args[internal], buffer);
					strcpy(buffer, "");
					w  = 0;
					internal++;
				}
			}

			valid = 0;
			c++;
			w = 0;
		}else{
			valid = 1;
			buffer[w] = read; 
			w++;
			c++;
		}
	}

	buffer[w] = '\0';
	if (internal == 0)
		strcpy(command_name, buffer);

	if (valid){
		command_args[internal] = malloc(1024*sizeof(char));
		strcpy(command_args[internal], buffer);
		internal++;
	}
	command_args[internal] = NULL;
	return 0;
}


int main(){

	while(1){
		write(1, "$ ", 2);
		char input_command[1024] = "";
		readline(input_command);

		if (!strcmp(input_command, "exit")){
			return 0;
		}

		if (input_command[0] == '\0'){
			continue;
		}

		pipeCount = 0;

		pipe_splits(input_command);

		int pid = fork();

		if (pid == 0){
			executeCommand(0, 1);
			return 0;
		}else if (pid > 0){
			wait(NULL);

		}else{
			printf("Fork error\n");
			exit(-1);
		}
		
	}
	return 0;

}


void executeCommand(int index, int further_flag){

	if (further_flag == 0 || index == pipeCount-1 || pipeCount==1){
		char command_name[80];
		char* command_args[1024];
		int status = setSymbols(all_piped[index], command_name, command_args);
		if (status ==  -1){
			return;
		}
		int c = execvp(command_name, (char**)command_args);

		if (c == -1){
			write(2, "Exec failed..., Invalid command\n", 33);
			return;
		}
	}else{

		int pid;
		int fd[2];
		pipe(fd);
		pid = fork();
		if (pid == 0){
			close(1);
			dup(fd[1]);
			close(fd[1]);
			close(fd[0]);
			executeCommand(index, 0);
		}
		int pid2 = fork();
		if (pid2 == 0){
			close(0);
			dup(fd[0]);
			close(fd[1]);
			close(fd[0]);
			executeCommand(index+1, 1);		
		}

		close(fd[0]);
		close(fd[1]);
		wait(NULL);
		wait(NULL);
	}
}