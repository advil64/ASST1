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
#include <time.h>

#define TRUE 1
#define FALSE 0

//node struct which comprise our hashtable tree
struct node{
  //identify what type of node we are dealing with 0->parent 1->leaf
  int identifier;
  //pointer to the next node in the given linked list
  struct node * next;
  //the key (token) of the specific node
  char * myKey;
  //the value (frequency) of the specific node (token)
  int frequency;
  //this is the huffman parent node
  struct node * rChild;
  struct node * lChild;
};

//prototypes
int direcTraverse (DIR *, int, int, char *);
int buildCodebook (int);
int fileReader (int);
int tableInsert (char *);
int tokenizer (char *, int);
int buildSubTrees();
struct node * pop();
int siftDown(struct node *, int);
int heapTransfer();
int increaseHeapSize();
int siftUp(struct node *, int);
int heapInsert(struct node *);
void printHeap();
int printTable();
void freeFiles(int);
int decompressFiles(int);
void buildHuffTree(char *, int);
int codebookReader (int);
int huffInsert(char *, char *, struct node *);
int readHcz(int, int);
int huffmanCodebookReader (int);
int huffTokenizer (char *, int);
int hInsert (char *, char *);
int compressionFileReader (int, int);
int compressionWriter(char *, int);
int savTokenizer (char *, int, int);
int compressFiles(int);
int freeHuffmanTable();
int depthFirstSearch(struct node *, int, char *);

//heap represented as an array with its capacity
struct heap{
  //heap array
  struct node **arr;
  //current used capacity
  int used;
  //current capacity of the heap
  int cap;
};

//struct for the comparision nodes hashtable
struct huffNode { // the huffman codebook nodes
  char * token; // the token
  char * bittok; // the bitcode
  struct huffNode * next; // the next node in the linked list (these nodes will be stored in a hashtable)
};

//global variables
char ** files;
struct node * hashTable[256];
struct huffNode * huffmanTable[256];
struct heap * myHeap;
struct node * huffHead;
char * escapeSequence;

