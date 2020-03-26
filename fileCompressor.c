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
int buildCodebook (int);
int fileReader (int);
int tableInsert (char *);
int tokenizer (char *, int);

//node struct which comprise our hashtable tree
struct node{
  //pointer to the next node in the given linked list
  struct node * next;
  //the key (token) of the specific node
  char * myKey;
  //the value (frequency) of the specific node (token)
  int frequency;
};

//heap represented as an array with its capacity
struct heap{
  //heap array
  struct node **arr;
  //current used capacity
  int used;
  //current capacity of the heap
  int cap;
};

//global variable which stores the names of the files from which we build the huffman codebook
char ** files;
struct node * hashTable[256];
struct heap * myHeap;

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
  int finalCounter = direcTraverse(myDirec, 0, 100, argv[2]);
  //check if there is a -b flag (might not be the first argument)
  if(strcmp(argv[1],"-b") == 0){
    //we need to build the huffman codebook
    buildCodebook(finalCounter);
  }
  //after building the hashtable, we need to store everything in a heap
  myHeap = (struct heap *)malloc(sizeof(struct heap));
  //start with an initial capacity of 100
  myHeap -> arr = (struct node **)malloc(101*sizeof(struct node *));
  myHeap -> cap = 100;
  //we need to transfer the hashtable into the heap

  //we're done!
  return 0;
}

/*transfers the contents of the hashTable into the heap*/
int heapTransfer(){
  //declare counters
  int i;
  struct node *temp;
  //loop through the hash table
  for(i = 0; i < 256; i++){
    //assign temp to the start of the table index
    temp = hashTable[i];
    //loop through the nodes at each index
    while(temp != NULL){
      //insert into the heap
      heapInsert(temp);
    }
    //traverse through the linked list
    temp = temp -> next;
  }
}

int increaseHeapSize(){
  //add 100 to the current heap capacity
}

int siftUp(struct node * child, int currPos){
  //we need to sift up our latest inserted node until its in the right spot
  struct node * parent = myHeap -> arr[(currPos-1)/2];
  struct node * temp;
  //check if the parent is indeed less than the child
  if(parent -> frequency > child -> frequency){
    //store the parent in temp
    temp = parent;
    //reassign the parent
    myHeap -> arr[(currPos-1)/2] = child;
    //reassign the child
    myHeap -> arr[currPos] = temp;
    //call siftup again to now check the parent
    siftUp(child, (currPos-1)/2);
  }
}

int heapInsert(struct node * toInsert){
  //check if there is space to insert the node
  if(myHeap -> used <= myHeap -> cap){
    //increase the heap size
    increaseHeapSize();
  }
  //insert the node at the end of the array and sift up accordingly
  myHeap -> arr[myHeap -> used] = toInsert;
  //sift up the last node accordingly
  siftUp(toInsert, myHeap -> used);
  //increment the used space
  return myHeap -> used += 1;
}

