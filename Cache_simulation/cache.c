#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<unistd.h>
#define d 0 //data

char FIFO[5] = "FIFO";
char LRU[4] = "LRU";
char RANDOM[7] ="RANDOM";
char WT[3] = "WT";
char WB[3] = "WB";

struct block{
    int validBit;
    int tag;
    int data;
};

struct blockLL{ //works for fully associative where it can be seen as single set and all are in different ways 
    int way_no;
    struct blockLL* next;
    struct blockLL* prev;
};

struct blockLL* createBlockLL(struct blockLL** head,struct blockLL** tail,int way_no){
    struct blockLL* new_block = (struct blockLL*)malloc(sizeof(struct blockLL));
    (*tail)->next = new_block;
    new_block->way_no = way_no;
    new_block->next = NULL;
    new_block->prev = (*tail);
    (*tail) = new_block;
    //printf("new block,tail in way,head:%d %d %d\n",new_block->way_no,(*tail)->way_no,(*head)->way_no);
    return new_block;
}

int removeHead(struct blockLL** head,struct blockLL** tail){
    if((*head)!=NULL){
        struct blockLL* new_head = (*head)->next;
        new_head->prev = NULL;
        (*head) = new_head;
    }
    else{
        printf("head isn't created properly\n");
    }
}

void moveLruBlockToHead(struct blockLL* lruBlock,struct blockLL** head,struct blockLL** tail){
    if( lruBlock == (*head) ){
        return;
    }
    
    struct blockLL* prev_block = lruBlock->prev;
    struct blockLL* next_block = lruBlock->next;
    struct blockLL* old_head = (*head);
    if(next_block != NULL){
        next_block->prev = prev_block;
    }
    prev_block->next = next_block;
    old_head->prev = lruBlock;
    lruBlock->prev = NULL;
    lruBlock->next = old_head;
    (*head) = lruBlock;
    if(lruBlock == (*tail)){
        (*tail) = prev_block;
    }
}

void deleteBlock(struct blockLL* block,struct blockLL** head,struct blockLL** tail){
    if((*head) == block){
        (*head) = (*head)->next;
        (*head)->prev = NULL;
        return;
    }
    else if((*tail) == block){
        (*tail) = (*tail)->prev;
        (*tail)->next = NULL;
        return;
    }
    struct blockLL* prev_block = block->prev;
    struct blockLL* next_block = block->next;
    prev_block->next = next_block;
    next_block->prev = prev_block;
}

