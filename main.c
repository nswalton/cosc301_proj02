// Neil Walton
// COSC301
// Project 02
// 10/16/14
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>






void free_commands(char ***all_commands) {			//Used to free the array of commands
    for (int x = 0; all_commands[x] != NULL; x++) {
	for (int y = 0; all_commands[x][y] != NULL; y++) {
	    free(all_commands[x][y]);
	}
	free(all_commands[x]);
    }
    free(all_commands);
}



char** tokenify(const char *s, int flag) {

    char *copy = strdup(s);
    char **alltokens = malloc(sizeof(char*)*strlen(s));
    char *token;

    int index = 0;

    char *delim;
    if (flag == 0) {			//On first pass through, we want to use ';' as the delimiter to separate commands
	delim = ";\n";
    }
    if (flag == 1) {			//Second pass through, we want to separate command and options with " \t\n"
	delim = " \t\n";
    }
    token = strtok(copy, delim);
    while (token != NULL) {
	alltokens[index] = strdup(token);
	token = strtok(NULL, delim);
	index++;
    }

    free(copy);
    alltokens[index] = NULL;
    return alltokens;
}



void process_commands(char*** com_list, int *mode_flag) {

    int exit_flag = 0;				//If set to 1, will exit at end of processing commands
    int num_waits = 0;				//When running in parallel mode, keep track of the number of wait calls to be made
    int temp_mode_flag = *mode_flag;		//Temp flag to keep track of any mode changes w/o affecting current mode

    int k = 0;
    while (com_list[k] != NULL) {
	char **argv = com_list[k];		    //Get next command from user input
	if (strcmp(argv[0],"exit") == 0) {	    //If the command is exit, set flag so exit will take place after all other commands
	    exit_flag = 1;
	    k++;
	    continue;
    	}

    	if ((strcmp(argv[0],"mode") == 0) && (argv[1] != NULL)) {
	    if ((strcmp(argv[1],"sequential") == 0) || (strcmp(argv[1],"s") == 0)) {  //User telling shell to run in sequential mode
	    	temp_mode_flag = 0;						      //When flag = 0, running in sequential mode
		k++;								      //Increment index and move on to next command
	    	continue;
	    }

	    if ((strcmp(argv[1],"parallel") == 0) || (strcmp(argv[1],"p") == 0)) {    //User telling shell to run in parallel mode
	        temp_mode_flag = 1;						      //When flag = 1, running in parallel mode
	        k++;								      //Increment index and move on to next command
	    	continue;
	    }
    	}

    	if ((strcmp(argv[0],"mode") == 0) && (argv[1] == NULL)) {	//If just mode is input, tell user what mode shell is in
	    if (*mode_flag == 0) {
		printf("Running in sequential mode\n");
		k++;
		continue;
	    }

	    if (*mode_flag == 1) {
		printf("Running in parallel mode\n");
		k++; 
		continue;
	    }
    	}

	pid_t pid = fork();	

	if (*mode_flag == 0) {		//sequential mode
            if (pid == 0) {			//Child, execute commands here

	        if (execv(argv[0], argv) < 0) {						//An error has occured
                    fprintf(stderr, "execv failed: %s\n", strerror(errno));
		    free_commands(com_list);						//Free up heap memory and exit
		    free(mode_flag);
		    exit(1);
                }
            } 
            else {
                wait(NULL);			//Child is running, wait for child to finish before resuming
	    }
	}

	if (*mode_flag == 1) {				//Parallel mode
	    if (pid == 0) {				//Child, execute commands here
		num_waits++;
	        if (execv(argv[0], argv) < 0) {						//An error has occured
                    fprintf(stderr, "execv failed: %s\n", strerror(errno));
		    free_commands(com_list);						//Free up heap memory and exit
		    free(mode_flag);
		    exit(1);
                }
            }
        
	    for (int q = 0; q < num_waits; q++) {		//For each child process that is currently running, wait
	        wait(NULL);
	    }
	}
	k++;
	}
        
    *mode_flag = temp_mode_flag;			//Now that all commands have been processed, change the running mode
    free_commands(com_list);				//Free up heap allocated memory before moving on to next line of commands
    if (exit_flag == 1) {				//One of the commands was exit, free up flag memory and exit
	free(mode_flag);
	exit(0);
    }
}



//Replace the first occurance of '#' with and end line character, so the rest of the line is ignored for all further processing.
void remove_comments(char *command_string) {
    int j = 0;
    while (command_string[j] != '\0') {
	if (command_string[j] == '#') {
	    command_string[j] = '\0';
	    break;
	}
	j++;
    }
}



int main(int argc, char **argv) {

    


    int *mode_flag = malloc(sizeof(int));			//Flag to track which mode (sequential/parallel) shell is running in
    *mode_flag = 0;

    char *prompt = "without_delay> ";
    printf("%s", prompt);
    fflush(stdout);
    char command[1024];

    while (fgets(command, 1024, stdin) != NULL) {
    
	remove_comments(command);				//Strip comments off the end of line of commands

        char **commandlist = tokenify(command, 0);		//Split line into separate commands using ';' as delimiter
	char ***all_commands = malloc(sizeof(char**)*64);	//Array of all commands in the command line
	memset(all_commands, 0, sizeof(char**)*64);		//Zero out the all_commands array
        int i = 0;
        while (commandlist[i] != NULL) {
	    char** command_options = tokenify(commandlist[i], 1);	//Separate each command into the command and its options

	    all_commands[i] = command_options;				//Add the above to all_commands array

	    i++;

        }
	for (int v = 0; commandlist[v] != NULL; v++) {			//Free up the memory allocated to commandlist
	    free(commandlist[v]);
	}
	free(commandlist);
	process_commands(all_commands, mode_flag);			//Function call to process the commands retrieved above
        printf("%s", prompt);
    }
    free(mode_flag);							//If there is no more user input, free memory and return
    return 0;
}























