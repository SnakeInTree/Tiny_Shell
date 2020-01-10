/*Kees VandenBerg - 260725510

COMP 310 - ASSIGNMENT 1 - tiny_shell.c file

*/

#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/time.h>
#include  <sys/resource.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include  <signal.h>
#include  <unistd.h>

//global char array  
char *line = NULL;

//multi-dimensional char array to hold all commands which have been sent to my_system
char strs[1024][150];

//records the number of commands executed by the current shell
int num_commands = 0;

//Function which takes char array ptr and returns the length of the char array. 
int length(char* str) {
     return strlen(str);
}

//Takes input from stdin --> should be updated to use fgets. 
char * get_a_line() {
     
     //Input char array                
     static char temp[1024];             
     
     //Display user prompt
     printf(" < ");

     //get user input and return
     gets(temp);
     return temp;
}


/* Function remove_char_from_string - Main purpose is to remove \n chars from a given String. 

     <params>

          - char c - character to be removed
          - char *str - string which the c is being removed from

 */
void remove_char_from_string(char c, char *str) {
    int i=0;
    int len = strlen(str)+1;

    //iterate through each char of a string
    for(i=0; i<len; i++)
    {
         //if we find the char 'c' to remove, all subsequent chars are shifted one to the left. 
        if(str[i] == c)
        {
            strncpy(&str[i],&str[i+1],len-i);
        }
    }
}

//removes whitespace, extra chars (newline, tab etc)
void tok(char *arr, char **argv) {

     while (*arr != '\0') {  //while not at the end of the line, contunue
                
          while (*arr == ' ' || *arr == '\t' || *arr == '\n' || *arr == '<' || *arr == '\n') {
               *arr++ = '\0'; //if the above chars found - remove them 
          }
                    
          *argv++ = arr;   // argv keeps track of arg position

          while (*arr != '\0' && *arr != ' ' && *arr != '\t' && *arr != '\n')  {
               arr++; //Skip arg until reaching the end
          }
     }
     *argv = '\0';   //array ends with NULL   
}

void execute(char **argv) {

     pid_t  pid;    
     int    status;

     pid = fork();

     if (pid < 0) {   //Child Process has error
          printf("Error in child process creation \n");
          exit(1);
     }
     else if (pid == 0) {   // Child process       
          
          //execvp takes as arg the file ptr and the rest of the array
          
          if (execvp(*argv, argv) < 0) {     
               printf("Error in child process  \n");
               exit(1);
          }
     }
     else {                                  //parent
          while (wait(&status) != pid)       // wait for completion  
               ;
     }

}

void handle_pipe(char * piped) {

     //piped is the piped comamand
     
     char *arg1[100];
     char *arg2[100];

     int status;

     char cpy1[100];
     char cpy2[100];

     //split the command around the '|'
     char* token = strtok(piped, "|"); 
     
     if (token[strlen(token)-1] == ' ') {
          token[strlen(token)-1] = 0;
     }

     //get the first part of the command into cpy1
     strcpy(cpy1, token);
     
     token = strtok(NULL, "|"); 

     //2nd part of the command into cpy2 
     strcpy(cpy2, token);

     //pass both commands through tok
     tok(cpy1, arg1);
     tok(cpy2, arg2);

     //make file descriptor array and pass to pipe() system call.
     int pid, fd[2];
     pipe(fd);
     pid = fork();

     if(pid==0) {   //child process

          //stdin coming from the read side of pipe
          dup2(fd[0], fileno(stdin));

          //close both sides of pipe
          close(fd[0]);
          close(fd[1]);

          //execute the second command, taking input from f[0]
          execvp(*arg2, arg2);

          //if the code reaches this pt, throw error, as execvp should not return.
          fprintf(stderr, "Failed to execute!\n");
          exit(1);
     }
     else { 

          //second fork to ensure return to parent process
          pid=fork();

          if(pid==0) { //inner child

               //instead of writing to stdout, write to fd[1]
               dup2(fd[1], fileno(stdout));

               close(fd[0]);
               close(fd[1]);

               //execute first half of piped cmd
               execvp(*arg1, arg1);
               fprintf(stderr, "Failed to execute!\n");
               exit(1);
          }
          else {

               //wait for the completion of the original process until completing
               int status;
               close(fd[0]);
               close(fd[1]);
               waitpid(pid, &status, 0);
          }
     }
}

/* Check to see if a file exists and is readable */
int cfileexists(const char * filename){
     FILE *file;
     char *cpy[100];

     //copy the filename into a temp var
     strcpy(cpy, filename);

     //remove trailing spaces
    remove_spaces(cpy);

    //try to open the file - if it is readable, return 1
    if (file = fopen(cpy, "r")){
        fclose(file);
        return 1;
    }
    return 0;
}

/* Function to remove trailing spaces and '<'*/
void remove_spaces(char* s) {
    const char* d = s;
    while (*s++ = *d++) {
        while (*d == ' ' || *d == '<') {
            ++d;
        }
    } 
}

//file handler function
void handle_file (char* fl[]) {
     
    char *str[150];
    FILE* fp;

     //if the file is an executable (like ./hog), we can pass it straight to the execute fn. 
     if (access(fl, F_OK|X_OK) == 0) {

          execute(fl);

     }    

     //if the file is not executable, we assume it is a list of commands
     else {
          
          fp = fopen(fl, "r");
          
          //iterate through the list of commands, and send them one by one to my_system, who treats them like individual commands. 
          while (fgets(str,150, fp)) {

                    remove_char_from_string('\n', str);
                    my_system(str);    
       
          }
          fclose(fp);

     }

    
}

