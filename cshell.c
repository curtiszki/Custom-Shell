// cshells.c
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
// print out a prompt
int readLogFromFile();
int prompt() {
	printf("Input the prompt");
	return 0;
}
/*
int strtoktest() {
	char str[80] = "this - not - really about- anything - in - here";
	const char s2[2] = "-";
	char *token;

	token = strtok(str, s2);

	while (token != NULL) {
		printf(" %s\n", token);
		token = strtok(NULL, s2);
	}
	return 0;
}
*/
void read_input(char** command, char** parameters, int *param_count, FILE *f) {
	char lineBuffer[250];

	if (f == NULL) {
		if(fgets(lineBuffer, 250, stdin) == NULL) {
			fprintf(stderr, "Error on input command\n");
			command[0]='\0';
			return;
		}
	}
	if (f != NULL) {
		if(fgets(lineBuffer, 250, f) == NULL) {
			fprintf(stderr, "Error: Script should end on log command\n");
			exit(0);
		}
	}
	
	if (*lineBuffer == '\n') {
		return;
	}
	else {
		// remove the new line character
		lineBuffer[strcspn(lineBuffer, "\n")] = 0;
		int index = 0;
		char* token = strtok(lineBuffer, " ");
		char* tempArray[32];
		// tokenize the input
		while(token != NULL) {
			tempArray[index++] =  strdup(token);
			token = strtok(NULL, " ");
		}
		// last token is not null terminated
		char* tmp = strdup(tempArray[index-1]);
		free(tempArray[index-1]);
		tempArray[index-1] = malloc(sizeof(char) * (strlen(tmp)+1));
		strcpy(tempArray[index-1], tmp);
		tempArray[index-1][strlen(tmp)] = '\0';
		free(tmp);

		// put the values in the arrays
		for(int i = 0; i < 1 && i < index; i++) {
			command[i] = strdup(tempArray[i]);
		}
		for(int i = 1; i < index; i++) {
			parameters[(*param_count)++] = strdup(tempArray[i]);
		}
		// deallocate temp array
		for (int i = 0; i < index; i++) {
			free(tempArray[i]);
		}
	}
}
struct EnvVar {
	char *name;
	char *value;
};

int execute_command(char** command, char** parameters, int param_count, int *color_value, struct EnvVar evar[], int env_var_count) {
	// ensure that the first command is cshells
	// get the second command
	if (command[0] == NULL) {
		fprintf(stderr, "No command given \n");
		return -1;
	}

	// built in commands
	if (!strcmp(command[0], "print")) {
		if (param_count == 0) {
			fprintf(stderr, "No arguments supplied to print function\n");
			return -1;
		}
		for (int i = 0; i < param_count; i++) {
			printf("%s ", parameters[i]);
		}
		printf("\n");
		return 0;
	}
	if (!strcmp(command[0], "exit")) {
		// exit command
		if (param_count > 0) {
			fprintf(stderr, "Error: too many arguments detected. Exiting anyway..\n");
		}
		for (int i = 0; i < param_count; i++) {
			free(parameters[i]);
		}

		for (int i = 0; i < env_var_count; i++) {
		free(evar[i].name);
		free(evar[i].value);	}
		free(command[0]);
		command[0] = NULL;
		printf("Bye!\n");
		exit(0);
	}
	if (!strcmp(command[0], "log")) {
		// log command
		return readLogFromFile();
	}
	if (!strcmp(command[0], "theme")) {
		// theme command
		// should apply to child processes as well
		if (param_count >= 1) {
			if (param_count > 1) {
				fprintf(stderr, "Too many arguments, using the first argument\n");
			}
			if(!strcmp(parameters[0], "red")) {
				(*color_value) = 1;
				return 0;
			}
			if(!strcmp(parameters[0], "green")) {
				(*color_value) = 2;
				return 0;
			}
			if(!strcmp(parameters[0], "blue")) {
				(*color_value) = 3;
				return 0;
			}
			fprintf(stderr, "unsupported theme\n");
			return -1;
		}
		else {
			fprintf(stderr, "unsupported theme\n");
			return -1;
		}
	}
	// non-builtin commands
	char path[] = "/bin/";
	char* fullpath = malloc(sizeof(char)*strlen(command[0]) + sizeof(char)*(1+strlen(path)));
	strcpy(fullpath, path);
	strcat(fullpath, command[0]);
	char* fullarglist[param_count + 2];
	fullarglist[0] = strdup(command[0]);
	bool backgroundProcess = false;
	// if it is a background process by & at the end
	if(fullarglist[0][strlen(fullarglist[0])-1] == '&') {
		// cut off the ampersand
		fullpath[strlen(fullpath) - 1] = '\0';
		fullarglist[0][strlen(fullarglist[0])-1] = '\0';
		backgroundProcess = true;
	}
	//strcat(fullarglist[0], command[0]);
	for(int i = 1; i <= param_count; i++) {
		fullarglist[i] = strdup(parameters[i-1]);
	}
	fullarglist[param_count+1] = NULL;

	int pid;
	// create a child process
	// create pipe for function result
	int fd[2];
	int fail = 0;
	pipe(fd);
	pid = fork();
	if(pid == 0) {
		//execute sh
		close(fd[0]);
		if(execv(fullpath, fullarglist) == -1) {
			// if there's an error
			fprintf(stderr, "Missing keyword or command, or permission problem\n");
			fail = 1;
			write(fd[1], &fail, sizeof(fail));
			close(fd[1]);
		}
		exit(0);
	}
	close(fd[1]);
	read(fd[0], &fail, sizeof(fail));
	// deallocate memory
	free(fullpath);
	for (int i = 0; i <= param_count; i++) {
		free(fullarglist[i]);
	}
	// wait for child processes to complete
	if (!backgroundProcess) {
		wait(NULL);
	}

	if(!fail) {
		return 0;
	}
	return -1;
}
// struct for env var from instruction
void display_prompt(int *color_value, int argc) {
	// int value 0-4 for ansi escape
	if (*color_value == 1) {
		// red
		printf("\x1b[31m");
	}
	else if (*color_value == 2) {
		// green
		printf("\x1b[32m");
	}
	else if (*color_value == 3) {
		// blue
		printf("\x1b[34m");
	}
	else if (*color_value == 4) {
		// reset value
		printf("\x1b[0m");
		// change color back to default
		(*color_value) = 0;

	}
	if (argc == 1) {
		printf("cshell$ ");
	}
}


