#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

pthread_cond_t trie_write_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
// declaring mutex
pthread_mutex_t lock_trie_write = PTHREAD_MUTEX_INITIALIZER;


int thread_count = 0;

// C implementation of search and insert operations
// on Trie
#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

// Alphabet size (# of symbols)
#define ALPHABET_SIZE (27)

// Converts key current character into index
// use only 'a' through 'z' and lower case
#define CHAR_TO_INDEX(c) ((int)c - (int)'a')
#define THREAD_SIZE (10)
#define FILE_NUM (10)

// trie node
struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    // isEndOfWord is true if the node represents
    // end of a word
    bool isEndOfWord;
    int count; // it stores the replication of a word, so in insert count will be incremented by 1.

};


// Returns new trie node (initialized to NULLs)
struct TrieNode *getNode(void)
{
    struct TrieNode *pNode = NULL;

    pNode = (struct TrieNode *)malloc(sizeof(struct TrieNode));

    if (pNode)
    {
        int i;

        pNode->isEndOfWord = false;
        pNode->count = 0;

        for (i = 0; i < ALPHABET_SIZE; i++)
            pNode->children[i] = NULL;

    }

    return pNode;
}

struct thread_args{
    struct TrieNode *arg1; // root pointer
    int arg2; // thread id
};

// If not present, inserts key into trie
// If the key is prefix of trie node, just marks leaf node
void insert(struct TrieNode *root, const char *key, int inc_size) // if it is called from merge then size will added to count
{

    int level;
    int length = strlen(key);
    int index;
    struct TrieNode *pCrawl = root;

    for (level = 0; level < length; level++)
    {
        if(!isalpha(key[level]))// if it is not a letter then it will be counted as space character
            index = 26;
        else{

            index = CHAR_TO_INDEX(tolower(key[level]));
        }
        if (!pCrawl->children[index]){
            pCrawl->children[index] = getNode();
        }

        pCrawl = pCrawl->children[index];
    }

    // mark last node as leaf
    pCrawl->isEndOfWord = true;

    if(inc_size == -1) // if the function is not called from merge function then inc_size will be -1
        pCrawl->count = pCrawl->count + 1;
    else // else it is called from merge function
        pCrawl->count = pCrawl->count + inc_size;
}

void display(struct TrieNode* root, char str[], int level,FILE* out_file)
{
    // If node is leaf node, it indicates end
    // of string, so a null character is added
    // and string is displayed

    if (root->isEndOfWord != false)
    {   //printf("%d\n",level);
        str[level] = '\0';
        //printf("%s\t%d\n", str, root->count);
        fprintf(out_file,"%s\t%d\n",str,root->count);

    }

    int i;
    for (i = 0; i < ALPHABET_SIZE; i++)
    {
        // if NON NULL child is found
        // add parent key to str and
        // call the display function recursively
        // for child node
        if (root->children[i])
        {
            if(i == 26) // index 62 space karakteri olacak
                str[level] = 32;
            else // kucuk harf ise
                str[level] = i + 'a';
            display(root->children[i], str, level + 1, out_file);
        }
    }
}

struct TrieNode *merge(struct TrieNode* root,struct TrieNode *root_glob, char str[], int level)
{
    // If node is leaf node, it indicates end
    // of string, so a null character is added
    // and string is displayed

    if (root->isEndOfWord != false)
    {   //printf("%d\n",level);
        str[level] = '\0';
        //printf("%s and %d\n",str,root->count);
        insert(root_glob, str, root->count);
    }

    int i;
    for (i = 0; i < ALPHABET_SIZE; i++)
    {
        // if NON NULL child is found
        // add parent key to str and
        // call the display function recursively
        // for child node
        if (root->children[i])
        {
            if(i == 26) // index 62 space karakteri olacak
                str[level] = 32;
            else // kucuk harf ise
                str[level] = i + 'a';
            merge(root->children[i], root_glob, str, level + 1);
        }
    }
    return root_glob;
}

struct TrieNode *readOneAndConstructTrie(struct TrieNode* root, FILE* in_file){

    char buffer[1000];                 // decide the buffer size as per your requirements.
    while((fgets (buffer, 1000, in_file))!= NULL) {

        buffer[strlen(buffer)-1] = 0; // get rid of end of line character
        //printf("%s\n",buffer);
        insert(root, buffer, -1);

    }
     fclose(in_file);
    return root;
}

struct TrieNode *readMultipleAndConstructTrie(struct TrieNode* root, FILE* fileptr){ // read binary

    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    int filelen = ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file

    int buffer_length = 3000;
    char *buffer = (char *)malloc(buffer_length+1 * sizeof(char)); // Enough memory for the file

