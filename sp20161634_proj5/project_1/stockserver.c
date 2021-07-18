/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#include <stdbool.h>

typedef struct Node{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    struct Node *leftChild;
    struct Node *rightChild;
}Node;

int client_num=0;
int byte_cnt=0;
int stock_num=0;
Node* root = NULL;
FILE *stock_file;

void inOrder(Node *ptr, char *buf);
Node* search(Node*root, int ID);
struct timespec begin, end;
float res;
bool con=false;

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
}pool;

void init_pool(int listenfd, pool *p){
    int i;
    p->maxi=-1;
    for(i=0;i<FD_SETSIZE;i++) p->clientfd[i]=-1;
    
    p->maxfd=listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p){
    int i;
    p->nready--;
    for(i=0;i<FD_SETSIZE;i++){
        if(p->clientfd[i]<0){
            p->clientfd[i]=connfd;
            Rio_readinitb(&p->clientrio[i], connfd);
            FD_SET(connfd, &p->read_set);

            if(connfd>p->maxfd) p->maxfd=connfd;
            if(i>p->maxi) p->maxi=i;
            break;
        }
    }
    if(i==FD_SETSIZE) app_error("add_client error: Too many clients");
}

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

void check_clients(pool *p){
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for(i=0;(i<=p->maxi)&&(p->nready>0);i++){
        connfd=p->clientfd[i];
        rio=p->clientrio[i];

        if((connfd>0) && (FD_ISSET(connfd, &p->ready_set))){
            p->nready--;
            if((n=Rio_readlineb(&rio, buf, MAXLINE))!=0){
                byte_cnt+=n;
                char type[10];
                int req_ID;
                int req_n;
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
                        target->left_stock-=req_n;
                        sprintf(buf, "[buy] success\n");
                    }
                }
                else if(!strcmp(type, "sell")){
                    Node* target=search(root, req_ID);
                    target->left_stock+=req_n;
                    sprintf(buf, "[sell] successs\n");
                }
                else{
                    sprintf(buf, "invalid input\n");
                }
                n=strlen(buf);
                Rio_writen(connfd, buf, n);
            }
            else{
                printf("disconnection\n");
                update(stock_file, root);
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i]=-1;
                client_num--;
                if(client_num==0){
                    printf("no connections\n");
        			clock_gettime(CLOCK_MONOTONIC,&end);
			        res=(end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
			        //printf("res : %f\n",res);
                }
                
            }
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


void echo(int connfd);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    static pool pool;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    //load table and make binary search tree
    //Open stock.txt in +r mode
    stock_file = fopen("stock.txt", "r+"); 
    int stock_ID, left_stock, stock_price;
    while(fscanf(stock_file, "%d %d %d", &stock_ID, &left_stock, &stock_price)!=EOF){
        stock_num++;
        root=insertItem(root, stock_ID, stock_price, left_stock);
    }

    listenfd = Open_listenfd(argv[1]); 
    init_pool(listenfd, &pool);

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &pool.ready_set)){
            clientlen=sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
            if(!con && client_num==0){
                clock_gettime(CLOCK_MONOTONIC, &begin);
            }
            client_num++;
            add_client(connfd, &pool);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
        }

        check_clients(&pool);
    }
    fclose(stock_file);
    exit(0);
}
/* $end echoserverimain */