/* The brains of the operation */
int main (int argc, char ** argv){
  //TODO: delete in final iteration time measure
  clock_t tic = clock();
  //booleans indicating flags the -b flag was passed
  int build = 0;
  //the -R flag was passed
  int recursive = 0;
  //the -c flag was passed
  int compress = 0;
  //the -d flag was passed
  int decompress = 0;
  //counter for the loop
  int i = 0;
  //stores the number of files given
  int fileCounter;
  //loop through the arguments
  for(i = 0; i < argc; i++){
    //check the flags / set the flags
    if(strcmp(argv[i], "-b") == 0){
      //set the build flag to be true
      build = 1;
    } else if(strcmp(argv[i], "-R") == 0){
      //set the recursive flag to be true
      recursive = 1;
    } else if(strcmp(argv[i], "-c") == 0){
      //we need to compress the given file
      compress = 1;
    } else if(strcmp(argv[i], "-d") == 0){
      //we need to decompress the given file
      decompress = 1;
    } else if(build){
      //first check that you don't have an additional argument
      if(i != argc-1){
        //you should only pass in at most 3 arguments for the build codebook part
        printf("WARNING: No need to pass the Huffman Codebook for -b, did you mean to -c or -d instead?\n");
        //exit this mofo
        exit(0);
      }
      //we're building a hash table, memset that mofo
      memset(hashTable, 0x0, 256*sizeof(struct node *));
      //check if we have to traverse multiple files
      if(recursive){
        //make space in the files array
        files = (char **) malloc(100 * sizeof(char *));
        //check is the given file exists (we ran out of memory)
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(0);
        }
        //open the directory stream
        DIR *myDirec = opendir(argv[i]);
        //check is the given file exists
        if(!myDirec){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Directory does not exist, please use -b flag ONLY if your argument is a file\n");
          //exit the code
          exit(1);
        }
        //pass along the file directory to the traversal method which stores the files
        fileCounter = direcTraverse(myDirec, 0, 100, argv[i]);
        //close the directory
        closedir(myDirec);
      } else{
        //only make space for one file path
        files = (char **) malloc(1 * sizeof(char *));
        //check is the given file exists
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(0);
        }
        //the only file
        files[0] = argv[i];
        //assign 1 to the file counter variable
        fileCounter = 1;
      }
      //we need to build the huffman codebook
      if(buildCodebook(fileCounter)){
        //1 is unsuccessful, and zero is successfull
        printf("FATAL ERROR: Unable to build codebook\n");
        //exit
        exit(0);
      }
      //after building the hashtable, we need to store everything in a heap
      myHeap = (struct heap *)malloc(sizeof(struct heap));
      //check for memory capacity
      if(!myHeap){
        //file does not exist it is a fatal error
        printf("FATAL ERROR: Not enough memory\n");
        //exit the code
        exit(0);
      }
      //start with an initial capacity of 101
      myHeap -> arr = (struct node **)malloc(101*sizeof(struct node *));
      //check for memory capacity
      if(!(myHeap->arr)){
        //file does not exist it is a fatal error
        printf("FATAL ERROR: Not enough memory\n");
        //exit the code
        exit(0);
      }
      //the array initially has 100 spaces with 1 space hidden
      myHeap -> cap = 100;
      //set all the elements to null just in case
      memset(myHeap -> arr, 0x0, (myHeap -> cap)*sizeof(struct node *));
      //the heap is initially empty
      myHeap -> used = 0;
      //we need to transfer the hashtable into the heap
      if(heapTransfer()){
        //transfering hashtable into the minheap was unsuccessfull
        printf("FATAL ERROR: Transfering hashtable into the minheap was unsuccessfull\n");
        //exit
        exit(0);
      }
      //now we need to start building the Huffman Tree
      if(buildSubTrees()){
        //build subtrees was unsuccessful
        printf("FATAL ERROR: Building subtrees was unsuccessful\n");
        //exit
        exit(0);
      }
      //once the subtrees are built, we need to write to a huffman codebook, FIRST CREATE a codebook file
      int codFD = open("HuffmanCodebook", O_TRUNC | O_RDWR | O_CREAT,  S_IRUSR | S_IWUSR);
      //creating the file is unsuccessfull
      if(codFD < 0){
        //unable to open the codebook for whatever reason
        printf("FATAL ERROR: Unable to open the huffman codebook file\n");
      }
      //assign the value of the escape sequence
      escapeSequence = "$\0";
      //write the escape character being used
      write(codFD, "$\n", 2);
      //call the DFS to calculate the huffman codes
      depthFirstSearch(huffHead, codFD, "");
      //close the file descriptor once we're done writing
      close(codFD);
    } else if(decompress){
      //first makesure that you have a codebook argument
      if(i == argc-1){
        //there is no huffman codebook argument
        printf("ERROR: to use the decompress flag you must provide a huffman codebook\n");
        //exit this sucker
        exit(0);
      }
      //check out how we're traversing this directory
      if(recursive){
        //make space in the files array
        files = (char **) malloc(100 * sizeof(char *));
        //check is the given file exists
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(0);
        }
        //open the directory stream
        DIR *myDirec = opendir(argv[i]);
        //check is the given file exists
        if(!myDirec){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Directory does not exist, please use -b flag ONLY if your argument is a file\n");
          //exit the code
          exit(0);
        }
        //pass along the file directory to the traversal method which stores the files
        fileCounter = direcTraverse(myDirec, 0, 100, argv[i]);
        //close the directory
        closedir(myDirec);
      } else{
        //there is only one file to decompress
        files = (char **) malloc(1 * sizeof(char *));
        //check is the given file exists
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(0);
        }
        //the only file
        files[0] = argv[i];
        //assign 1 to the file counter variable
        fileCounter = 1;
      }
      //open the codebook
      int cdFd = open(argv[++i], O_RDONLY);
      //make a warning if it's not correct
      if(cdFd < 0){
        //codebook not present
        printf("WARNING: Codebook not present\n");
      }
      //allocate the parent node for the huffman tree TODO: free this tree
      huffHead = (struct node *)malloc(sizeof(struct node));
      //check for memory
      if(!huffHead){
        //codebook not present
        printf("FATAL ERROR: Not enough memory\n");
      }
      //recreating the huffman tree, make both children null
      huffHead -> rChild = NULL;
      huffHead -> lChild = NULL;
      //we need to build the huffman tree
      if(codebookReader(cdFd)){
        //we were unable to read the codebook
        printf("FATAL ERROR: Unable to read the huffman codebook\n");
        //exit
        exit(0);
      }
      //decompress files reads our file(s) and converts back into regular text
      if(decompressFiles(fileCounter)){
        //we were unable to read the codebook
        printf("FATAL ERROR: Unable to read the decompress files\n");
        //exit
        exit(0);
      }
      //close the file descriptor used to read the codebook
      close(cdFd);
    } else if(compress){
      //first makesure that you have a codebook argument
      if(i == argc-1){
        //there is no huffman codebook argument
        printf("ERROR: to use the compress flag you must provide a huffman codebook\n");
        //exit this sucker
        exit(0);
      }
      //check out how we're traversing this directory
      if(recursive){
        //make space in the files array
        files = (char **) malloc(100 * sizeof(char *));
        //check is the given file exists
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(1);
        }
        //open the directory stream
        DIR *myDirec = opendir(argv[i]);
        //check is the given file exists
        if(!myDirec){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Directory does not exist\n");
          //exit the code
          exit(1);
        }
        //pass along the file directory to the traversal method which stores the files
        fileCounter = direcTraverse(myDirec, 0, 100, argv[i]);
        //close the directory
        closedir(myDirec);
      } else{
        //there is only one file to decompress
        files = (char **) malloc(1 * sizeof(char *));
        //check is the given file exists
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(1);
        }
        //the only file
        files[0] = argv[i];
        //assign 1 to the file counter variable
        fileCounter = 1;
      }
      // declaring file descriptor for Huffman Codebook
  	  int huffFD = open(argv[++i], O_RDONLY);
      //check to see if huffman codebook exists
      if(huffFD < 0){
        printf("FATAL ERROR: Unable to open given codebook file\n");
        //exit the code
        exit(0);
      }
      //create a hashtable which stores the values of the huffman codebook
      if(huffmanCodebookReader(huffFD)){
        //print an error
        printf("ERROR: Unable to read given codebook file\n");
        //exit the code
        exit(0);
      }
      //compress the files in our files array
      if(compressFiles(fileCounter)){
        //print an error
        printf("ERROR: Unable to compress given file(s)\n");
        //exit the code
        exit(0);
      }
      //close the huffman codebook file descriptor
      close(huffFD);
      //free the huffman table
      freeHuffmanTable();
    }
  }
  //release the files!
  freeFiles(fileCounter);
  clock_t toc = clock();
  //printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
  //we're done!
  return 0;
}

/*Frees the Huffman Table*/
int freeHuffmanTable(){
  //traversal nodes 
  struct huffNode * head;
  struct huffNode * temp;
  //looping index
  int i;
  //loop through the whole table
  for(i = 0; i < 256; i++){
    //assign the node at i to head
    head = huffmanTable[i];
    //loop through the table
    while(head != NULL){
      //assign temp to be the next
      temp = head -> next;
      //free the contents
      free(head->token);
      free(head->bittok);
      //free the head
      free(head);
      //reassign head to temp
      head = temp;
    }
  }
  //return success
  return 0;
}

