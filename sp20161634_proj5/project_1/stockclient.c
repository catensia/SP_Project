/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
	fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {

        if(!strcmp(buf, "exit\n")){
            break;
        }
        

        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);

        char type[7];
        int offset;
        char *bufptr=buf;
        sscanf(buf, "%s%n", type, &offset);
        if(!strcmp(type, "show")){
            bufptr+=offset;
            int stock_num;
            int a,b,c;
            sscanf(bufptr, "%d %n", &stock_num, &offset);
            bufptr+=offset;
            for(int i=0;i<stock_num;i++){
                sscanf(bufptr, "%d %d %d %n", &a, &b, &c, &offset);
                bufptr+=offset;
                sprintf(buf, "%d %d %d\n", a,b,c);
                Fputs(buf, stdout);
            }
        }
        else{
            Fputs(buf, stdout);
        }
    }
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */
