#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv){
    //Load config files
    //Run command loop
    lsh_loop();


    //Perform shutdown/clean up
}

/* Basic loop of a shell
Read: Read the command from standard input
Parse: Separate the command string into a program and arguments
Execute: Run the parsed command
*/

void lsh_loop(){
    char* line; //The address of a char or points to an array of characters makes a word
    char** args; //points to an array of words 
    int status; //Status of the terminal

    do{
        printf("> ");
        line = lsh_read_line(); //Reads the user input
        args = lsh_split_line(line); //Splits the line and parses it
        status = lsh_execute(args); //Executes the arguments 

        free(line);
        free(args);
    }while(status);
}

/*
Reading a line
This implementation would allow the user to enter as many characters as they want
By dynamically allocating more memory for the user input
*/

#define LSH_RL_BUFSIZE 1024 //Allocate 1024 to
char *lsh_read_line(void){ //Returns a word or pointer to array of characters
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char* buffer = malloc(sizeof(char) * bufsize);
    int c;

    //Check malloc
    if(!buffer){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        //Read a character
        c = getchar();

        //If we hit EOF, replace it with a null character and return
        if(c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }

        position++;

        //If we have exceeded the buffer, reallocate
        if(position >= bufsize){
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize); //Reallocate using the base address to the new size
            if(!buffer){
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*
The above function can be compacted by using getline() from stdio.h
*/

char* lsh_read_line2(void){
    char* line = NULL;
    ssize_t bufsize = 0;

    if(getline(&line, &bufsize, stdin) == -1){
        if(feof(stdin)){
            exit(EXIT_SUCCESS);
        } else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}


/*
Similar to the functions above in structure but this time using strtok
*/
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char** lsh_split_line(char* line){
    int bufsize = LSH_RL_BUFSIZE, position = 0;
    char** tokens = malloc(bufsize * sizeof(char *));
    char* token;

    if(!tokens){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while(token != NULL){
        tokens[position] = token;
        position++;

        if(position >= bufsize){
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens){
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/*
    Function declarations for builtin shell commands
*/
int lsh_cd(char** args);
int lsh_help(char** args);
int lsh_exit(char** args);

/*
list of builtin commands, followed by their corresponding functions
*/
char* builtin_str[] = { //equiv to char** i think since it is an array of pointers
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}

/*
    Built in function implementations
*/
int lsh_cd(char** args){
    if(args[1] == NULL){
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if(chdir(args[1]) != 0){
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char** args){
    int i;
    printf("Samuel Briones-Plascencia's LSH\n
            Type program names and arguments, and hit enter.\n
            The following are built in:\n");

    for(i = 0; i < lsh_num_builtins(); i++){
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use the man command for more information on other programs.\n");
    return 1;
}

int lsh_exit(char** args){
    return 0;
}
/*
*/

int lsh_launch(char** args){
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid == 0){ //Child process
        if(execvp(args[0], args) == -1){ //execvp is unix based this is the execute command
            //execvp completely overrules any code after executed unless an error occurs
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0){
        //Error in forking
        perror("lsh");
    } else {
        //Parent process
        do{
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int lsh_execute(char ** args){
    int i;

    if(args[0] == NULL){
        return 1;
    }

    for(i = 0; i < lsh_num_builtins(); i++){
        if(strcmp(args[0], builtin_str[i]) == 0){
            return(*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}