//handler for the history command
void print_history() {

     int i = 0;
     //we only want to print the most recent 100 --> 
     if (num_commands > 100) {
          i = 100 - num_commands;
     }
     
     //iterate through strs and print all recorded commands.
     while(i < num_commands) {
          printf("%s\n", strs[i]);
          i++;
     }
}

//if the input command is 'cd', we want to go to the home directory.
void handle_single_cd() {
     chdir(getenv("HOME"));
}

//handle change to new dir
void handle_dir_change(char* str[]) {

     strtok(str, " ");

     //get the name of the new dir
     char *dir = strtok(NULL, " ");

     char cwd[1024];

     if(dir[0] != '/') {
        getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd,dir);
        chdir(cwd);
     }

     //concat the new dir with the curr dir and switch into it
    else {
        chdir(dir);
    }

}


//handle the command 'limit x'
void handle_limit(char* str[]) {

     //use strtok to isolate x - this being the desired memory value for the limit 
     strtok(str, " ");
     char *limitVal = strtok(NULL, " ");

     struct rlimit limit;

     /* Get current limit information  . */
     getrlimit(RLIMIT_DATA, &limit);

     //set the object's limit to the int val of x.
     limit.rlim_cur = atoi(limitVal);

     //confirm the new limit has been set. 
     setrlimit(RLIMIT_DATA, &limit);
     getrlimit(RLIMIT_DATA, &limit);
     
     return 0;

}

/* Handle ^C signal*/
void  INThandler(int sig)
{
     char buffer[100];

     //first, reset the signal function 
     signal(sig, SIG_IGN);
     printf("\n Do you want to quit? [y/n] ");
     
     fgets(buffer, 100 , stdin);
     
     remove_char_from_string('\n', buffer);
     
     //if the user says 'yes' - exit the shell. 
     int i = strcmp(buffer, "yes");
     int k = strcmp(buffer, "y");
     if ((i == 0) || (k == 0)) {
          exit(0);
     }
     else {
          //reset the Interrupt handler for the next time we press ^C
          signal(SIGINT, INThandler);
     }
}

/* Handle ^Z signal*/
void  stopHandler(int sig)
{
     //catch the signal for SIGTSTP (^Z)
     signal(sig, SIGTSTP);

     //reset the same handler for SIGTSTP (^Z) - ignores the call 
     signal(SIGTSTP, stopHandler);

}

void my_system(char* str[]){
     
     /*This is called because the limit command was causing libraries to be unmounted - making commands like 'ls' unusable.
     This way, every time a new string is received, we re-set the library path.*/
     putenv("LD_LIBRARY_PATH=/usr/local/lib");

     //copy the current command into the array of past commands (for history)
     strcpy(strs[num_commands], str);
     
     //iterate the number of commands
     num_commands++;

     //if str is exit - exit(0)
     int leave = strcmp(str, "exit");
     if (leave == 0) {
          exit(0);
     }

     char *argv[100];

     //check if the str is a file name - if so, pass the file to the handler fn.  
     int is_file = cfileexists(str);
     if (is_file) {
          handle_file(str);
     }

     //check if the str is a piped command - if so, pass the cmd to the handler fn.  
     if (strchr(str, '|') != NULL) {
          handle_pipe(str);
     }
     else {

          //flags to detect if the command is special - cd, limit or history
          int hist = strcmp(str, "history");
          int sing_cd = strcmp(str, "cd");
          int cd = hasPrefix(str, "cd");
          int chdir = hasPrefix(str, "chdir");
          int limit = hasPrefix(str, "limit");

          //if commands are special, pass to the appropriate handlers
          if (hist == 0) {
               print_history();
          }
          else if (sing_cd == 0) {
               handle_single_cd();
          }
          else if(cd == 0 || chdir == 0) {
               handle_dir_change(str);
          }

          else if(limit == 0) {
               handle_limit(str);
          }

          //if command is not special, we tok it, and sent the array to the execute function. 
          else {
               tok(str, argv);
               execute(argv);
          }
     }
}

/* Check if a given String p has the prefix q*/
int hasPrefix(char const *p, char const *q) {
    int i = 0;

    //iterate through the chars of q - only return true if all q chars match p chars at the same index
    for(i = 0; q[i]; i++) {
        if(p[i] != q[i])
            return -1;
    }
    return 0;
}

int main(int num_commands,char* command_list[]) { 

     //Set the two main signal handlers for the shell - SIGINT looking for CTRL + C, and SIGTSTP looking for CTRL + Z.
     signal(SIGINT, INThandler);
     signal(SIGTSTP, stopHandler);

     //isatty checks to see if the shell has accepted an argument at start
     int is_arg = isatty(0); 

     //if the shell has accepted an argument, we assume it is a file and begin to read it
     if (!is_arg) {
          
          char buffer[100];

          //continue reading to the end of the file
          while (fgets(buffer, 100 , stdin)) {

               //remove newline char from the String 
               remove_char_from_string('\n', buffer);

               //call mysystem to execute the string 
               my_system(buffer);
          }
     }
     
     //if the shell was not provided an argument on startup, we begin the interactive version of the shell. 
     else {

          while (1) {

               //get a line from stdin
               line = get_a_line();
               
               //if line is not empty, we execute my_system. 
               if (length(line) > 1) {
                    my_system(line);
               }
          }
     
     }
     return 0;
}