char* getTime() {
	struct tm* timeptr;
	time_t t;
	t = time(NULL);
	timeptr = localtime(&t);
	char* timestr = asctime(timeptr);
	return timestr;
}

void resetLogFile() {
	FILE *f = fopen("log.txt", "w");
	if (f == NULL)
	{
		printf("Unable to read file log.txt\n");
		exit(1);
	}
	fclose(f);
}

void logToFile(char** command, char* timestamp, int returnvalue) {
	FILE *f = fopen("log.txt", "a");
	if (f == NULL)
	{
		printf("Error open file log.txt\n");
		exit(1);
	}
	fprintf(f, "%s", timestamp);
	fprintf(f, " %s", command[0]);
	fprintf(f, " %d\n", returnvalue);
	fclose(f);
}

bool isAssignment(char** command) {
	if (command[0][0] == '$') {
		return true;
	}
	return false;
}


int readLogFromFile() {
	char lineBuffer[251];
	FILE *f = fopen("log.txt", "r");
	if ("f" == NULL)
	{
		printf("Error opening file log.txt\n");
		exit(1);
	}
	while(fgets(lineBuffer, 251, f)) {
		printf("%s", lineBuffer);
	}
	fclose(f);
	return 0;
}
/*
char* concatenateCommand(char** command, char** parameters, int *param_count) {
	for(int i = 0; i < param_count; i++) {
		strcat(command[0], " ");
		strcat(command[0], parameters[i]);
	}
};
*/

int EnvVarIndex(char* name, char* value, int EnvTotal, struct EnvVar env_array[]) {
	int index = -1;
	for(int i = 0; i < EnvTotal; i++) {
		if (!strcmp(name, env_array[i].name)) {
			index = i;
		}
	}
	return index;	
}

bool checkParametersForVariable(int param_count, char** parameters, struct EnvVar env_array[], int env_var_count) {
	char* param;
	bool found;
	bool atLeastOneMissing = false;
	for(int i = 0; i < param_count; i++) {
		if(parameters[i][0] == '$') {
			found = false;
			param = (char*)malloc(sizeof(char) * (1+ strlen(parameters[i])));
			strncpy(param, &(parameters[i])[1], strlen(parameters[i])-1);
			param[strlen(parameters[i]) - 1] = '\0';
			// replace var in param with env variable value
			for(int j = 0; j < env_var_count; j++) {
				if(!(strcmp(env_array[j].name, param))) {
					free(parameters[i]);
					parameters[i] = strdup(env_array[j].value);
					found = true;
					break;
				}
			}
			if(!found) {
				atLeastOneMissing = true;
				printf("Error: No Environment Variable %s found\n", parameters[i]);
			} 
			free(param);
		}
		
	}
	return atLeastOneMissing;
}

