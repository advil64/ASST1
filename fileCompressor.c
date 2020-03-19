/* reading from file requires standard io library and others */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#define TRUE 1
#define FALSE 0

//prototypes
int direcTraverse (DIR *, int, int, char *);

//global variable which stores the names of the files from which we build the huffman codebook
char ** files;

/* The brains of the operation */
int main (int argc, char ** argv){

  /* TODO: How do I check if we must do our command recursively? 
  Can there be multiple flags given in the command line?
  */

  //make space in the files array
  files = (char **) malloc(100 * sizeof(char *));

  //open the directory stream
  DIR *myDirec = opendir(argv[2]);
  //check is the given file exists
  if(myDirec < 0){
    //file does not exist it is a fatal error
    printf("FATAL ERROR: File does not exist\n");
    //exit the code
    exit(1);
  }
  //pass along the file directory to the traversal method which stores the files
  direcTraverse(myDirec, 0, 100, argv[2]);
  //check if there is a -b flag (might not be the first argument)
  if(strcmp(argv[1],"-b") == 0){
    //we need to build the huffman codebook
  }
  //we're done!
  return 0;
}

/* given a directory stream, traverses the directory/files and stores in the files array */
int direcTraverse (DIR *myDirectory, int counter, int currSize, char * currDirec){
  //stores the filepath of our subdirectories
  char filePBuff[PATH_MAX + 1];
  strcpy(filePBuff, currDirec);
  strcat(filePBuff, "/");
  //the dirent struct which holds whatever readdir returns
  struct dirent *currDir;
  //loop through the contents of the directory and store in files array
  while((currDir = readdir(myDirectory)) != NULL){
    //skip the . and ..
    if(strcmp(currDir->d_name, ".") == 0 || strcmp(currDir->d_name, "..") == 0){
      //skip the iteration
      continue;
    }
    //first check if the currdir is a file or a directory
    if(currDir->d_type == DT_DIR){
      //add the directory in question to the path
      strcat(filePBuff, currDir->d_name);
      //traverse the new directory
      counter = direcTraverse(opendir(filePBuff), counter, currSize, filePBuff);
      strcpy(filePBuff, currDirec);
      strcat(filePBuff, "/");
      //find the new max size of the array
      currSize = ((counter%100)+1)*100;
    } else{
      //allocate space for the file path
      files[counter] = malloc((PATH_MAX+1) * sizeof(char));
      //add the file path to the array
      strcpy(files[counter],filePBuff);
      //store the names of the files in our files array
      strcat(files[counter], currDir->d_name);
      //just to test the code
      printf("%s\n", files[counter]);
      //check if files array needs more space
      if(++counter >= currSize){
        //realloc 100 more spaces in our files array
        files = realloc(files, (currSize+100) * sizeof(char *));
        //change the curr size accordingly
        currSize += 100;
      }
    }
  }
  //return the current count
  return counter;
}

/* given a file descriptor, builds the codebook */
int buildCodebook (){
return 0;
}