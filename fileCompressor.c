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

//global variable which stores the names of the files from which we build the huffman codebook
char ** files;
struct node * hashTable[256];
struct heap * myHeap;
struct node * huffHead;

/* The brains of the operation */
int main (int argc, char ** argv){

  //booleans indicating flags
  int build = 0;
  int recursive = 0;
  int compress = 0;
  int decompress = 0;
  //counter for the loop
  int i = 0;
  //loop through the arguments
  for(i = 0; i < argc; i++){
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
          printf("FATAL ERROR: File does not exist\n");
          //exit the code
          exit(1);
        }
        //pass along the file directory to the traversal method which stores the files
        int fileCounter = direcTraverse(myDirec, 0, 100, argv[i]);
        //we need to build the huffman codebook
        buildCodebook(fileCounter);
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
          exit(1);
        }
        //the only file
        files[0] = argv[i];
        //we need to build the huffman codebook
        buildCodebook(1);
      }
      //TODO: delete this in final iteration of project
      //printTable();
      //after building the hashtable, we need to store everything in a heap
      myHeap = (struct heap *)malloc(sizeof(struct heap));
      //start with an initial capacity of 100
      myHeap -> arr = (struct node **)malloc(101*sizeof(struct node *));
      myHeap -> cap = 100;
      memset(myHeap -> arr, 0x0, (myHeap -> cap)*sizeof(struct node *));
      myHeap -> used = 0;
      //we need to transfer the hashtable into the heap
      heapTransfer();
      //TODO: remove this before submission
      printHeap();
      //now we need to start building the Huffman Tree
      buildSubTrees();
      //TODO: remove this before submission
      //printHeap();
      //once the subtrees are built, we need ot write to a huffman codebook, FIRST CREATE a codebook file
      int codFD = open("HuffmanCodebook", O_RDWR | O_CREAT);
      //write the escape character being used
      write(codFD, "$ \n", 3);
      //call the DFS to calculate the huffman codes
      depthFirstSearch(huffHead, codFD, "");
      //close the file descriptor once we're done writing
      close(codFD);
    }
  }
  //we're done!
  return 0;
}

/* writes the codes to a codebook file */
int depthFirstSearch(struct node * curr, int fd, char * path){
  //create a local copy of the path
  char localPath[PATH_MAX];
  strcpy(localPath, path);
  //for some reason if curr is null, gtfo
  if(curr == NULL){
    return 0;
  }
  //check to see if the node has children
  if(curr->identifier == 1){
    //if not, we have hit a leaf node
    write(fd, path, strlen(path));
    write(fd, "\t", 1);
    write(fd, curr -> myKey, strlen(curr -> myKey));
    write(fd, "\n", 1);
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
  //return success
  return 0;
}

/*Start building the Subtrees Tree*/
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

/*sift down the new root until it is at the correct position*/
int siftDown(struct node * parent, int currPos){
  //calculate the potential positions
  int rPos = (currPos*2)+2;
  int lPos = (currPos*2)+1;
  //check if we are at the end of the heap
  if(rPos > myHeap -> cap || lPos > myHeap -> cap){
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
  //return the number of transferrences
  return i;
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

/* iterates through the files and first calls file reader on each file */
int buildCodebook (int filesSize){
  //random local variables
  int counter;
  char * temp;
  //loop through the files array
  for(counter = 0; counter < filesSize; counter++){
    //check out the working file path
    temp = files[counter];
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
  //TODO: delete, this is only for testing purposes
  //printf("%s", myBuffer);
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
    if(ctemp == '\t' || ctemp == ' ' || ctemp == '\n' || ctemp == ',' || ctemp == '.' || ctemp == '"'){
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
          tableInsert("$\t");
          break;
        case ' ':
          tableInsert("$ ");
          break;
        case '\n':
          tableInsert("$\n");
          break;
        case ',':
          tableInsert(",");
          break;
        case '.':
          tableInsert(".");
          break;
        case '"':
          tableInsert("\"");
          break;
      }
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
  int sum = 0;
  //loop through the characters of the token
  for (int k = 0; k < strlen(token); k++){
    //calculate the ascii sum of the token
    sum = sum + (int)token[k];
  }
  //mod it to fit inside the 256 length array
  int index = sum % 256;
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