int main(int argc, char* argv[]) {
	char *parameters[30] = {'\0'};
	char *command[1] = {'\0'};
	int param_count = 0;
	int color_value = 0;
	int returnvalue;
	// static struct array
	struct EnvVar evar[100];
	int env_var_count = 0;
	FILE *f = NULL;
	// open file for log output
	// do forever
	resetLogFile();
	if (argc>1) {
		f = fopen(argv[1], "r");
		if (f == NULL)
		{
		printf("Unable to read script file: %s\n", argv[1]);
		exit(1);
		}
	}
	while(1) {
		// default to interactive mode
		display_prompt(&color_value, argc);
		if(argc<= 1) {
			read_input(command, parameters, &param_count, NULL);
		}
		else {
			read_input(command, parameters, &param_count, f);
		}
		if(command[0] != NULL) {
			if (isAssignment(command)) {
				char* var1;
				char* var2;
				char *token;
				char* tmpcmd = strdup(command[0]);
				token = strtok(tmpcmd, "=");
				if (token == NULL) {
					fprintf(stderr, "Variable value expected\n");
					returnvalue = -1;
				}
				else {
					// analyze the first token
					if (strlen(token) <= 1) {
						fprintf(stderr, "token should be in format $[a-zA-Z0-9_]\n");
						returnvalue = -1;
					}
					else {
						// since $ not included
						var1 = (char*)malloc((strlen(token)) * sizeof(char));
						strncpy(var1, &token[1], strlen(token)-1);
						var1[strlen(token)-1] = '\0';
						returnvalue = 0;
					}
				}
				// analyze the second token
				token = strtok(NULL, "=");
				// copy/paste abomination from above
				if(returnvalue == 0) {
					if (token == NULL) {
					fprintf(stderr, "No assignment token found for variable assignment\n");
					returnvalue = -1;
					}
				else {
					// analyze the second token
					if (strlen(token) < 1) {
						fprintf(stderr, "token should be in format [a-zA-Z0-9_]\n");
						returnvalue = -1;
					}
					else {
						var2 = (char*)malloc((1+strlen(token)) * sizeof(char));
						strncpy(var2, &token[0], strlen(token));
						var2[strlen(token)] = '\0';
						returnvalue = 0;
							
					}
				}
				}
				token = strtok(NULL, "=");
				if(token != NULL) {
					fprintf(stderr, "Too many arguments supplied for variable assignment \n");
				}
				

				if(returnvalue == 0) {

					int indexFound; 
					indexFound = EnvVarIndex(var1, var2, env_var_count, evar);
					if (indexFound == -1) {
						struct EnvVar newStruct;
						//newStruct.name = (char*)malloc(strlen(var1) * sizeof(char));
						newStruct.name = strdup(var1);
						//newStruct.value = (char*)malloc(strlen(var2) * sizeof(char));
						newStruct.value = strdup(var2);
						free(var1);
						free(var2);
						evar[env_var_count++] =  newStruct;
					}
					else {
						// update the value
						evar[indexFound].value = strdup(var2);
					}
				}
				//concatenateCommand(command, parameters, param_count);
				logToFile(command, getTime(), returnvalue);
			}
			else {
				// returns true if a variable is not there
				if(!checkParametersForVariable(param_count, parameters, evar, env_var_count)) {
					returnvalue = execute_command(command, parameters, param_count, &color_value, evar, env_var_count);
					//concatenateCommand(command, parameters, param_count);
					logToFile(command, getTime(), returnvalue);
				}
			}
			// set the arrays to null character
			//command = NULL;
			for (int i = 0; i < param_count; i++) {
				free(parameters[i]);
			}
			for (int i = 0; i < 30; i++) {
				parameters[i] = '\0';
			}
			param_count = 0;

			// if script mode and command is log exit
			if(argc > 1 && (!strcmp(command[0],"log"))) {
				printf("Bye!\n");
				fclose(f);
				// clear environment vars
				for (int i = 0; i < env_var_count; i++) {
				free(evar[i].name);
				free(evar[i].value);	}

				free(command[0]);
				command[0] = NULL;
				exit(0);
			}
			free(command[0]);
			command[0] = NULL;
		}
	}
	// clear environment vars
	for (int i = 0; i < env_var_count; i++) {
		free(evar[i].name);
		free(evar[i].value);
	}
	exit(0);
}