/* this method traverses the files array and compresses all of them returns 0 on success*/
int compressFiles(int numOfFiles){
  //loop counter
  int i;
  //stores the file extension of the current file (used to skip .hcz files)
  char fileType[5];
  //stores the file path of the .hcz file
  char newFilePath[PATH_MAX+1];
  //stores the length of the current file
  int length;
  //file descriptor opens the current file
  int fd;
  //file descriptor opens the current .hcz file
  int writefd;
  //loop through the files and skip the ones not compressed already
  for(i = 0; i < numOfFiles; i++){
    //calculate the index of the point
    length = strlen(files[i]);
    //first check if the string is less than 5 characters long
    if(length > 4){
      //extract the file extension from the path
      strncpy(fileType, &files[i][length-4], 4);
      //null terminate the filetype
      fileType[4] = '\0';
      //check if the file is an hcz file and skip if it is
      if(strcmp(fileType, ".hcz") == 0){
        //if this is the only file, we want to return an error
        if(numOfFiles == 1){
          //returning 1 will triger a print in the main method
          return 1;
        }
        //skip this file
        continue;
      }
    }
    //open the uncompressed file
    fd = open(files[i], O_RDONLY);
    //check to see if there was trouble opening the file
    if(fd < 0){
      //print error
      printf("ERROR: Unable to open the %dth uncompressed file\n", i);
      //exit in style
      exit(0);
    }
    //copy the files[i] into the newfilepath
    strcpy(newFilePath, files[i]);
    //concatenate the .hcz onto the new file path
    strcat(newFilePath, ".hcz\0");
    //also open the file descriptor to write
    writefd = open(newFilePath, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    //check to see if there was trouble opening the file
    if(writefd < 0){
      //print error
      printf("ERROR: Unable to open the .hcz for the %dth uncompressed file\n", i);
      //exit in style
      exit(0);
    }
    //read the file
    if(compressionFileReader(fd, writefd)){
      //error while trying to compress a file
      printf("ERROR: Unable to compress file %d\n", i);
    }
    //close both file descriptors once we are done using them
    close(fd);
    close(writefd);
  }
  //we were successfull
  return 0;
}

/* Reads the given file and loads it into a local buffer calls sav tokenizer afterwards*/
int compressionFileReader (int fileDescriptor, int writeFD){
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
      if(myBuffer == NULL){
        //print an error
        printf("ERROR: Not enough space on heap\n");
        //exit
        return 1;
      }
      //set everything in the buffer to the null terminator
      memset(myBuffer, '\0', buffSize);
      //copy the old memory into the new buffer
      memcpy(myBuffer, temp, readIn);
      //free the old memory that was allocated temporarily
      free(temp);
    }
  }
  //we hand over the contents of our file (stored in buffer) to the table loader function
  if(savTokenizer(myBuffer, buffSize, writeFD)){
    //print the error
    printf("ERROR: While trying to tokenize the file\n");
    //there was an error in the tokenizer
    return 1;
  }
  //finish it
  return 0;
}

/* After reading the complete contents of a given file(in this case the text file we want compressed), we need to split the file up into tokens
and then load each token into the linked list.  This is also a modified version of Adviths tokenizer */
int savTokenizer (char *buff, int buffSize, int writeFD){
  //counter to loop through the buffer
  int counter = 0;
  //counter to loop through the cdata
  int localcount = 0;
  //store the current character
  char ctemp;
  //allocate starting space into cdata
  char *cdata = (char *)malloc(11*sizeof(char));
  //temp when you need to expand the string tokens
  char *temp;
  //check if the pointer is null
  if(cdata == NULL){
    //print an error
    printf("ERROR: Not enough space on heap\n");
    //return unsuccessfull
    return 1;
  }
  //set the array to be all null terminators
  memset(cdata, '\0', 11);
  //current size of cdata
  int currSize = 10;
  //loop through the string until we hit a comma or terminator
  while(buff[counter] != '\0'){
    //get the current character at every iteration
    ctemp = buff[counter];
    //check if we have hit a control character
    if(ctemp == '\t' || ctemp == ' ' || ctemp == '\n'){
      //we don't need to store empty tokens
      if(strcmp(cdata, "") != 0){
        //isolate the token and insert it into the table
        compressionWriter(cdata, writeFD);
        //freecdata we're done with using it
        free(cdata);
        //allocate a new cdata for the next token
        cdata = (char *)malloc(11*sizeof(char));
        //set the array to be all null terminators
        memset(cdata, '\0', 11);
        //reset the curr size
        currSize = 10;
      }
      //reset the local count
      localcount = -1;
      //also insert the special terminator character
      switch(ctemp){
        case '\t':
          compressionWriter("\t", writeFD);
          break;
        case ' ':
          compressionWriter(" ", writeFD);
          break;
        case '\n':
          compressionWriter("\n", writeFD);
          break;
      }
    } else if(localcount >= currSize){ //store the character in the node array but first check for overflow
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
        //return an error
        return 1;
      }
      //set the random stuff to null terminators again
      memset(cdata, '\0', currSize);
      //copy the old memory into the new buffer
      memcpy(cdata, temp, localcount);
      //free the old variable
      free(temp);
      //store the buffer char
      cdata[localcount] = buff[counter];
    } else{
      //store the buffer character into the cdata character
      cdata[localcount] = buff[counter];
    }
    //increment counters
    counter++;
    localcount++;
  }
  //we have reached the end put the last guy in there
  if(buff[counter] == '\0'){
    //isolate the token and insert it into the table
    compressionWriter(cdata, writeFD);
  }
  //free the original character buffer before leaving
  free(buff);
  //return success!
  return 0;
}

/*given the token and the file descriptor, this method writes the token's bitcode to the file descriptor*/
int compressionWriter(char * token, int fd){
  //store the current node
  struct huffNode * curr;
  //calculate the hash code
  int sum = 0;
  //index
  int k;
  //loop through the characters of the token
  for (k = 0; k < strlen(token); k++){
    //calculate the ascii sum of the token
    sum = sum + (int)token[k];
  }
  //mod it to fit inside the 256 length array
  int index = sum % 256;
  //error check the index
  if(index < 0){
    //make the index positive
    index *= -1;
  }
  //check to see if the head node of the given index is Null
  if(huffmanTable[index] == NULL){
    //there is an error
    printf("ERROR: Codebook mismatch, your codebook might be empty\n");
    //exit this badboy
    exit(0);
  } else{
    //gather the current node
    curr = huffmanTable[index];
    //we need to traverse and find the bitcode
    while(curr != NULL){
      //check if the current node is our desired token
      if(strcmp(curr->token, token) == 0){
        //we need to write our bitcode
        write(fd, curr->bittok, strlen(curr->bittok));
        //break the loop once we are done
        break;
      }
      //make sure to traverse through the whole damn list
      curr = curr -> next;
    }
  }
  //success!
  return 0;
}

/* Read the given file descriptor and write each item to the huffman table */
int huffmanCodebookReader (int fileDescriptor){
  //buffer size
  int buffSize = 100;
  //create the buffer to store the file
  char *myBuffer = (char *)malloc(101*sizeof(char));
  //check if the pointer is null
  if(!myBuffer){
    //print an error
    printf("ERROR: Not enough space on heap\n");
    //we were unsuccessful
    return 1;
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
      if(myBuffer == NULL){
        //print an error
        printf("ERROR: Not enough space on heap\n");
        //we were unnseccufl
        return 1;
      }
      //copy the old memory into the new buffer
      memcpy(myBuffer, temp, readIn);
      //free the old memory that was allocated
      free(temp);
    }
  }
  //we need to split the buffer into tokens
  if(huffTokenizer(myBuffer, readIn)){
    //there was an error
    printf("ERROR: Unable to tokenize given codebook\n");
    //return unsuccessful
    return 1;
  }
  //free the buffer once it has been broken into tokens
  free(myBuffer);
  //success!
  return 0;
}

/*this method will be for going through the huffman codebook*/
int huffTokenizer (char *buff, int buffSize){
  //check to see that we have shit in the buffer
  if(buffSize == 0){
    //print an error
    printf("WARNING: Empty codebook\n");
    //return unsuccessfull
    return 1;
  }
  //counter to loop through the buffer
  int counter = 0;
  //counter to loop through the cdata
  int localcount = 0;
  //find the escaping sequence first allocate space for it
  escapeSequence = (char *) malloc(50*sizeof(char));
  //throw an error in case this fails
  if(!escapeSequence){
    //print an ERROR
    printf("FATAL ERROR: Not enough space on the heap/n");
    //return unsuccessful
    return 1;
  }
  //memset the escape sequence to be null
  memset(escapeSequence, '\0', 50);
  //loop through the characters until you hit the first new line
  while(buff[counter] != '\n'){
    //store the characters in the escaping sequence one by one
    escapeSequence[counter] = buff[counter];
    //increment the counter
    counter++;
  }
  //this value stores the character at counter
  char ctemp;
  //this value indicates wether we are reading a token or bit sequence
  int indicator = 0;
  //allocate starting space into cdata
  char *cdata = (char *)malloc(11*sizeof(char));
  char *bdata = (char *)malloc(11*sizeof(char));
  //used temporarily for reallocing memory
  char *temp;
  //check if the pointer is null
  if(!cdata || !bdata){
    //print an error
    printf("FATAL ERROR: Not enough space on heap\n");
    //return unsuccessful
    return 1;
  }
  //set the array to be all null terminators
  memset(cdata, '\0', 11);
  memset(bdata, '\0', 11);
  //initial sizes
  int cSize = 10;
  int bSize = 10;
  //skip the fnewline char at the end of the first line
  counter ++;
  //loop through the string until we hit a terminator which means end of the buffer and text file
  while(buff[counter] != '\0'){
    //store the current value of the buffer at counter
    ctemp = buff[counter];
    //check if we have hit a newline
    if(ctemp == '\n') { 
    	// dont need to store empty tokens TODO: add this to the decompress method
    	if(strcmp(cdata, "") != 0) {
        //null terminate our bdata
        cdata[localcount] = '\0';
        //insert our bitcode and token into the hashtable
    		if(hInsert(cdata, bdata)){
          //print an error
          printf("ERROR: Unable to insert %s into our huffman table", cdata);
          //return unnsuccessful
          return 1;
        }
    		// allocate new space
    		cdata = (char *)malloc(11*sizeof(char));
    		bdata = (char *)malloc(11*sizeof(char)); 
        //set the characters to null terminators
    		memset(cdata, '\0', 11);
    		memset(bdata, '\0', 11);
        //reset the sizes with one space for the null terminator
        bSize = 10;
        cSize = 10;
        //flip indicator back
        indicator = 0;
    	}
      //reset the local count
    	localcount = -1;
    } else if(ctemp == '\t'){ //check if we have hit a tab
      //next we need the cdata
      indicator = 1;
      //we don't need to store empty tokens
     	if(strcmp(bdata, "") == 0){
        //we failed to read the bdata
        printf("WARNING: Codebook formatted incorrectly, compression will not work with only one element in the codebook\n");
        //return unsuccessful
        return 1;
      }
      //reset the local count
      localcount = -1;
      //dont need Adviths switch statements, huffman codebook formatted properly
    } else if(indicator) { // we are dealing with the token
      //check to see if there is still space in the character array
    	if(localcount >= cSize){
    		//temporarily relocate the existing characters
    		temp = cdata;
        //allocate more space for the token
    		cdata = (char*)malloc((cSize+10)*sizeof(char));
        //checks make sure there is space
    		if(cdata == NULL) {
          //there's no space
    			printf("ERROR: Not enough space on heap\n");
          //return unsuccessfull
          return 1;
    		}
        //increase the space used by our token
        cSize += 10;
        //set the token to null terminators
    		memset(cdata, '\0', cSize);
        //copy the temp stuff back into our cdata
    		memcpy(cdata, temp, localcount);
        //TODO: remember to free temp in the decompress tokenizer
    		free(temp);
    	}
      //check the escaping sequence
      if(ctemp == escapeSequence[localcount]){
        //check to see if the rest of the escaping sequence is also present
        if(strncmp(escapeSequence, &buff[counter], strlen(escapeSequence)) == 0){
          //we need to escape sequence with one of the following
          if(buff[counter+strlen(escapeSequence)] == 'n'){
          //store the newline character in cdata
          cdata[0] = '\n';
          //skip the rest of the escape sequence
          counter += strlen(escapeSequence);
        } else if(buff[counter+strlen(escapeSequence)] == '\n'){
          //store the space character in cdata
          cdata[0] = ' ';
          //skip the rest of the escape sequence
          counter += strlen(escapeSequence)-1;
        } else if(buff[counter+strlen(escapeSequence)] == 't'){
          //store the tab character in cdata
          cdata[0] = '\t';
          //skip the rest of the escape sequence
          counter += strlen(escapeSequence);
        }
        }
      } else{
        //continue filling up cdata
        cdata[localcount] = buff[counter];
      }
    } else if(!indicator){ // we are dealing with the bitcode
    	if(localcount >= bSize) {
    		//temporarily relocate the existing numbers
    		temp = bdata;
        //allocate more space for the bitcode
    		bdata = (char*)malloc((bSize+10)*sizeof(char));
        //checks make sure there is space
    		if(bdata == NULL) {
          //there's no space
    			printf("ERROR: Not enough space on heap\n");
          //unsuccessful
          return 1;
    		}
        //increase the space used by our token
        bSize += 10;
        //set the token to null terminators
    		memset(bdata, '\0', bSize);
        //copy the temp stuff back into our cdata
    		memcpy(bdata, temp, localcount);
        //TODO: remember to free temp in the decompress tokenizer
    		free(temp);
    	}
      //continue reading the bitcode
      bdata[localcount] = buff[counter];
	  }
  //increment counters
  counter++;
  localcount++;
  }
  return 0;
}

/* this method will take the token and bitcode of the token and make a node for it and insert it into the hashtable */
int hInsert (char * name, char * bitname) {
  //store the hashcode
	int sum = 0;
  //loop counter declaration
  int i;
  //store the newnode
  struct huffNode * newNode;
  //for LL traversal
  struct huffNode * temp;
  //calculate the hashcode
	for (i = 0; i < strlen(name); i++){
    sum += (int)name[i];
	}
  //calculate the index by modding the hashcode
	int index = sum % 256;
  //check if the index is negative and make it positive
  if(index < 0){
    index *= -1;
  }
  //check if the hashtable at said index is empty
	if(huffmanTable[index] == NULL) {
    //TODO: edit my initial hashtable method to reflect the changes create a new node and put er in there
    newNode = (struct huffNode *)malloc(sizeof(struct huffNode));
    //memory error
    if(!newNode){
      //no space
      printf("FATAL ERROR: Not enough space on the heap");
      //return unsuccessful
      return 1;
    }
    //populate the node's values
    newNode -> token = name;
    newNode -> bittok = bitname;
    newNode -> next = NULL;
    //now assign the head of the particular index to the huffman table
    huffmanTable[index] = newNode;
    //we finished the insertion
    return 0;
	} else {
    //we need to traverse a linkedlist TODO: Also edit my initial hashtable method
		temp = huffmanTable[index];
    //make newNode the new head of the linked list at the index
		newNode = (struct huffNode *)malloc(sizeof(struct huffNode));
    //memory error
    if(!newNode){
      //no space
      printf("FATAL ERROR: Not enough space on the heap");
      //return unsuccessful
      return 1;
    }
    //populate
    newNode -> token = name;
    newNode -> bittok = bitname;
		newNode->next = temp;
    //make newnode the new head
    huffmanTable[index] = newNode;
	}
  //success!
  return 0;
}

/* Reads the given file and loads it into a local buffer */
int codebookReader (int fileDescriptor){
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
      if(myBuffer == NULL){
        //print an error
        printf("ERROR: Not enough space on heap\n");
      }
      //set everything in the buffer to the null terminator
      memset(myBuffer, '\0', buffSize);
      //copy the old memory into the new buffer
      memcpy(myBuffer, temp, readIn);
      //free the old memory that was allocated
      free(temp);
    }
  }
  //we hand over the contents of our file (stored in buffer) to the table loader function
  buildHuffTree(myBuffer, buffSize);
  //free the buffer once we're done using it
  free(myBuffer);
  //finish it
  return 0;
}

/* traverse through the given huffman codebook */
void buildHuffTree(char * myBook, int buffSize){
  //check to see that we have shit in the buffer
  if(buffSize == 0){
    //print an error
    printf("WARNING: Empty codebook\n");
    //return unsuccessfull
    exit(0);
  }
  //counters
  int t = 0;
  int indicator = 0;
  int localCount = 0;
  //find the escaping sequence first allocate space for it
  escapeSequence = (char *) malloc(50*sizeof(char));
  //throw an error in case this fails
  if(!escapeSequence){
    //print an ERROR
    printf("FATAL ERROR: Not enough space on the heap/n");
    //return unsuccessful
    exit(0);
  }
  //memset the escape sequence to be null
  memset(escapeSequence, '\0', 50);
  //loop through the characters until you hit the first new line
  while(myBook[t] != '\n'){
    //store the characters in the escaping sequence one by one
    escapeSequence[t] = myBook[t];
    //increment the counter
    t++;
  }
  //store the path and token
  char currPath[PATH_MAX+1];
  //reset the path
  memset(currPath, '\0', PATH_MAX+1);
  //malloc the token
  char * token = (char *)malloc(101*sizeof(char));
  //temp for increasing the character size
  char * temp;
  //set tokens to null
  memset(token, '\0', 101);
  //initial token size
  int tokenSize = 100;
  //skip the fnewline char at the end of the first line
  t ++;
  //loop counter
  int i;
  //now traverse the codebook
  for(i = t; i < buffSize; i++){
    //if we hit a tab, switch the indicator to 1
    if(myBook[i] == '\t'){
      //start getting the token
      indicator = 1;
      //reset localCount and add the null terminator
      currPath[localCount] = '\0';
      localCount = 0;
    } else if(myBook[i] == '\n'){
      //we don't want to insert empty tokens
      if(strcmp(token, "") != 0){
        //add a null terminator just in case
        token[localCount] = '\0';
        //insert the token into the appropriate spot of the huffman tree
        huffInsert(currPath, token, huffHead);
      }
      //flip the indicator back
      indicator = 0;
      //make a new token
      token = malloc(101*sizeof(char));
      //reset token size as well
      tokenSize = 100;
      //reset localCount and add the null terminator
      localCount = 0;
      //reset the path as well
      memset(currPath, '\0', PATH_MAX+1);
    } else if(!indicator){
      //calculate the path
      currPath[localCount] = myBook[i];
      localCount++;
    } else if(indicator){
      //check to see if localcount went overboard
      if(localCount >= tokenSize){
        //temporily move token to temp
        temp = token;
        //malloc more memory to token
        token = (char *)malloc((tokenSize+101)*sizeof(char));
        //increase token size
        tokenSize += 101;
        //set token to \0
        memset(token, '\0', tokenSize);
        //move the temp stuff back into token
        memcpy(token, temp, tokenSize-100);
      }
      //check the escaping sequence
      if(myBook[i] == escapeSequence[localCount]){
        //check to see if the rest of the escaping sequence is also present
        if(strncmp(escapeSequence, &myBook[localCount], strlen(escapeSequence)) == 0){
          //we need to escape sequence with one of the following
          if(myBook[i+strlen(escapeSequence)] == 'n'){
          //store the newline character in cdata
          token[0] = '\n';
          //skip the rest of the escape sequence
          i += strlen(escapeSequence);
        } else if(myBook[i+strlen(escapeSequence)] == '\n'){
          //store the space character in cdata
          token[0] = ' ';
          //skip the rest of the escape sequence
          i += strlen(escapeSequence)-1;
        } else if(myBook[i+strlen(escapeSequence)] == 't'){
          //store the tab character in cdata
          token[0] = '\t';
          //skip the rest of the escape sequence
          i += strlen(escapeSequence);
        }
        }
      } else{
        //find the token
        token[localCount] = myBook[i];
      }
      //in any case, increment the local count
      localCount++;
    }
  }
}

