/* 
Ernest Viola
Professor John Carroll
CS570
Program 4
December 6, 2017
Handled '< >' and '> <' as errors as it was not specified to what functionality was supposed to be given
*/

// all includes


#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>

//#include "CHK.h"
#include "p2.h"
#include "getword.h"

char s[STORAGE*2550]; //character storage with enough spaces
char* newargv[MAXITEM]; //pointers to each cmd
char* pipPtr[100]; //pointers to each location of pipe start
int pipLocation[100]; //finds the nulls for every pipe in newargv

int i, j, bang, length, amp, inCount, outCount, fileIn, fileOut, errCode;

int skipPrompt = 0;

int ctr; //counter for size of newargv
int pipCtr; //counter for number of pipes

char* output; //pointer for >
char* input; //pointer for <
char* bangOutput; //pointer for >!
char* bangCmd = {">!"}; //for string compare for >!

//===================START FUNCTIONS===================//


//===================START SIGNAL HANDLER FUNCTION===================//
// Handles signal for sigterm at kill
void myhandler(int signum) {
}
//====END FUNCTION===//

//===================START PRINT PROGRAM FUNCTION===================//
//print prompt 'p2: '
void printP() {
	printf("p2: ");
}
//====END FUNCTION===//

//===================START MAIN===================//


