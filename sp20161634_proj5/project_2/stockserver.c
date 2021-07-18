/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#include <stdbool.h>
#define SBUFSIZE 100
#define NTHREADS 1000

static int byte_cnt;
static sem_t mutex;
int stock_num=0;
int thread_num=0;

typedef struct Node{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    struct Node *leftChild;
    struct Node *rightChild;
}Node;

typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sbuf_t;
sbuf_t sbuf;


clock_t start, end;
double res;
Node* root = NULL;
FILE *stock_file;
bool closed=false;

void *thread(void *vargp);
static void init_echo_cnt(void);
void echo_cnt(int connfd);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
void echo_cnt(int connfd);
void inOrder(Node *ptr, char *buf);
Node* search(Node*root, int ID);
Node* insertItem(Node* root, int stock_ID, int stock_price, int left_stock);
Node* availableNode(Node* root);
void update(FILE *fp, Node* root);

void update(FILE *fp, Node* root){
    char buf[MAXLINE];
    memset(buf, 0, sizeof(buf));
    inOrder(root, buf);
    char *bufptr=buf;
    int offset,a,b,c;
    fp=fopen("stock.txt", "w");
    for(int i=0;i<stock_num;i++){
        sscanf(bufptr, "%d %d %d %n", &a,&b,&c, &offset);
        bufptr+=offset;
        fprintf(fp, "%d %d %d\n", a,b,c);
        printf("%d %d %d\n", a,b,c);
    }
    fclose(fp);
    fp=fopen("stock.txt", "r+");
}


void echo_cnt(int connfd) 
{
    int n;
    char buf[MAXLINE];
    rio_t rio;
    static pthread_once_t once = PTHREAD_ONCE_INIT;

    Pthread_once(&once, init_echo_cnt);
    Rio_readinitb(&rio, connfd);
    while((n=Rio_readlineb(&rio, buf, MAXLINE))!=0){
        
        char type[10];
        int req_ID, req_n;
        sscanf(buf, "%s %d %d", type, &req_ID, &req_n);
        memset(buf, 0, sizeof(buf));
        if(!strcmp(type, "show")){
            sprintf(buf, "show %d ", stock_num);
            inOrder(root, buf);
            strcat(buf, "\n");
        }
        else if(!strcmp(type, "buy")){
            Node* target=search(root, req_ID);
            if(target->left_stock<req_n){
                sprintf(buf, "Not enough left stock\n");
            }
            else{
                P(&mutex);
                target->left_stock-=req_n;
                sprintf(buf, "[buy] success\n");
            }
        }
        else if(!strcmp(type, "sell")){
            Node* target=search(root, req_ID);
            P(&mutex);
            target->left_stock+=req_n;
            sprintf(buf, "[sell] successs\n");
        }
        else{
            sprintf(buf, "wrong input\n");
        }
        n=strlen(buf);
        byte_cnt+=n;
        V(&mutex);
        Rio_writen(connfd,buf,n);
    }

}

void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n=n;
    sp->front=sp->rear=0;
    Sem_init(&sp->mutex, 0,1);
    Sem_init(&sp->slots, 0,n);
    Sem_init(&sp->items, 0,0);
}

void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)]=item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}


static void init_echo_cnt(void){
    Sem_init(&mutex, 0,1);
    byte_cnt=0;
}

void *thread(void *vargp){
    Pthread_detach(pthread_self());
    while(1){
        int connfd = sbuf_remove(&sbuf);
        echo_cnt(connfd);
        Close(connfd);
        thread_num--;
        if(thread_num==0){
            update(stock_file, root);
        }
    }
}


void inOrder(Node *ptr, char *buf){
    if(ptr==NULL)
        return;
    inOrder(ptr->leftChild, buf);
    char temp[MAXLINE];
    sprintf(temp, "%d %d %d ", ptr->ID, ptr->left_stock, ptr->price);
    strcat(buf, temp);
    inOrder(ptr->rightChild, buf);
}

Node* search(Node*root, int ID){
    if(root==NULL)
        return NULL;
    if(root->ID==ID)
        return root;
    else if(root->ID>ID)
        return search(root->leftChild, ID);
    else
        return search(root->rightChild, ID);
}

Node* insertItem(Node* root, int stock_ID, int stock_price, int left_stock){
    if(root == NULL){
        root = (Node*)malloc(sizeof(Node));
        root->ID=stock_ID; root->price=stock_price; root->left_stock=left_stock;
        root->leftChild=root->rightChild=NULL;
        return root;
    }
    else{

        if(root->ID>stock_ID){
            root->leftChild=insertItem(root->leftChild, stock_ID, stock_price, left_stock);
        }
        else root->rightChild=insertItem(root->rightChild, stock_ID, stock_price, left_stock);

    }
    return root;
}

Node* availableNode(Node* root){
    Node *cur;
    for(cur=root;cur->leftChild!=NULL;cur=cur->leftChild){}
    return cur;
}


int main(int argc, char **argv) 
{
    int listenfd, connfd,i;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    stock_file = fopen("stock.txt", "r+"); 
    int stock_ID, left_stock, stock_price;
    while(fscanf(stock_file, "%d %d %d", &stock_ID, &left_stock, &stock_price)!=EOF){
        stock_num++;
        root=insertItem(root, stock_ID, stock_price, left_stock);
    }

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    for(i=0;i<NTHREADS;i++){
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
	    clientlen = sizeof(struct sockaddr_storage); 
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        thread_num++;
        printf("Connected to (%s, %s) Total Threads: %d\n", client_hostname, client_port, thread_num);
        sbuf_insert(&sbuf, connfd);

        
    }
    exit(0);
}
/* $end echoserverimain */