    int i = 0;
    //printf("filelen = %d\n",filelen);
    bool control = false;
    while(!control){
        fread(buffer, buffer_length, 1, fileptr);
        //printf("%s\n",buffer);
        int t = strlen(buffer)-1;
        int shift_amount = 0;
        while(buffer[t]!='\n'){
            //printf("%c",buffer[t]);
            t = t - 1;
            shift_amount = shift_amount + 1;
        }

        int offset = buffer_length-shift_amount;
        //printf("offset = %d\n",offset);
        buffer[offset] = 0;

        char *pch = strtok (buffer,"\n");
        while (pch != NULL)
        {
            insert(root, pch, -1);
            //printf ("%s\n",pch);
            pch = strtok (NULL, "\n");
        }
        shift_amount = -1*shift_amount;

        fseek(fileptr,shift_amount,SEEK_CUR);
        i = i + offset;
        //printf("i=%d\n",i);
        if(filelen-i<buffer_length){

            buffer_length = filelen-i;
            control = true;
            //printf("new buf len = %d\n",buffer_length);

        }
    }

    fread(buffer, buffer_length, 1, fileptr);
    buffer[buffer_length] = 0;
    char *pch = strtok (buffer,"\n");
    while (pch != NULL)
    {
        insert(root, pch, -1);
        //printf ("%s\n",pch);
        pch = strtok (NULL, "\n");
    }
    fclose(fileptr);
    return root;
}


struct TrieNode *readAllAndConstructTrie(struct TrieNode* root, FILE* fileptr){ // read binary

    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    int filelen = ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file

    int buffer_length = filelen;
    char *buffer = (char *)malloc(buffer_length+1 * sizeof(char)); // Enough memory for the file
    fread(buffer, buffer_length, 1, fileptr);
    fclose(fileptr);
    buffer[filelen] = 0;
    char *pch = strtok (buffer,"\n");
    while (pch != NULL)
    {
        insert(root, pch, -1);
        //printf ("%s\n",pch);
        pch = strtok (NULL, "\n");
    }

    fclose(fileptr);
    return root;
}

void *getTask(void *command) {
    //printf("asasdsklalsak\n");

   struct thread_args *args = (struct thread_args *)command;
   struct TrieNode *root = args->arg1;
   int id = args->arg2;
   //printf("id = %d\n",id);
   //printf("%my id is = %d and root = d\n",id,root);
   char str[2];
   //printf("root address = %d and I am pid of %d",root,id);
   sprintf(str, "%d", id);
   char filename[15] = "data";
   strcat(filename,str);
   strcat(filename,".txt");
   //printf("%s\n",filename);

   FILE *in_file = fopen(filename,"r");
   if (in_file == NULL)
   {
    printf("Error! Could not open file\n");
    exit(-1); // must include stdlib.h
   }

    pthread_mutex_lock(&thread_mutex);
    printf("Pid = %d is inserting queries from %s into a Trie\n", id, filename);
    root = readOneAndConstructTrie(root, in_file);
    printf("Pid = %d is done with inserting queries from %s into a Trie\n", id, filename);
    pthread_mutex_unlock(&thread_mutex);

    thread_count = thread_count + 1;

    pthread_mutex_lock(&lock_trie_write);
    //printf("I'am here pid = %d",id);

    if(thread_count == THREAD_SIZE){
        for(int i = 0; i < THREAD_SIZE; i++)
            pthread_cond_signal(&trie_write_cond); // send signals for each thread when all threads done with trie construction
    }
    while(thread_count != THREAD_SIZE){

        pthread_cond_wait(&trie_write_cond, &lock_trie_write); // wait

    }
    pthread_mutex_unlock(&lock_trie_write);
    printf("Pid = %d is DONE!!!\n", id);
        /*
        pthread_mutex_lock(&thread_mutex);
        display(root,str1,level,out_file);
        pthread_mutex_unlock(&thread_mutex);
        */
}

// Driver


int main()
{
     //DENEME
    /*
     struct TrieNode *root = getNode();
    FILE *in_file = fopen("totalData.txt","r");
    if (in_file == NULL)
    {
        printf("Error! Could not open file\n");
        exit(-1); // must include stdlib.h
    }

    root = readOneAndConstructTrie(root, in_file);

    FILE *out_file = fopen("task11","w");

    if (out_file == NULL)
    {
        printf("Error! Could not open file\n");
        exit(-1); // must include stdlib.h
    }

    int level = 0;
    char str[2000];
    display(root, str, level, out_file);
    */
    // DENEME


    int task_id;
    printf("Task 1: Sequential Execution - One Query at a Time"
           "\nTask 2: Sequential Execution - Multiple Queries"
           "\nTask 3: Sequential Execution - All Queries"
           "\nTask 4: Threaded Execution - Single Trie"
           "\nTask 5: Threaded Execution - Multiple Tries"
           "\nPlease enter the task number:");
    scanf("%d", &task_id);


    if(task_id == 1){
        clock_t start, end;
        double cpu_time_used;
        start = clock();
       struct TrieNode *root = getNode();

       for(int i = 0; i < FILE_NUM; i++){
           char str[2];
           sprintf(str, "%d", i+1);
           char filename[15] = "data";
           strcat(filename,str);
           strcat(filename,".txt");
           printf("Inserting queries from %s into a Trie\n",filename);

           FILE *in_file = fopen(filename,"r");
           if (in_file == NULL)
           {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
           }

            root = readOneAndConstructTrie(root, in_file);
       }

        FILE *out_file = fopen("task1","w");

        if (out_file == NULL)
        {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
        }
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("task1 took %f seconds to execute\n",cpu_time_used);

        int level = 0;
        char str[2000];
        display(root, str, level, out_file);
    }
    else if(task_id == 2){
       clock_t start, end;
       double cpu_time_used;
       start = clock();

       struct TrieNode *root = getNode();

       for(int i = 0; i < FILE_NUM; i++){
           char str[2];
           sprintf(str, "%d", i+1);
           char filename[15] = "data";
           strcat(filename,str);
           strcat(filename,".txt");
           printf("Inserting queries from %s into a Trie\n",filename);

           FILE *in_file = fopen(filename,"r");
           if (in_file == NULL)
           {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
           }

            root = readMultipleAndConstructTrie(root, in_file);
       }

        FILE *out_file = fopen("task2","w");

        if (out_file == NULL)
        {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
        }
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("task2 took %f seconds to execute\n",cpu_time_used);

        int level = 0;
        char str[2000];
        display(root, str, level, out_file);
    }

    else if(task_id == 3){

        clock_t start, end;
        double cpu_time_used;
        start = clock();

        struct TrieNode *root = getNode();

       for(int i = 0; i < FILE_NUM; i++){
           char str[2];
           sprintf(str, "%d", i+1);
           char filename[15] = "data";
           strcat(filename,str);
           strcat(filename,".txt");
           printf("Inserting queries from %s into a Trie\n",filename);

           FILE *in_file = fopen(filename,"r");
           if (in_file == NULL)
           {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
           }

            root = readAllAndConstructTrie(root, in_file);
       }

        FILE *out_file = fopen("task3","w");

        if (out_file == NULL)
        {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
        }

        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("task3 took %f seconds to execute\n",cpu_time_used);

        int level = 0;
        char str[2000];
        display(root, str, level, out_file);
    }

    else if(task_id == 4){
        clock_t start, end;
        double cpu_time_used;
        start = clock();

        int p;
        struct TrieNode *root  = getNode();
        FILE *out_file = fopen("task4","w");

        if (out_file == NULL)
        {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
        }
        pthread_t threads[THREAD_SIZE];
        int t;
        for (t = 0; t < THREAD_SIZE; t++) { // iteration for thread creating

            struct thread_args *args = (struct thread_args *)malloc(sizeof(struct thread_args));
            args->arg1 = root;
            args->arg2 = t+1;

            p = pthread_create(&threads[t], NULL, getTask, (void *)args); // thread creation

            if (p) {
                printf("ERROR; return code from pthread_create() is %d\n", p);
                exit(-1);
            }


        }
        for( int k = 0; k < THREAD_SIZE; k++){
            pthread_join(threads[k],NULL);

        }

        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("task4 took %f seconds to execute\n",cpu_time_used);

        int level = 0;
        char str[2000];
        display(root, str, level, out_file);
    }
    else if(task_id == 5){
        clock_t start, end;
        double cpu_time_used;
        start = clock();

        int p;
        struct TrieNode* root_glob = getNode();
        struct TrieNode *root1 = getNode();
        struct TrieNode *root2 = getNode();
        struct TrieNode *root3 = getNode();
        struct TrieNode *root4 = getNode();
        struct TrieNode *root5 = getNode();
        struct TrieNode *root6 = getNode();
        struct TrieNode *root7 = getNode();
        struct TrieNode *root8 = getNode();
        struct TrieNode *root9 = getNode();
        struct TrieNode *root10 = getNode();

        FILE *out_file = fopen("task5","w");

        if (out_file == NULL)
        {
            printf("Error! Could not open file\n");
            exit(-1); // must include stdlib.h
        }

        struct TrieNode *arr[THREAD_SIZE] = {root1,root2,root3,root4,root5,root6,root7,root8,root9,root10};

        pthread_t threads[THREAD_SIZE];
        int t;
        struct  thread_args* args;
        for (t = 0; t < THREAD_SIZE; t++) { // iteration for thread creating
            struct thread_args* args = (struct thread_args *)malloc(sizeof(struct thread_args));

            args->arg1 = arr[t];
            //printf("arg_node = %d and arr[t] = %d\n",args->arg1, arr[t]);
            args->arg2 = t+1;

            p = pthread_create(&threads[t], NULL, getTask, (void *)args); // thread creation

            if (p) {
                printf("ERROR; return code from pthread_create() is %d\n", p);
                exit(-1);
            }

        }
        for( int k = 0; k < THREAD_SIZE; k++){
            pthread_join(threads[k],NULL);
        }

        for(int i = 0; i < THREAD_SIZE; i++){
            int level = 0;
            char str[2000];
            root_glob = merge(arr[i],root_glob,str,level);
            free(arr[i]);
        }
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("task5 took %f seconds to execute\n",cpu_time_used);

        int level = 0;
        char str[2000];
        display(root_glob,str,level,out_file);
    }

    return 0;
}
