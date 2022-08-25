//#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 512
#define MAX_TOKEN_SIZE 32
#define MAX_NUM_TOKENS MAX_INPUT_SIZE/2

#define STD_INPUT 0
#define STD_OUTPUT 1

void type_prompt() {
	printf("shell: ");
}

void print_error(char * error_msg) {
	printf("ERROR: %s\n", error_msg);
}

void print_command(char ** parameters[]) {
	printf("commands: \n");
	int i = 0;
	char **par = parameters[i++];
	while (par != NULL) {
		printf("command: ");
		int j = 0;
		char *p = par[j++];
		while (p != NULL) {
			printf("%s ", p);
			p=par[j++];
		}
		printf("\n");
		par = parameters[i++];
	}
	printf("\n");
}

void print_tokens(char * parameters[]) {
	printf("tokens: ");
	int i = 0;
	char *par = parameters[i++];
	while (par != NULL) {
		//int j = 0;
		//char *p = par[j++];
		//while (p != NULL) {
		printf("%s ", par);
		//	p=par[j++];
		//}
		par = parameters[i++];
	}
	printf("\n");
}

bool read_command (char buffer[]) {
	// check if EOF
	if ( fgets(buffer, MAX_INPUT_SIZE, stdin) == NULL ) {
		printf("\ngoodbye!\n");
		exit(EXIT_SUCCESS);
	}

	//check if whitespace
	if(buffer[strspn(buffer, " \f\r\t\v")] == '\n') {
		return false;
	}

	// check if input too long
	if (strchr(buffer, '\n') == NULL) {
		// clear stdin
		while ((getchar()) != '\n');

		print_error("input line too long");
		return false;
	}
	return true;
}

// at most one '<' token can be given for a single command
// only the first command on the input line can have its input redirected
// the '<' token must be followed by a token that represents a file name

// at most one '>' token can be given for a single command
// only the kast command on the input line can have its output redirected
// the '>' token must be followed by a token that represents a file name

// the '&' token may only appear as the last token of a line
bool parse_command(char buffer[], char **parameters[], char **input, char **output, bool * has_ampersand) {
	char *saveptr;
	char *delim = " \n\f\r\t\v";
	char *delim2 = "<>|&";
	int lens[MAX_NUM_TOKENS];
	char *tokens[MAX_NUM_TOKENS];
	char current_token[MAX_TOKEN_SIZE];
	char** par;
	bool has_redir_input = false;
	bool has_redir_output = false;
	int pipe_count = 0;
	int token_count=0;
	*input=NULL;
	*output=NULL;
	*has_ampersand = false;

	for (int i = 0; i < MAX_NUM_TOKENS; i++) {
		parameters[i] = NULL;
		lens[i] = 0;
		tokens[i]=NULL;
	}

	char *token = strtok_r(buffer, delim, &saveptr);
	while (token != NULL) {
		int token_index = 0;
		for (int i = 0; i < strlen(token); i++) {
			if(strpbrk(&token[i], delim2) == &token[i]) {
				if(token_index!=0){
					current_token[token_index]='\0';
					tokens[token_count++]=strdup(current_token);
					token_index=0;
				}
				current_token[0]=token[i];
				current_token[1]='\0';
				tokens[token_count++]=strdup(current_token);
			} else {
				current_token[token_index++]=token[i];
			}
		}
		if(token_index!=0){
			current_token[token_index]='\0';
			tokens[token_count++]=strdup(current_token);
			token_index=0;
		}

		token = strtok_r(NULL, delim, &saveptr);
	}

	// '| ls'
	if (strcmp(tokens[0], "|") == 0) {
		print_error("syntax error near unexpected token `|'");
		return false;
	}

	par=malloc(sizeof(char**));
	*par=malloc(sizeof(char*)*MAX_NUM_TOKENS);
	for (int i = 0; i < token_count; i++) {
		token=tokens[i];
		if (strcmp(token, "|") == 0) {
			if (has_redir_output) {
				print_error("only the last command may have its output redirected");
				return false;
			}
			if (lens[pipe_count] == 0) {
				print_error("syntax error near unexpected token `|'");
				return false;
			}
			par[lens[pipe_count]] = NULL;
			parameters[pipe_count] = par;
			par=malloc(sizeof(char**));
			*par=malloc(sizeof(char*)*MAX_NUM_TOKENS);
			pipe_count++;
		} else if (strcmp(token, "<") == 0) {
			if (has_redir_input) {
				print_error("at most one '<' token allowed");
				return false;
			} else if (pipe_count > 0) {
				print_error("only the first command may have its input redirected");
				return false;
			} else if (lens[pipe_count] == 0) {
				print_error("a command must appear before input redirection");
				return false;
			} else {
				if ((i+1)<token_count){
					*input=tokens[i+1];
					i++;
					has_redir_input = true;
				} else {
					print_error("expected the name of a file");
					return false;
				}
			}
		} else if (strcmp(token, ">") == 0) {
			if (has_redir_output) {
				print_error("at most one '>' token allowed");
				return false;
			} else if (lens[pipe_count] == 0) {
				print_error("a command must appear before output redirection");
				return false;
			} else {
				if ((i+1)<token_count){
					*output=tokens[i+1];
					i++;
					has_redir_output = true;
				} else {
					print_error("expected the name of a file");
					return false;
				}
			}
		} else if (strcmp(token, "&") == 0) {
			if (*has_ampersand) {
				print_error("at most one '&' token allowed");
				return false;
			} else {
				*has_ampersand = true;
			}
		} else {
			par[lens[pipe_count]++] = strdup(token);
		}
	}
	par[lens[pipe_count]] = NULL;
	parameters[pipe_count] = par;

	// check if '&' is the last character
	if (*has_ampersand) {
		if (strcmp(tokens[token_count-1], "&") != 0) {
			print_error("'&' must be last token of input line");
			return false;
		}
	}

	return true;
}