/*Insert the node in the correct part of our huffman tree*/
int huffInsert(char * currPath, char * token, struct node * curr){
  //follow the instructions given in the path
  if(strcmp(currPath, "") == 0){
    //we have found the node ladies and gents
    curr -> rChild = NULL;
    curr -> lChild = NULL;
    curr ->identifier = 1;
    curr ->myKey = token;
  } else{
    //check to go right or left
    if(currPath[0] == '0'){
      //go left, check if it's null
      if(!(curr -> lChild)){
        //allocate memory
        curr -> lChild = (struct node *)malloc(sizeof(struct node));
        //make sure the that its children are nulled out
        curr -> lChild -> rChild = NULL;
        curr -> lChild -> lChild = NULL;
      }
      //change path accordingly
      currPath++;
      //make sure the identifier signifies a parent
      curr ->identifier = 0;
      //recursively call huffInsert
      huffInsert(currPath, token, curr -> lChild);
    } else if(currPath[0] == '1'){
      //we need to go right
      if(!(curr -> rChild)){
        //allocate memory
        curr -> rChild = (struct node *)malloc(sizeof(struct node));
        //make sure the that its children are nulled out
        curr -> rChild -> rChild = NULL;
        curr -> rChild -> lChild = NULL;
      }
      //change path accordingly
      currPath++;
      //make sure the identifier signifies a parent
      curr ->identifier = 0;
      //recursively call huffInsert
      huffInsert(currPath, token, curr -> rChild);
    }
  }
  return 0;
}

