#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <string.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#define CHUNK 1024
#define MAX_BUFFER_SIZE 0x40000
#define MAX_CONNECTIONS 1000

char root[PATH_MAX];
char not_found [200]= "HTTP/1.1 404 Not Found\r\n"
						"Connection: close\r\n"
						"Content-Type: text/html\r\n\r\n"
						"<html><body>Error 404: Not Found</body></html>\r\n\0";
char ok [200]= "HTTP/1.1 200 OK\r\n"
				"Connection: close\r\n";
				//"Content-Type: application/octet-stream\r\n\r\n";
char port[6];
int socket_fd;
int clients[MAX_CONNECTIONS] = {-1};
int sel_result, recv_result;
socklen_t len;
struct sockaddr addr;
char buffer [MAX_BUFFER_SIZE] = {'\0'};
int j = 0;

void startServer(char *);
void processreq();
void respond(int);
void response(char*, int );
bool endsWith(char *, char *);

int main(int argc, char** argv){
	signal(SIGCHLD, SIG_IGN);
	if (getcwd(root, sizeof(root)) == NULL)
       exit(1);//error
	strcpy(port, "10000");
	char c;
	while ((c = getopt (argc, argv, "p:")) != -1) //flag -p za postavljanje proizvoljnog porta
        switch (c)
        {
            case 'p':
                strcpy(port,optarg);
                break;
            case '?':
                printf("Wrong arguments given!!!\n");
                exit(1);
            default:
                exit(1);
        }

	for (int i = 0; i < MAX_CONNECTIONS; ++i)
        clients[i]=-1;
    startServer(port);
	while(1)
		processreq();
    return 0;
}


//startanje servera, otvaranje socketa, bind i listen
void startServer(char *port){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    
    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    int err;
    if(err = getaddrinfo(NULL, port, &hints, &result) != 0){
        printf("%s\n", gai_strerror(err));
        exit(1);
    }
    
    //kreiranje i bindanje socketa sa adresom
    for (rp = result; rp != NULL; rp = rp->ai_next){
        socket_fd = socket (rp->ai_family, rp->ai_socktype, 0);
        if (socket_fd == -1) continue;
        if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
    }
    if (rp == NULL){
        printf("socket() or bind() error\n");
        exit(1);
    }

    freeaddrinfo(result);

    //listen
    if (listen (socket_fd, 10) != 0){
        printf("listen() error\n");
        exit(1);
    }
}

//cekanje i procesiranje 
void processreq(){
	len = sizeof(addr);
	clients[j] = accept(socket_fd, &addr, &len);
	 if (clients[j]<0)
		printf("accept() error\n");
	else{
		if(fork() == 0){
			respond(j);
			kill(getpid(), SIGTERM);
			//exit(0);
		}
	}
	while (clients[j] != -1)
		j = (j + 1) % MAX_CONNECTIONS;
}

//procesiranje zahtjeva i slanje odgovora
void respond(int peersock_fd){
				memset(&buffer, 0, MAX_BUFFER_SIZE);
				recv_result = recv(clients[peersock_fd], buffer, MAX_BUFFER_SIZE, 0);
			if (recv_result > 0){
				printf("%.*s", strlen(buffer), buffer);
				char *line[3], path[PATH_MAX], data[CHUNK];
				struct stat s;
				
				line[0] = strtok (buffer, " \t\n");
				if (strncmp(line[0], "GET\0", 4) == 0){
          			line[1] = strtok (NULL, " \t");
            		line[2] = strtok (NULL, " \t\n");				
					if(strncmp(line[2], "HTTP/1.0", 8) != 0 && strncmp(line[2], "HTTP/1.1", 8) != 0){
                		write(clients[peersock_fd], "HTTP/1.0 400 Bad Request\n", 25);
           			}else{
           				strcpy(path, root);
           				strcat(path, line[1]);
           				if(stat(path, &s) == -1){
           					send(clients[peersock_fd], not_found, strlen(not_found), MSG_NOSIGNAL);
           				}
           				else if(S_ISDIR(s.st_mode)){
           				 	char tmp_path[PATH_MAX] = {'\0'};
           				 	strcpy(tmp_path, path);
           					strcat(tmp_path, "/index.html");
           					if(stat(tmp_path, &s) == -1){
	           					//mojls
	           					char tmp[200] = {'\0'};
								int n, file;
								char command[1000] = {'\0'};strcpy(command, "./mojls ");strcat(command, path);
								FILE* f = popen(command, "r");
								file = fileno(f);
								
								write(clients[peersock_fd], line[1], strlen(line[1]));
								write(clients[peersock_fd], ":\n", 2);
								while ((n = read(file, data, CHUNK)) > 0)
		                    			write(clients[peersock_fd], data, n);
								pclose(f);
	           				}
	           				else{
	           					//index.html
	           					response(tmp_path, clients[peersock_fd]);
	           				}
           				}
           				else{
							//file
							response(path, clients[peersock_fd]);
					    }
           			}
				}			
			}
			else if (recv_result == 0){
				printf("client disconnected\n");
			}
			else{
				printf("recv() error\n");
			}
			shutdown (clients[peersock_fd], SHUT_RDWR);
			close(clients[peersock_fd]);
			clients[peersock_fd] = -1;
}

//salje http odgovor sa trazenim fajlom unutar tijela odgovora
void response(char* path, int j){
	FILE* f;
	int file, n;
	if ((file = open(path, O_RDONLY)) != -1){//ako je fajl uspjesno otvoren
			char tmp[100] = {'\0'};
			strcpy(tmp, ok);
			if(endsWith(path, ".txt") || 
			   endsWith(path, ".html") ||
			   endsWith(path, ".htm"))//ako je fajl tipa html, htm ili txt, salje se kao tekstualni fajl(otvara se direktno u browseru)
				strcat(tmp, "Content-Type: text/html\r\n\r\n");
			else//u suprotnom salji kao binarni fajl
				strcat(tmp, "Content-Type: application/octet-stream\r\n\r\n\0");
	 			send(j, tmp, strlen(tmp), MSG_NOSIGNAL);//slanje zaglavlja
	 		struct stat st;
			fstat(file, &st);
			sendfile(j, file, 0, st.st_size);//slanje fajla
			close(file);
	}
	else//ako nije baci error 404
		send(j, not_found, strlen(not_found), MSG_NOSIGNAL);
}


//prima putanju fajla i sufix te vraca true ako se ekstenzija fajla podudara sa suffix
bool endsWith(char *str, char *suffix){
	int str_len = strlen(str);
	int suffix_len = strlen(suffix);
	return (str_len >= suffix_len) && (0 == strcmp(str + (str_len-suffix_len), suffix));
}