/* given a directory stream, traverses the directory/files and stores in the files array */
int direcTraverse (DIR *myDirectory, int counter, int currSize, char * currDirec){
  //stores the filepath of our subdirectories
  char filePBuff[PATH_MAX + 1];
  //in the case of recursion, update the filepath so that we do not get lost
  strcpy(filePBuff, currDirec);
  //add a forward-slash at the end to get ready to add more to the path
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
      //we are back in the original file, get rid of the previous file path
      strcpy(filePBuff, currDirec);
      //put the forward-slash back in there
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

/* iterates through the files and first gets the frequencies of the tokens */
int buildCodebook (int filesSize){
  //random local variables
  int counter;
  //loop through the files array
  for(counter = 0; counter < filesSize; counter++){
    //open the file at the specific index to a read only mode
    int fd = open(files[counter], O_RDONLY);
    //check is the given file exists
    if(fd < 0){
      //file does not exist it is a fatal error
      printf("FATAL ERROR: File does not exist\n");
      //exit the code
      exit(1);
    }
    //hand the fd over to the build frequencies method to update our hash table
    fileReader(fd);
  }

  return 0;
}

/* Reads the given file and loads it into a local buffer */
int fileReader (int fileDescriptor){
  //buffer size
  int buffSize = 100;
  //create the buffer to store the file
  char *myBuffer = (char *)malloc(101*sizeof(char));
  //check if the pointer is null
  if(myBuffer == NULL){
    //print an error
    printf("ERROR: Not enough space on heap\n");
  }
  //set everything in the buffer to the null terminator
  memset(myBuffer, '\0', 101);
  //store the status of the read
  int readIn = 0;
  int status = 1;
  //temp pointer to hold the dynamically sized buffer
  char *temp;
  //loop until everything has been read
  while(status > 0){ 
    //read buff size number of chars and store in myBuffer
    do{
      status = read(fileDescriptor, myBuffer+readIn, 100);
      readIn += status;
    }while(readIn < buffSize && status > 0);
    //check if there are more chars left
    if(status > 0){
      //increase the array size by 100
      buffSize += 100;
      //store the old values in the temp pointer
      temp = myBuffer;
      //malloc the new array to myBuffer
      myBuffer = (char *)malloc(buffSize*sizeof(char));
      //check if the pointer is null
      if(temp == NULL){
        //print an error
        printf("ERROR: Not enough space on heap\n");
      }
      //copy the old memory into the new buffer
      memcpy(myBuffer, temp, readIn);
      //free the old memory that was allocated
      free(temp);
    }
  }
  //we hand over the contents of our file (stored in buffer) to the table loader function
  tokenizer(myBuffer, buffSize);
  //finish it
  return 0;
}

/* After reading the complete contents of a given file, we need to split the file up into tokens
and then load each token into the table or update the token's frequency */
int tokenizer (char *buff, int buffSize){
  //counter to loop through the buffer
  int counter = 0;
  //counter to loop through the cdata
  int localcount = 0;
  char ctemp;
  //allocate starting space into cdata
  char *cdata = (char *)malloc(11*sizeof(char));
  char *temp;
  //check if the pointer is null
  if(cdata == NULL){
    //print an error
    printf("ERROR: Not enough space on heap\n");
  }
  //set the array to be all null terminators
  memset(cdata, '\0', 11);
  //current size of cdata
  int currSize = 10;
  //loop through the string until we hit a comma or terminator
  while(buff[counter] != '\0'){
    ctemp = buff[counter];
    //check if we have hit a space
    if(buff[counter] == ' '){
    
      //isolate the token and insert it into the table
      tableInsert(cdata);
      //allocate a new cdata for the next token
      cdata = (char *)malloc(11*sizeof(char));
      //reset the local count
      localcount = 0;
      //reset the curr size
      currSize = 10;
    }
    //store the character in the node array
    else if(localcount == currSize){
      //make the cdata array 10 characters bigger
      currSize += 10;
      //store the old values in the temp pointer
      temp = cdata;
      //malloc the new array to myBuffer
      cdata = (char *)malloc(currSize*sizeof(char));
      //check if the pointer is null
      if(cdata == NULL){
        //print an error
        printf("ERROR: Not enough space on heap\n");
      }
      //set the random stuff to null terminators again
      memset(cdata, '\0', currSize);
      //copy the old memory into the new buffer
      memcpy(cdata, temp, localcount);
      //free the old variable
      free(temp);
      //store the buffer char
      cdata[localcount] = buff[counter];
      //decrement counter so that you read again
      counter--;
    } else{
      //store the buffer character into the cdata character
      cdata[localcount] = buff[counter];
    }
    //increment counters
    counter++;
    localcount++;
  }
  return 0;
}

/* for every token find its hash code and insert it into the table accordingly */
int tableInsert(char *token){
  //calculate the hash code
  int hashCode = (int)*token;
  //mod it to fit inside the 256 length array
  int index = hashCode % 256;
  //check to see if the head node of the given index is Null
  if(hashTable[index] == NULL){
    //create a new node and put er in there
    struct node * newNode = (struct node *)malloc(sizeof(struct node));
    //populate the node's values
    newNode -> myKey = token;
    newNode -> frequency = 1;
    newNode -> next = NULL;
    //gtfo
    return 0;
  } else{
    //curr node to loop through the list of nodes
    struct node * curr = hashTable[index];
    //there already exists a headnode and we must loop through to find out token
    while(curr != NULL){
      //check if curr is our token
      if(strcmp(curr -> myKey, token) == 0){
        //update the frequency and gtfo
        curr -> frequency++;
        return 0;
      } else{
        if(curr -> next == NULL){
          //create a new node and put er in there
          curr -> next = (struct node *)malloc(sizeof(struct node));
          //increment curr
          curr = curr -> next;
          //populate the node's values
          curr -> myKey = token;
          curr -> frequency = 1;
          curr -> next = NULL;
          //gtfo
          return 0;
        }
        //increment curr
        curr = curr -> next;
      }
    }
  }
  return 0;
}

/* We need to create a codebook text file  */