/* TODO this method traversees the files array and decompresses all of them*/
int decompressFiles(int numOfFiles){
  //counter and other stuff
  int i;
  char filePath[PATH_MAX+1];
  char fileType[4];
  int length;
  int fd;
  int writefd;
  //loop through the files and skip the ones not compressed already
  for(i = 0; i < numOfFiles; i++){
    //calculate the index of the point
    length = strlen(files[i]);
    //check to see if the path is even valid
    if(length < 5){
      continue;
    }
    //split the path and type
    strncpy(fileType, &files[i][length-4], 4);
    strncpy(filePath, files[i], length-4);
    //we need to add a null terminator to the filepath
    filePath[length-4] = '\0';
    //check if the file is an hcz file
    if(strcmp(fileType, ".hcz") == 0){
      //open the huffman codebook file
      fd = open(files[i], O_RDONLY);
      //also open the file descriptor to write
      writefd = open(filePath, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
      //pass it into the readHcz method to be read
      readHcz(fd, writefd);
    }
  }
  //success!
  return 0;
}

/*Reads the given .hcz file*/
int readHcz(int readFD, int writeFD){
  //store the character read from the hcz file
  char * direction = (char *)malloc(2*sizeof(char));
  //store the current node
  struct node * curr = huffHead;
  //status indicates the number of bits read
  int status = 1;
  //loop through the bitcode in the hcz file
  while(status != 0){
    //read in a bit and pick the direction
    status = read(readFD, direction, 1);
    //check if status is 1
    if(status == 1){
      //go down the tree accordingly
      if(direction[0] == '1'){
        //we need to go right
        if(!(curr -> rChild)){
          //there's something wrong
          printf("Warning: Codebook mismatch\n");
          exit(1);
        }
        curr = curr -> rChild;
      } else if(direction[0] == '0'){
        //we need to go left
        if(!(curr -> lChild)){
          //there's something wrong
          printf("Warning: Codebook mismatch\n");
          exit(1);
        }
        curr = curr -> lChild;
      }
      //check to see if you hit a leaf
      if(curr ->identifier == 1){
        //we need to write the token to the write fd
        write(writeFD, curr -> myKey, strlen(curr -> myKey));
        //reset curr to be the huffman head
        curr = huffHead;
      }
    }
  }
  return status;
}

/* writes the codes to a codebook file and frees the tree*/
int depthFirstSearch(struct node * curr, int fd, char * path){
  //create a local copy of the path
  char localPath[PATH_MAX];
  strcpy(localPath, path);
  //for some reason if curr is null, gtfo
  if(!curr){
    return 0;
  }
  //check to see if the node has children
  if(curr->identifier == 1){
    //if not, we have hit a leaf node write its code
    write(fd, path, strlen(path));
    write(fd, "\t", 1);
    write(fd, curr -> myKey, strlen(curr -> myKey));
    write(fd, "\n", 1);
    //free the node!
    free(curr);
    //return success
    return 0;
  } else{
    //otherwise we go down the path
    if(curr->rChild){
      //copy the path
      strcpy(localPath, path);
      //add a 1 to the path
      strncat(localPath, "1", 1);
      //go down the right child
      depthFirstSearch(curr->rChild, fd, localPath);
    }
    //go down the leftside
    if(curr->lChild){
      //reset the path
      strcpy(localPath, path);
      //add a 0 to the path
      strncat(localPath, "0", 1);
      //go down the left child
      depthFirstSearch(curr->lChild, fd, localPath);
    }
  }
  //free the node!
  free(curr);
  //return success
  return 0;
}

/*Start building the Subtrees Tree and free the heap upon completion*/
int buildSubTrees(){
  //have a temp node to store the pops
  struct node * temp1 = NULL;
  struct node * temp2 = NULL;
  struct node * parent = NULL;
  //loop through the original minheap
  while(myHeap -> used > 1){
    //check the roots of both heaps
    temp1 = pop();
    temp2 = pop();
    //make the next nodes null
    temp1 -> next = NULL;
    temp2 -> next = NULL;
    //allocate space for a parent node
    parent = (struct node *)malloc(sizeof(struct node));
    //initialize the values in the parent
    parent -> identifier = 0;
    parent -> myKey = NULL;
    parent -> next = NULL;
    parent -> frequency = temp1 -> frequency + temp2 -> frequency;
    //assign the right and left children of the parent
    parent -> lChild = temp1;
    parent -> rChild = temp2;
    //put the parent back into the heap
    heapInsert(parent);
  }
  //assign huffhead to hold the head of the tree
  huffHead = myHeap -> arr[0];
  //get rid of the heap
  free(myHeap -> arr);
  free(myHeap);
  //return success
  return 0;
}

/* pop's the minimum (root) off of the heap */
struct node * pop(){
  //find the node at the top
  struct node * toPop = myHeap -> arr[0];
  //replace the root with the lowest node
  myHeap -> arr[0] = myHeap -> arr[myHeap -> used-1];
  //make the last node null now that you've moved it
  myHeap -> arr[myHeap -> used-1] = NULL;
  //update the used space in the heap
  myHeap -> used--;
  //insert the root in the correct position
  siftDown(myHeap -> arr[0], 0);
  //return the node that was popped
  return toPop;
}

/*TODO: fix for big files sift down the new root until it is at the correct position*/
int siftDown(struct node * parent, int currPos){
  //calculate the potential positions
  int rPos = (currPos*2)+2;
  int lPos = (currPos*2)+1;
  //check if we are at the end of the heap
  while(rPos > myHeap -> cap || lPos > myHeap -> cap){
    //increase the heap size, this is unlikely tho
    increaseHeapSize();
  }
  //we need to sift up our latest inserted node until its in the right spot
  struct node * rightChild = myHeap -> arr[rPos];
  struct node * leftChild = myHeap -> arr[lPos];
  struct node * temp;
  //check if the parent is indeed less than the child
  if(rightChild && rightChild -> frequency < parent -> frequency){
    //something has to be sifted, figure out what
    if(rightChild -> frequency <= leftChild -> frequency){
      //store the parent in temp
      temp = parent;
      //reassign the parent
      myHeap -> arr[currPos] = rightChild;
      //reassign the child
      myHeap -> arr[(currPos*2)+2] = temp;
      //call siftup again to now check the parent
      siftDown(temp, (currPos*2)+2);
    } else if (leftChild && leftChild -> frequency < parent -> frequency){
      //store the parent in temp
      temp = parent;
      //reassign the parent
      myHeap -> arr[currPos] = leftChild;
      //reassign the child
      myHeap -> arr[(currPos*2)+1] = temp;
      //call siftup again to now check the parent
      siftDown(temp, (currPos*2)+1);
    }
  } else if (leftChild && leftChild -> frequency < parent -> frequency){
      //store the parent in temp
      temp = parent;
      //reassign the parent
      myHeap -> arr[currPos] = leftChild;
      //reassign the child
      myHeap -> arr[(currPos*2)+1] = temp;
      //call siftup again to now check the parent
      siftDown(temp, (currPos*2)+1);
    }
  return currPos;
}

/*transfers the contents of the hashTable into the heap*/
int heapTransfer(){
  //declare counters
  int i;
  //store the current struct in the hashtable
  struct node *temp;
  //loop through the hash table
  for(i = 0; i < 256; i++){
    //check that the start at index i is not null
    if(hashTable[i]){
      //assign temp to the start of the table index
      temp = hashTable[i];
      //loop through the nodes at each index
      while(temp != NULL){
        //insert into the heap
        heapInsert(temp);
        //traverse through the linked list
        temp = temp -> next;
      }
    }
  }
  //return success!
  return 0;
}

/*increases the heap array size by 100*/
int increaseHeapSize(){
  //first store the old array in a temp
  struct node ** temp = myHeap -> arr;
  //then mallocate a new array
  myHeap -> arr = malloc(((myHeap -> cap)+100)*sizeof(struct node *));
  //set the memory to all zeros
  memset(myHeap -> arr, 0x0, (myHeap -> cap+100)*sizeof(struct node *));
  //copy the temp back into the main memory
  memcpy(myHeap -> arr, temp, (myHeap -> cap)*sizeof(struct node *));
  //increase the capacity by 100
  myHeap -> cap += 100;
  //return the new capacity
  return myHeap -> cap;
}

int siftUp(struct node * child, int currPos){
  //calculate the potential position of the parent
  int parentPos = (currPos-1)/2;
  //check if the parent position is less than 0 or if both curr and parent are zero
  if(parentPos == 0 && currPos == 0){
    //we have gone as high as we could have
    return 0;
  }
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
  //return the new position
  return (currPos-1)/2;
}

/*print heap function (purely for testing)*/
void printHeap(){
  //counter
  int k = 0;
  int myfd = open("tableContents.txt", O_WRONLY);
  char snum[15];
  //loop through the heap used
  for(k = 0; k < myHeap -> used; k++){
    sprintf(snum,"%d" ,myHeap -> arr[k] -> frequency);
    write(myfd, myHeap -> arr[k] -> myKey, strlen(myHeap -> arr[k] -> myKey));
    write(myfd, "\t", 1);
    write(myfd, snum, strlen(snum));
    write(myfd, "\n", 1);
    //printf("%s %d %d \n",myHeap -> arr[k] -> myKey, myHeap -> arr[k] -> identifier, myHeap -> arr[k] -> frequency);
  } 
  printf("\n");
  close(myfd);
}

int heapInsert(struct node * toInsert){
  //check if there is space to insert the node
  if(myHeap -> used >= myHeap -> cap){
    //increase the heap size
    increaseHeapSize();
  }
  //insert the node at the end of the array and sift up accordingly
  myHeap -> arr[myHeap -> used] = toInsert;
  //sift up the last node accordingly
  siftUp(toInsert, myHeap -> used);
  //increment the used space
  myHeap -> used++;
  //printHeap();
  //increment the used space
  return myHeap -> used;
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
    //skip the . and .. and dsstore file
    if(strcmp(currDir->d_name, ".") == 0 || strcmp(currDir->d_name, "..") == 0 || strcmp(currDir->d_name,".DS_Store") == 0){
      //skip the iteration
      continue;
    }
    //first check if the currdir is a regular file or a directory
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
    } else if(currDir -> d_type == DT_REG){
      //allocate space for the file path
      files[counter] = (char *)malloc((PATH_MAX+1) * sizeof(char));
      //add the file path to the array
      strcpy(files[counter],filePBuff);
      //store the names of the files in our files array
      strcat(files[counter], currDir->d_name);
      //just to test the code
      //printf("%s\n", files[counter]);
      //check if files array needs more space
      if(++counter >= currSize){
        //realloc 100 more spaces in our files array
        files = realloc(files, (currSize+100) * sizeof(char *));
        //check is the given file exists
        if(!files){
          //file does not exist it is a fatal error
          printf("FATAL ERROR: Not enough memory\n");
          //exit the code
          exit(1);
        }
        //change the curr size accordingly
        currSize += 100;
      }
    }
  }
  //return the current count
  return counter;
}

