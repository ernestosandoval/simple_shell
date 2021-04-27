//#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// should be 514
#define MAX_INPUT_SIZE 512

#define STD_INPUT 0
#define STD_OUTPUT 1

void type_prompt() {
        printf("shell: ");
}

void print_error(char * error_msg) {
        printf("ERROR: %s\n", error_msg);
}

bool read_command (char buf[]) {
	// check if EOF
	if ( fgets(buf, MAX_INPUT_SIZE, stdin) == NULL ) {
		printf("\ngoodbye!\n");
		exit(EXIT_SUCCESS);
	}

	//check if whitespace
	if(buf[strspn(buf, " \f\r\t\v")] == '\n') {
		return false;
	}

	// check if input too long	
	if (strchr(buf, '\n') == NULL) {
		// clear stdin
		while ((getchar()) != '\n');

		print_error("input line too long");
		return false;
	}
	return true;
}

// ignore for now...
bool parse_command(char buf[], char cmd[], char *par[]) {

	char *delim = " \f\r\t\v";

        cmd[0] = '\0';
	// do this or set par[i][0]= '\0'?
        for(int i = 0; i < 256; i++) {
                par[i] = NULL;
        }
	
	char *p = strchr(buf, '\n');
	*p = '\0';
	
	p = strtok(buf, delim);

	int redir_input_count = 0;
	int redir_output_count = 0;
	int ampersand_count = 0;

	//parse the line into words and place in par
	int i = 0;
	while( p != NULL ) {
		// when we parse, we want to count the number of '<', '>', '&' 
		// our input is only allowed to have at most one of each
		if(strcmp(p, "<") == 0) {
			redir_input_count = redir_input_count + 1;
		} else if(strcmp(p, ">") == 0) {
			redir_output_count = redir_output_count + 1;
		} else if(strcmp(p, "&") == 0) {
			ampersand_count = ampersand_count + 1;
		}

		if(redir_input_count > 1) {
			print_error("at most one '<' token allowed");
			return false;
		} else if(redir_output_count > 1) {
			print_error("at most one '>' token allowed");
			return false;
		} else if(ampersand_count > 1) {
			print_error("at most one '&' token allowed");
			return false;
		}

		par[i++] = strdup(p);
		p = strtok(NULL, delim);
	}

	// we are done parsing; i is the length of par
	// we need to check if & is the last character
	if (ampersand_count == 1) {
		if (strcmp(par[i-1], "&") != 0) {
			print_error("'&' must be last token of input line");
			return false;
		}

	}

	if (redir_output_count > 0) {
		if (strcmp(par[i-2-ampersand_count], ">") != 0) {
			print_error("expected '>' token and a file name at the end");
			return false;
		}
	}

	// first parameter is command
	strncpy(cmd, par[0], 32);

	par[i] = NULL;
	return true;

}

void execute_command(char command[], char *parameters[]) {
	if(command[0] == '\0') {
		//idk
		exit(1);
	}
	if ( command[0] != '\n') {

		char *par;
		par = parameters[0];

		int coms[100];
		int lens[100];
		int dels[100];
		for (int i = 0; i < 100; i++) {
			coms[i] = 0;
			lens[i] = 0;
			dels[i] = 0;
		}

		int first = 1;
		int n = 0;
		int m = 0;
		while(par != NULL) {
			if(first) {
				coms[m] = n;
				first = 0;
			}
			if ((strcmp(par, "|") == 0) | (strcmp(par, "<") == 0) | (strcmp(par, ">") == 0) | (strcmp(par, "&") == 0 ) ) {
				dels[m] = n;
				first = 1;
				m = m + 1;
			} else {
				lens[m] = lens[m] + 1;
			}

			n = n + 1;
			par = parameters[n];
		}

		int pipes[256][2];
		// needs to do some stuff first
		char current_com[32], *current_par[256];
		memcpy(current_com, command, 32);
		memcpy(current_par, &parameters[0], lens[0]*sizeof(*parameters));

		char *delimiter;
		delimiter = parameters[dels[0]];

		int x = 1;
		// needs some condition
		//while(current_com != NULL) {

			// needs to be in a loop
			int status;
			pid_t child_id;

			child_id = fork();
			if (child_id != 0) {
				wait(&status);
			} else {
				int i = 0;
				char *param;
				param = parameters[0];

				while(param != NULL) {
					i = i + 1;
					param = parameters[i];
				}

				// then i is our length

				/*if (i > 2) {
					if (strcmp(parameters[i-ampersand_count-2], ">") == 0) {
						// we found a >
						// file should be param[i-1]
						if (parameters[i-1] != NULL) {
							int fd = open(parameters[i-1], O_CREAT | O_RDWR | O_TRUNC, 0644);
							dup2(fd, STD_OUTPUT);
							close(fd);
							parameters[i-2] = NULL;
							parameters[i-1] = NULL;
							//i = i + 2;
							//param = parameters[i];
							//while(param != NULL) {
							//      parameters[i-2] = param;
							//      i = i + 1;
							//      param = parameters[i];
							//}
						} else {
							print_error("expected the name of a file");
							exit(0);
						}
					}
                                }*/

				if (execvp(command, parameters) & ( command[0] != '\n')) {
					print_error("command not found...");
				}
				exit(EXIT_SUCCESS);
			}
		//}
	}
}

int main(int argc, char *argv[]) {

	char buf[MAX_INPUT_SIZE];
	char command[32], *parameters[256];

	bool prompt = true;

	if(argc > 1) {
		if( strcmp(argv[1],"-n") == 0 ) {
			prompt = false;
		}
	}

	while( 1 ) {
		if(prompt) {
			type_prompt();
		}

		if (read_command(buf)) {
			if (parse_command(buf, command, parameters)) {
				execute_command(command, parameters);
			}
		}
	}

	return 0;

}