void execute_command(char **parameters[], char **input, char **output, bool * has_ampersand) {
	char **par;
	int fd[2], status;
	int in = STD_INPUT;
	int num_commands = 0;
	pid_t child_id;

	par = parameters[0];
	while(par != NULL) {
		par = parameters[++num_commands];
	}

	for (int i = 0; i < num_commands; i++) {
		par = parameters[i];

		if (pipe(fd) == -1) {
			print_error("failed to create a pipe");
			exit(EXIT_FAILURE);
		}

		switch (child_id = fork()) {
		case -1:
			print_error("failed to create a fork");
			exit(EXIT_FAILURE);
		//child
		case 0:
			if (i==0) {
				if(*input) {
					in = open(*input, O_RDONLY);
				}
			}
			if (in!=STD_INPUT) {
				dup2(in, STD_INPUT);
				close(in);
			}
			
			int out=fd[1];
			if (i == num_commands-1) {
				if (*output) {
					//why 0644
					out = open(*output, O_CREAT | O_RDWR | O_TRUNC, 0644);
					dup2(out, STD_OUTPUT);
					close(out);
				}
			} else {
				dup2(out, STD_OUTPUT);
				close(out);
			}

			// execvp returns only on failure
			if (execvp(par[0], par)) {
				printf("%s: command not found\n", par[0]);
			}
			exit(EXIT_FAILURE);
		//parent
		default:
			close(fd[1]);
			in = fd[0];
			if(!*has_ampersand) {
				// we may have to waitpid if pipes
				wait(&status);
			}
			break;
		}
	}
	if(!*has_ampersand) {
                                // we may have to waitpid if pipes
                                wait(0);
                        }

//wait(0);
	close(in);
}

int main(int argc, char *argv[]) {
	char buffer[MAX_INPUT_SIZE], **parameters[MAX_NUM_TOKENS];
	char** input, **output;
	bool* has_ampersand;
	input=(char**)malloc(sizeof(char**));
	output=(char**)malloc(sizeof(char**));
	has_ampersand=(bool*)malloc(sizeof(bool*));

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

		if (read_command(buffer)) {
			if (parse_command(buffer, parameters, input, output, has_ampersand)) {
				//print_command(parameters);
				execute_command(parameters, input, output, has_ampersand);
			}
		}
	}

	return 0;
}