main() {	
	int kidpid, grandkid; //initialize children for forks
	char* home = getenv("HOME"); //get home environment for cd
	char* cd = {"cd"}; //for string compare for cd
	char* lsF = {"ls-F"}; //for string compare for ls-F
	char* execute = {"exec"}; //for string compare for exec

	setpgid(0,getpid()); // pid of current process for terminal
	signal(SIGTERM, myhandler); // Is used to send the termination request

	for(;;) {
		
		struct stat sb;//struct for stat and lstat
		DIR* d;
		struct stat statbuf;
		struct dirent* dir;
		int status;
		//initialize integers
		i = length = ctr = amp = pipCtr = inCount = outCount = fileIn = fileOut = errCode = 0;
		if (skipPrompt == 0) {
			printP(); //prompt
		}
		else {
			skipPrompt = 0;
		}

		if (parse() == -1) { //EOF exit condition 
			break;
		}

		if (length == -2) // Special error message for unclosed parenthesis
		{
			fprintf(stderr, "Unmatched quotes\n");
			skipPrompt = 1;
			continue;
		}
		if ((inCount == 1 || outCount == 1 || bang == 1) && ctr <= 2) // Checks to see if enough arguments for redirection
		{
			fprintf(stderr,"Redirect: Not enough arguments\n");
			skipPrompt = 1;
			continue;
		}
		if (errCode != 0) //Handles all errors in redirection and parsing from parse() function
		{
			switch(errCode)
			{
				case 1:
					fprintf(stderr,"Redirect: > is invalid\n");
					skipPrompt = 1;
					continue;
				case 2:
					fprintf(stderr,"Redirect: < is invalid\n");
					skipPrompt = 1;
					continue;
				case 3:
					fprintf(stderr,"Bang: >! is invalid\n");
					skipPrompt = 1;
					continue;
				case 4:
					fprintf(stderr,"Background: & is invalid\n");
					skipPrompt = 1;
					continue;
				case 5:
					fprintf(stderr,"Syntax: invalid\n");
					skipPrompt = 1;
					continue;
			}
		}

		if (ctr == 2) // ls-F with current directory
		{
			if (strcmp(newargv[0],lsF) == 0)
			{
				i = 0;
				if ((d = opendir(".")) == NULL) // Opens current directory else error
				{
					perror("Failed to open");
					skipPrompt = 1;
					continue;
				}
				if (d)
				{
					while ((dir = readdir(d)) != NULL) // Reads all in directory until end
					{

						// printf("%s\n", dir->d_name);


						if (lstat(dir->d_name,&sb) == -1)
						{
							perror("lstat");
							skipPrompt = 1;
							continue;
						}

						int statfile = sb.st_mode & S_IFMT;

						if (statfile == S_IFDIR) {
							printf("%s/\n", dir->d_name);
						}
						else if (statfile == S_IFLNK) {
							//used to get the actual path name for use in realpath
							char actualpath [PATH_MAX+1];
							char *ptr;
							ptr = realpath(dir->d_name, actualpath);
							if (stat(ptr,&sb) == -1) { //its a broken link
								printf("%s&\n", dir->d_name);
							} else { // working
								printf("%s@\n", dir->d_name);
							}
						}
						else if (sb.st_mode & S_IXUSR) {
							printf("%s*\n", dir->d_name);
						}
						else if (statfile == S_IFREG) {
							printf("%s\n", dir->d_name);
						}
					}
			
					closedir(d);
					continue;
				}
			}
		}

		if (ctr >= 3) //ls-F more than one directory
		{
			if (strcmp(newargv[0],lsF) == 0)
			{
				i = 0;
				if (newargv[1] == '\0') {
					fprintf(stderr, "Can't pipe with ls-F\n");
					skipPrompt = 1;
					continue;
				}
				if ((d = opendir(newargv[1])) != NULL)
				{
					while ((dir = readdir(d)) != NULL)
					{

						// slash and result are used to create the correct path for each file in order to use lstat
						char *slash = {"/"};
						char *result = malloc(strlen(newargv[1])+strlen(slash)+strlen(dir->d_name)+1);

						strcpy(result, newargv[1]);
						strcat(result, slash);
						strcat(result,dir->d_name);

						if (lstat(result,&sb) == -1)
						{
							perror("lstat");
							skipPrompt = 1;
							continue;
						}

						int statfile = sb.st_mode & S_IFMT;

						if (statfile == S_IFDIR) {
							printf("%s/\n", dir->d_name);
						}
						else if (statfile == S_IFLNK) {
							//used to get the actual path name for use in realpath
							char actualpath [PATH_MAX+1];
							char *ptr;
							ptr = realpath(result, actualpath);
							if (stat(ptr,&sb) == -1) { //checks to see if its a broken link
								printf("%s&\n", dir->d_name);
							} else { // working
								printf("%s@\n", dir->d_name);
							}
						}
						else if (sb.st_mode & S_IXUSR || sb.st_mode & S_IXGRP || sb.st_mode & S_IXOTH) {
							printf("%s*\n", dir->d_name);
						}
						else if (statfile == S_IFREG) {
							printf("%s\n", dir->d_name);
						}

						free(result);
					}
					closedir(d);
					continue;
				}
				else // uses stat in order to check if file instead of directory
				{
					if (stat(newargv[1],&sb) == -1)
					{
						fprintf(stderr, "%s:Not readable\n",newargv[1]);
						skipPrompt = 1;
						continue;
					}
					int statfile = sb.st_mode & S_IFMT; // Code to see if regular file instead
					if (statfile == S_IFREG)
					{
						printf("%s\n", newargv[1] );
						continue;
					}
					fprintf(stderr, "%s:Not readable\n",newargv[1]);
					skipPrompt = 1;
					continue;
				}
			}
		}


		// start cd
		if (ctr == 2) // CD for cd by itself sends to HOME of user
		{
			if (strcmp(newargv[0],cd) == 0) //Checks to see if CD is called
			{
				if (chdir(home) == -1) // Unable to open HOME
				{
					perror("Directory not found");
					skipPrompt = 1;
					continue;
				}
				continue;
			}
		}
		if (ctr >= 3) // CD with arguments
		{
			if (strcmp(newargv[0],cd) == 0) //Checks to see if CD is called
			{
				if (ctr >= 4) 
				{
					fprintf(stderr, "Too many arguments\n");
					skipPrompt = 1;
					continue;
				}
				if (chdir(newargv[1]) == -1) // Invalid Directory
				{
					perror("Directory not found");
					skipPrompt = 1;
					continue;
				}
			}
		}
		// end cd


		fflush( stdin );
		fflush( stderr );


		// exec built-in
		if (ctr >= 3)
		{
			if (strcmp(newargv[0],execute) == 0)
			{
				//executes in current process in order to exit and executes starting at next command
				execvp(newargv[1],newargv+1);
				perror("Exec: execvp failed");
				_exit(9);
			}
		}
		if(-1 == (kidpid=fork())) // Fork is used to create a second process to handle execvp
		{
			perror("Cannot fork");
			_exit(1);
		}
		if (0 == kidpid) {

			if (bang == 1) { //>!
			//truncates to remove the text of the file and overwrites
				if ((fileOut = open(bangOutput, O_WRONLY | O_CREAT | O_TRUNC)) == -1) {
						fprintf(stderr,"%s:Is not readable\n",bangOutput);
						_exit(1);
				}
				// Changes the STDOUT to the STDIN of the created file
				if (dup2(fileOut, STDOUT_FILENO) == -1)
				{
					perror("Can't redirect");
					_exit(1);
				}
				close(fileOut);
			}


			if (outCount == 1) // >
			{
				// Creates new file to be written to if error then prints error
				if ((fileOut = open(output, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR)) == -1) {
						perror("Redirect");
						_exit(1);
				}
				// Changes the STDOUT to the STDIN of the created file
				if (dup2(fileOut, STDOUT_FILENO) == -1)
				{
					perror("Can't redirect");
					_exit(1);
				}
				close(fileOut);
			} // END >
			
			if (inCount == 1) // <
			{
				// Opens file to be read
				if ((fileIn = open(input, O_RDONLY)) == -1) // can't test if opening
				{
					perror("Failed to open");
					_exit(1);
				}
				// Changes the STDIN to the STDOUT of the file pointed at
				if (dup2(fileIn, STDIN_FILENO) == -1)
				{
					perror("Can't redirect");
					_exit(1);
				}
				close(fileIn);
			} // END <
			if (pipCtr > 0) // Piping
			{
				//piping was done by having certain test cases where the oldest child will always do the furthest right command and the second newest child will always do the furthest left command. my fork() loop will create an extra child at the very end in which I have to call for it to exit as I reach the last command
				int fildes[pipCtr][2];
				pipe(fildes[0]); // initialize pipe
				
				// Nothing to be pipped to, unable to continue, missing arguments
				if (pipPtr[0] == NULL)
				{
					fprintf(stderr, "Nothing to pipe to\n");
					_exit(1);
				}
				
				// Unable to fork in order to handle piping
				if(-1 == (grandkid=fork())) //error for piping output of LHS is connected to input of RHS
				{
					perror("Cannot pipe");
					_exit(1);
				} 
				else if (0 == grandkid) // go into grandchild of child to initialize left side fork this is LS
				{
					for (i = 0; i <= pipCtr - 1; i++) {
						fflush( stdin );
						fflush( stderr );

						if( i != pipCtr - 1) pipe(fildes[i+1]);

						if (fork() == 0) {
							if (i == pipCtr - 1) //exit on the extra fork
							_exit(1);
						}
						else {
							if (i == pipCtr - 1) { // handle most left
							wait(NULL);
								dup2(fildes[i][1], STDOUT_FILENO);
								close(fildes[i][0]);
								execvp(newargv[0], newargv);
								perror("execvp");
								_exit(1);
							}
							else { // handle middle
							wait(NULL);
								dup2(fildes[i+1][0], STDIN_FILENO);
								dup2(fildes[i][1], STDOUT_FILENO);
								for (j = i; j <= i+1; j++){
									close(fildes[j][1]);
									close(fildes[j][0]);
								}
								execvp(pipPtr[pipCtr - 2 - i], newargv + pipLocation[pipCtr - 2 - i]+1);
								perror("execvp");
								_exit(1);
							}
						}
					}		
							// else {
							// 	printf("CMD: %s %d\n", newargv[0], i);
							// 	printf("Loc: %d \n", newargv + 
				}
				else // child this furthest right command
				{
					wait(NULL);
					//Changes STDIN of Pipe to STDIN of File
					dup2(fildes[0][0], STDIN_FILENO);
					close(fildes[0][1]);
					execvp(pipPtr[pipCtr-1], newargv + pipLocation[pipCtr-1] + 1);
					perror("execvp");
					_exit(1);
				}
				continue;
			} // END PIPE
			// execvp takes in an array of pointers and increments until reaches null
			execvp(newargv[0],newargv);
			fprintf(stderr,"%s:Command cannot execute\n",newargv[0]); // is never reached unless execvp fails
			_exit(9);
		}
		else {
			// parent checks for '&' if there is then skip and let child run @ same time {
			if (amp != 1) //Waits for child unless bg process is called
			while (wait(NULL) != kidpid);
			if (amp == 1) { // If bg process then print cmd and pid
				printf("%s [%d]\n",newargv[0], kidpid );
			}
		}		// end fork
	}	// start kill
	killpg(getpgrp(), SIGTERM);
	printf("p2 terminated.\n");
	exit(0);
}