struct blockLL* blockWithGivenWay(int way,struct blockLL* head){
    struct blockLL* temp = head;
    while(temp != NULL){
        if(temp->way_no == way){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

struct blockLL* createHead(struct blockLL** head,struct blockLL** tail,int way_no){
    struct blockLL* new_block = (struct blockLL*)malloc(sizeof(struct blockLL));
    new_block->way_no = way_no;
    (*head) = new_block;
    (*head)->next = NULL;
    (*head)->prev = NULL;
    (*tail) =new_block;
    (*tail)->next = NULL;
    (*tail)->prev = NULL;
    //printf("head and tail in way %d %d\n",(*head)->way_no,(*tail)->way_no);
    return new_block;
}

unsigned int getSegOfAddress(int higherEnd,int lowerEnd,unsigned int address){
    unsigned int segment = address<<(31-higherEnd);
    segment = segment>>(31-higherEnd+lowerEnd);
    return segment;
}

struct block placeBlock(int associativity,struct block a[][associativity],int rowPlace,int colPlace,int tag,int data){
    //struct block* newBlock = (struct block*)malloc(sizeof(struct block));
    a[rowPlace][colPlace].data = data;
    a[rowPlace][colPlace].tag = tag;
    a[rowPlace][colPlace].validBit = 1;
}

int initialise(int associativity,struct block a[][associativity],int row,int col,int initialiser){
    initialiser =0; 
    for(int i=0;i<row;i++){
        for(int j=0;j<col;j++){
            a[i][j].validBit = 0;
        }
    }
}

int printLL(struct blockLL* head){
    if(head == NULL){
        printf("head 1 is NULL\n");
    }
    while(head !=  NULL){
        if(head == NULL){
        printf("head is NULL\n");
        }
        printf("%d ",head->way_no);
        head = head->next;
        
        }
}

int main(int argCount,char *argValues[]){

    srand(time(NULL) ^ getpid()); //seed for rand
    FILE* fp_config = fopen(argValues[1],"r");//file name is given in the command line argument
    char inputLine[20];int lineNum = 0;
    int cacheSize=0,blockSize=0,associativity = 0;
    char repPolicy[10],wbPolicy[3];
    while(fgets(inputLine,sizeof(inputLine),fp_config)!=NULL){
        if( inputLine[0] == ' ' || inputLine[0] == '\n' ){ //to ignore trailing spaces and new line char in the file
            continue;
        }
        lineNum++;
        switch (lineNum){
            case 1:
                sscanf(inputLine,"%d",&cacheSize);
                break;
            case 2:
                sscanf(inputLine,"%d",&blockSize);
                break;
            case 3:
                sscanf(inputLine,"%d",&associativity);
                break;
            case 4:
                sscanf(inputLine,"%s",repPolicy);
                break;
            case 5:
                sscanf(inputLine,"%s",wbPolicy);
                break;
            
            default:
                break;
        }

    }
    fclose(fp_config);
    //printf("%d %d %d %s %s\n",cacheSize,blockSize,associativity,repPolicy,wbPolicy);
    
    //setting cache

    int sets =0;
    int associativity_original = associativity;
    if(associativity !=0){
        sets = cacheSize/(blockSize*associativity);
    }
    else{ //fully associative chaged to obtain uniformity
        sets =1;
        associativity = cacheSize/blockSize;
    }
    //handle fully associative cache properly there we dont search by sets all are searched via ways
    struct block cache[sets][associativity];
    initialise(associativity,cache,sets,associativity,0);//valid bit us made 0 initially

    struct blockLL* fifoHead[sets];
    struct blockLL* fifoTail[sets];
    struct blockLL* lruHead[sets];
    struct blockLL* lruTail[sets];
    int firstAccess[sets]; //created to check if they are accessed to create head
    for(int i=0;i<sets;i++){
        firstAccess[i] = 0;
    }
    //setting cache

    FILE *fp_access = fopen(argValues[2],"r");
    while(fgets(inputLine,sizeof(inputLine),fp_access) != NULL){
        const char delim[5] = " :,\n";
        char *token1 = strtok(inputLine,delim);
        char *token2 = strtok(NULL,delim);
        char mode = token1[0];
        char addressChar[11]; unsigned int address; //as address is 32 bit it fits in integer
        //printf("%ld %ld %ld\n",strlen(inputLine),strlen(token1),strlen(token2));
        
        if(token2 != NULL){
            strcpy(addressChar,token2);
            sscanf(&addressChar[2],"%x",&address); //to not include 0x in the integer
            //printf("%u-add\n",address);
        }

        unsigned int set_lowEnd = log2(blockSize);
        unsigned int set_highEnd = log2(sets)+set_lowEnd-1;
        unsigned int setOfBlock = getSegOfAddress(set_highEnd,set_lowEnd,address);
        if(associativity_original == 0){ //for fully associative cache there is only one set
            setOfBlock = 0;
            set_highEnd = log2(blockSize)-1;
        }
        unsigned int tagOfBlock = getSegOfAddress(31,set_highEnd+1,address);
        //change this for fully associative
        int check = 0;int placeIn = -1;int hit = 0;int hit_way=0;
        

        for(int i=0;i<associativity;i++){
            if(cache[setOfBlock][i].validBit !=0){
                if(cache[setOfBlock][i].tag == tagOfBlock){
                    hit = 1;
                    printf("Address: 0x%x, Set: 0x%x, Hit, Tag: 0x%x\n",address,setOfBlock,tagOfBlock);
                    hit_way = i;
                    break;
                }
                check++;
            }
            else{
                check++;
                //printf("placein for tag %d %d\n",tagOfBlock,placeIn);
                placeIn = i;//placed at last empty place checked change it properly
            }
        }
        //printf("placein %d\n",placeIn);
        //deciding head 
        int head =0;
        if( (firstAccess[setOfBlock] == 0) && ((mode == 'W' && strcmp(wbPolicy,WB)==0) || mode == 'R') ){
            createHead(&fifoHead[setOfBlock],&fifoTail[setOfBlock],placeIn);
            createHead(&lruHead[setOfBlock],&lruTail[setOfBlock],placeIn);
            firstAccess[setOfBlock] = 1;
            placeBlock(associativity,cache,setOfBlock,placeIn,tagOfBlock,d); 
            printf("Address: 0x%x, Set: 0x%x, Miss, Tag: 0x%x\n",address,setOfBlock,tagOfBlock);
            head = 1;
        }
        else{
            //createBlockLL need to delete already existing block i.e replaced block
            
        }
        if(strcmp(repPolicy,LRU) == 0 && hit == 1){
            //printf("moving\n");
            moveLruBlockToHead(blockWithGivenWay(hit_way,lruHead[setOfBlock]),&lruHead[setOfBlock],&lruTail[setOfBlock]);
            //printf("moved\n");
        }
        //replacement plolicy think efficient approaches for all types
        if( check == associativity && (mode == 'W' && strcmp(wbPolicy,WT)==0) ){ //for write through we dont add the block to cache
            printf("Address: 0x%x, Set: 0x%x, Miss, Tag: 0x%x\n",address,setOfBlock,tagOfBlock); 
        }
        else if(check == associativity && placeIn != -1 && head != 1){//if doesn't match to present blocks and set is not full
            placeBlock(associativity,cache,setOfBlock,placeIn,tagOfBlock,d); 
            createBlockLL(&fifoHead[setOfBlock],&fifoTail[setOfBlock],placeIn);
            struct blockLL* addedBlock = createBlockLL(&lruHead[setOfBlock],&lruTail[setOfBlock],placeIn);
            moveLruBlockToHead(addedBlock,&lruHead[setOfBlock],&lruTail[setOfBlock]);
            printf("Address: 0x%x, Set: 0x%x, Miss, Tag: 0x%x\n",address,setOfBlock,tagOfBlock);       
        }
        if(hit !=1 && placeIn == -1 && (!(mode == 'W' && strcmp(wbPolicy,WT)==0)) ){//all are full replacement policies need to be used
            printf("Address: 0x%x, Set: 0x%x, Miss, Tag: 0x%x\n",address,setOfBlock,tagOfBlock);
            if(strcmp(repPolicy,FIFO) == 0){
                placeIn = fifoHead[setOfBlock]->way_no;
                placeBlock(associativity,cache,setOfBlock,placeIn,tagOfBlock,d);
                if(associativity != 1){
                    deleteBlock(blockWithGivenWay(placeIn,fifoHead[setOfBlock]),&fifoHead[setOfBlock],&fifoTail[setOfBlock]);
                    createBlockLL(&fifoHead[setOfBlock],&fifoTail[setOfBlock],placeIn);
                }
                
            }
            if(strcmp(repPolicy,RANDOM) == 0){
                int randomWay = rand()%associativity;
                //printf("%d-randomway\n",randomWay);
                placeIn = randomWay;
                placeBlock(associativity,cache,setOfBlock,placeIn,tagOfBlock,d);
            }
            if(strcmp(repPolicy,LRU) == 0){
                placeIn = lruTail[setOfBlock]->way_no;
                if(associativity ==1){
                    //no changes are needed new block is placed
                }
                else{
                    deleteBlock(blockWithGivenWay(placeIn,lruHead[setOfBlock]),&lruHead[setOfBlock],&lruTail[setOfBlock]);
                    struct blockLL* addedBlock = createBlockLL(&lruHead[setOfBlock],&lruTail[setOfBlock],placeIn);
                    moveLruBlockToHead(addedBlock,&lruHead[setOfBlock],&lruTail[setOfBlock]);
                }
                placeBlock(associativity,cache,setOfBlock,placeIn,tagOfBlock,d);
                //printLL(lruHead[1]);puts("");
            }
        }
        usleep(10000); 
    }

    //printLL(lruHead[17]);puts("");
    
    //printf("%d-sets\n",sets);
    /*for(int i=0;i<sets;i++){
        if(lruHead[i] !=NULL){
            printLL(lruHead[i]);puts("");
        }
    }*/
    
    fclose(fp_access);
}