/* free the files array */
void freeFiles(int numberOfFiles){
  //counter
  int i;
  //no need to loop is there is only one file
  if(numberOfFiles > 1){
    //loop through the files array and free em all
    for(i = 0; i < numberOfFiles; i++){
      //free file at i
      free(files[i]);
    }
  }
  //free the files array itself
  free(files);
}


/* iterates through the files and first calls file reader on each file */
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
    //close the file descriptor once you are done with it
    close(fd);
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
      if(myBuffer == NULL){
        //print an error
        printf("ERROR: Not enough space on heap\n");
      }
      //set everything in the buffer to the null terminator
      memset(myBuffer, '\0', buffSize);
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
    if(ctemp == '\t' || ctemp == ' ' || ctemp == '\n'){
      //we don't need to store empty tokens
      if(strcmp(cdata, "") != 0){
        //isolate the token and insert it into the table
        tableInsert(cdata);
        //allocate a new cdata for the next token
        cdata = (char *)malloc(11*sizeof(char));
        //set the array to be all null terminators
        memset(cdata, '\0', 11);
        //reset the curr size
        currSize = 10;
      }
      //reset the local count
      localcount = -1;
      //also insert the special terminator character
      switch(ctemp){
        case '\t':
          tableInsert("$t");
          break;
        case ' ':
          tableInsert("$");
          break;
        case '\n':
          tableInsert("$n");
          break;
      }
    }
    //store the character in the node array
    else if(localcount >= currSize-1){
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
    } else{
      //store the buffer character into the cdata character
      cdata[localcount] = buff[counter];
    }
    //increment counters
    counter++;
    localcount++;
  }
  //we have reached the end put the last guy in there
  if(buff[counter] == '\0'){
    //check to see if cdata is empty we don't want to unput empty strings
    if(strcmp(cdata, "") != 0){
      //isolate the token and insert it into the table
      tableInsert(cdata);
    }
  }
  return 0;
}

/* for every token find its hash code and insert it into the table accordingly */
int tableInsert(char *token){
  //calculate the hash code
  int sum = 0;
  int k;
  //loop through the characters of the token
  for (k = 0; k < strlen(token); k++){
    //calculate the ascii sum of the token
    sum = sum + (int)token[k];
  }
  //mod it to fit inside the 256 length array
  int index = sum % 256;
  //error check the index
  if(index < 0){
    //make the index positive
    index *= -1;
  }
  //check to see if the head node of the given index is Null
  if(hashTable[index] == NULL){
    //create a new node and put er in there
    struct node * newNode = (struct node *)malloc(sizeof(struct node));
    //populate the node's values
    newNode -> identifier = 1;
    newNode -> myKey = token;
    newNode -> frequency = 1;
    newNode -> next = NULL;
    newNode -> rChild = NULL;
    newNode -> lChild = NULL;
    //store the newly created node in the table
    hashTable[index] = newNode;
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
          curr -> identifier = 1;
          curr -> myKey = token;
          curr -> frequency = 1;
          curr -> next = NULL;
          curr -> rChild = NULL;
          curr -> lChild = NULL;
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

/*print the table only to check the frequencies of everything*/
int printTable(){
  //declare counters
  int i;
  struct node *temp;
  int myfd = open("tableContents.txt", O_WRONLY);
  char snum[15];
  //loop through the hash table
  for(i = 0; i < 256; i++){
    //check that the start at index i is not null
    if(hashTable[i]){
      //assign temp to the start of the table index
      temp = hashTable[i];
      //loop through the nodes at each index
      while(temp != NULL){
        sprintf(snum,"%d" ,temp -> frequency);
        write(myfd, temp -> myKey, strlen(temp -> myKey));
        write(myfd, "\t", 1);
        write(myfd, snum, strlen(snum));
        write(myfd, "\n", 1);
        //traverse
        temp = temp -> next;
      }
    }
  }
  close(myfd);
  return 0;
}