//parse is used to create the newargv array in which this program reads in commands
int parse() {
	char* w = s;
	//set integers
	outCount = inCount = bang = 0;
	//set pointers
	output = bangOutput = input = NULL;
	int meta = 0; // counter for #metacharacters
	
	for(;;) {

		length = getword(w);
		// printf("Start:%s:End\n",w );
		if (length == -2) break;
		// printf("%s %d \n", w, length);
		if (meta == 1) // Checks for invalid syntax when a metacharacter is previously called
		{
			if (*w == '>' || *w == '<' || *w == '|' || *w == '&' || strcmp(w,bangCmd)==0)
			{
				errCode = 5;
				meta --;
				break;
			}
		}
		if (*w == '>' && length == 1) // stdout
		{
			if ( outCount == 1)
			{
				errCode = 1;
				break;
			}
			outCount++;
			w = w + length + 1;
			output = w; // gets address of next word
			meta++;
		}
		else if (*w == '<' && length == 1) // stdin issues 
		{
			if (inCount == 1)
			{
				errCode = 2;
				break;
			}
			inCount++;
			w = w + length + 1;
			input = w; // gets address of next word
			meta++;
		}
		else if (strcmp(w,bangCmd)==0) // bang
		{
			if (bang == 1)
			{
				errCode = 3;
				break;
			}
			bang++;
			w = w + length + 1;
			bangOutput = w; // gets address of next word
			meta++;
		}
		else if (*w == '|' && length == 1) { // we have a pipe
			pipLocation[pipCtr] = ctr;
			newargv[ctr] = '\0'; //sets pipe in newargv to null
			ctr++;
			w = w + length + 1; //next char after pipe
			pipPtr[pipCtr] = w; //address of next char after pipe
			pipCtr++;
		}
		else if (*w == '&' && length == 1) // sets flag for bg process
		{
			if (amp == 2)
			{
				errCode = 4;
				break;
				ctr++;
			}
			amp++;
			ctr++;
			break;
		}
		else if (meta == 1) // skips storing address of word for redirection
		{
			w = w + length + 1;
			meta = 0;
		}
		else { // we have a non-metacharacter
			newargv[ctr] = w;
			w = w + length + 1;
			ctr++;
		}

		if (length <= 0) break; //breaks loop when EOF is found
	} //end loop

	newargv[ctr -1] = '\0';
	return length;
}