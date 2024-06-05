/**
*
* UBIT: breckenm || breckenm@buffalo.edu
* 
* References 
* @ref PA1 Demo Code
* @ref Beej's Guide to Network Programming
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>

#include "../include/server.h"
#include "../include/logger.h"



struct client* Clients = NULL; //global pointer to memory addr of clients struct that is the head of the linked list of clients structs
int num_clients = 0;

/////////////////////////

void add_client(int c_fd,int port, char* ip, char* hostname){ //create a new client and put them in the linked list, intialize with all client data
	//O(MAX_MSGS)
	struct client* newclient = (struct client*)malloc(sizeof(struct client));

	memset(newclient->hostname,'\0',sizeof(newclient->hostname));
	memset(newclient->ip_addr,'\0',sizeof(newclient->ip_addr));

	newclient->port = port; //ip hostname need to be constant since assigning memory address instead of copy
	newclient->fd = c_fd; //set the file descriptor for the client to allow clients to send messages indirectly through server to them
	newclient->online = 1; //if they login then they are online until they exit
	newclient->msgs_recv = 0;
	newclient->msgs_sent = 0;
	newclient->bl = NULL;
	newclient->blocked = NULL;

	memcpy(newclient->hostname,hostname,strlen(hostname));
	memcpy(newclient->ip_addr,ip,strlen(hostname));

	//printf("%d\n",strlen(hostname));

	//linked list stores newest client added as the head like a stack (made this choice since more efficient from time complexity standpoint
	//, add_client only O(MAX_MSGS) since just makeing head (Clients) point to newest client and newest client's next now points to previously
	// newest client so dont have to iterate through whole linked list and add at the end (O(MAX_MSGS*N) where N is number of clients in ll))
	newclient->next = NULL;
	newclient->next = Clients;
	Clients = newclient;
}



/////////////////////////

void get_external_ip(char* extern_ip){ //get external ip address of server by creating udp dummy socket and connecting to outside server
//then getting the ip stored, also gets hostname just in case need it for later
	struct sockaddr_in ser_addr;
	memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(53); // DNS port
    ser_addr.sin_addr.s_addr = inet_addr("8.8.8.8");

	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	connect(udp_socket,(struct sockaddr*)&ser_addr,sizeof(ser_addr));

	int ser_addr_size = sizeof(ser_addr);
	getsockname(udp_socket,(struct sockaddr*)&ser_addr,&ser_addr_size);


	inet_ntop(AF_INET,&(ser_addr.sin_addr),extern_ip,BUFFER_SIZE); 

	//printf("%s\n",extern_ip);
	

	close(udp_socket);
}


/////////////////////////

int socket_bind_listen(char* port){ //creates socket for server and binds port to socket and starts to listen for incoming connections
	struct addrinfo hints, *res;

	/* Set up hints structure */
	memset(&hints, 0, sizeof(hints)); //hints allows getaddrinfo to fill in all necessary info inside res struct dynamically based off hints
    	hints.ai_family = AF_INET;
		//hints.ai_family = AF_UNSPEC;
    	hints.ai_socktype = SOCK_STREAM;
    	hints.ai_flags = AI_PASSIVE;

	/* Fill up address structures */
	if (getaddrinfo(NULL, port, &hints, &res) != 0)
		perror("getaddrinfo failed");
	
	/* Socket */
	int server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(server_socket < 0)
		perror("Cannot create socket");
	
	
	/* Bind */ //below commmented code: having issue where server will refuse external ip addresses from connecting so
	//binded socket to external ip address to see if that would fix it. of course it didint

	//char extern_ip[BUFFER_SIZE]; 
	//get_external_ip(extern_ip);

	//struct sockaddr_in extern_info;
	//memset(&extern_info,0,sizeof(extern_info));
	//extern_info.sin_family = AF_INET;
	//extern_info.sin_port = htons(4322);

	//if (inet_pton(AF_INET,extern_ip,&extern_info.sin_addr) <= 0){ //convert the string ip address into the binary and store it in the exter_ip sockaddr_in struct
		//printf("invalid ip");
	//} 

	if(bind(server_socket, res->ai_addr, res->ai_addrlen) < 0 ){
		perror("Bind failed");
	}
	//if(bind(server_socket, (struct sockaddr*)&extern_info,sizeof(extern_info)) < 0){
		//perror("Bind failed");
	//}

	freeaddrinfo(res);
	
	/* Listen */
	//if(listen(server_socket, BACKLOG) < 0)
		//perror("Unable to listen on port");

	return server_socket;
}


////////////////////////////
int compare(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

void handle_STATISTICS_command(){
	cse4589_print_and_log("[STATISTICS:SUCCESS]\n");

	int num_online = 0;
	struct client* c1 = Clients;
	while (c1 != NULL){
		num_online ++;
		c1 = c1->next;
	}

	int ports[num_online];

	int j = 0;
	struct client* curr1 = Clients;
	while(curr1 != NULL){
		ports[j] = curr1->port;
		//printf("%d\n",ports[j]);
		j++;
		curr1 = curr1->next;
	}

	qsort(ports,num_online,sizeof(int),compare);


	//send(sock_index,int_num,strlen(int_num),0);
	char* result = (char*)malloc(sizeof(char)*BUFFER_SIZE*5);
	memset(result,0,BUFFER_SIZE*5);

	int id = 1;
	for (int m = 0; m < num_online; m++){
		//printf("%d\n",ports[m]);
		if (ports[m] != -1){
			struct client* curr = Clients;
			while (curr != NULL){
				if (curr->port == ports[m]){
					char msg_send[BUFFER_SIZE];
					memset(msg_send,'\0',sizeof(msg_send));

					curr->hostname[strlen(curr->hostname)] = '\0';
					curr->ip_addr[strlen(curr->ip_addr)] = '\0';

					if (curr->online == 1){
						char* status = "logged-in";
						sprintf(msg_send,"%-5d%-35s%-8d%-8d%-8s\n",id,curr->hostname,curr->msgs_sent,curr->msgs_recv,status);
					}else if (curr->online == 0){
						char* status = "logged-out";
						sprintf(msg_send,"%-5d%-35s%-8d%-8d%-8s\n",id,curr->hostname,curr->msgs_sent,curr->msgs_recv,status);
					}

					//send(sock_index,msg_send,strlen(msg_send),0);
					//if (type == 's'){
						//cse4589_print_and_log("%s",msg_send);
						//fflush(stdout);
					//}
										
					//"%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num
					//if (type == 'c'){
					strcat(result,msg_send);
					//}
					memset(msg_send,'\0',sizeof(msg_send));

					id++;

				}	
			
				curr = curr->next;
			}
		}
	}

	//printf("RESULT: %s",result);

	cse4589_print_and_log(result);
	cse4589_print_and_log("[STATISTICS:END]\n");
	fflush(stdout);

	free(result);		
}

void handle_LIST_command(int fd, char type){ //sorts clients by port and either sends the clients to the client to store 
//or display to the server 

	if (type == 's' || type == 'r'){
		cse4589_print_and_log("[LIST:SUCCESS]\n");
	}

	int num_online = 0;
	struct client* c1 = Clients;
	while (c1 != NULL){
		if (c1->online == 1){
			num_online ++;
		}
		c1 = c1->next;
	}

	int ports[num_online];

	int j = 0;
	struct client* curr1 = Clients;
	while(curr1 != NULL){
		if (curr1->online == 1){
			ports[j] = curr1->port;
			//printf("%d\n",ports[j]);
			j++;
		}
		curr1 = curr1->next;
	}

	qsort(ports,num_online,sizeof(int),compare);


	//send(sock_index,int_num,strlen(int_num),0);
	char* result = (char*)malloc(sizeof(char)*BUFFER_SIZE*5);
	memset(result,0,BUFFER_SIZE*5);

	int id = 1;
	for (int m = 0; m < num_online; m++){
		//printf("%d\n",ports[m]);
		if (ports[m] != -1){
			struct client* curr = Clients;
			while (curr != NULL){
				if (curr->online == 1 && curr->port == ports[m]){
					char msg_send[BUFFER_SIZE];
					memset(msg_send,'\0',sizeof(msg_send));

					curr->hostname[strlen(curr->hostname)] = '\0';
					curr->ip_addr[strlen(curr->ip_addr)] = '\0';

					if (type == 'r'){
						if (curr->online == 1){
								char* status = "logged-in";
								sprintf(msg_send,"%-5d%-35s%-8d%-8d%-8s\n",id,curr->hostname,curr->msgs_sent,curr->msgs_recv,status);
							}else if (curr->online == 0){
								char* status = "logged-out";
								sprintf(msg_send,"%-5d%-35s%-8d%-8d%-8s\n",id,curr->hostname,curr->msgs_sent,curr->msgs_recv,status);
							}
					}else{
						sprintf(msg_send,"%-5d%-35s%-20s%-8d\n",id,curr->hostname,curr->ip_addr,curr->port);
					}

					//send(sock_index,msg_send,strlen(msg_send),0);
					//if (type == 's'){
						//cse4589_print_and_log("%s",msg_send);
						//fflush(stdout);
					//}
										
					//"%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num
					//if (type == 'c'){
					strcat(result,msg_send);
					//}
					memset(msg_send,'\0',sizeof(msg_send));

					id++;

				}	
			
				curr = curr->next;
			}
		}
	}

	//printf("RESULT: %s",result);

	if (type == 'c'){
		strcat(result,"\0"); 
		send(fd,result,strlen(result),0);
		printf("[DEV MODE] sent");
	}else if (type == 's' || type == 'r'){
		cse4589_print_and_log(result);
		cse4589_print_and_log("[LIST:END]\n");
		fflush(stdout);
	}

	free(result);	
}


int build_substring_serv(char* cmd,int start,char* substr){ //get the ip and port strings in the LOGIN msg, sinc the commadn is of the form
//LOGIN <server_ip> <server_port>
    int k = 0;
    for (int i = start; i < strlen(cmd); i++){ //start at 5th index to start at all chars after LOGIN including space after LOGIN
        if (cmd[i] != ' ' && cmd[i] != '\0'){
            substr[k] = cmd[i];
            k = k + 1;
        }else{
            return i+1;
        }
    }
    return -1;
}



//////////////////////////////


void server_client_cmds(int fdaccept,char* buffer, char* port, char* client_extern_ip, char s_c, char* blocked_ip){
	//this function deals with all commands inputted into prompt on both server side and client side specified by the s_c char 
	//('s' for server, 'c' for client)

	if (strcmp(buffer, "AUTHOR\n") == 0){
		char* author = "I, breckenm, have read and understood the course academic integrity policy.\n";
		//printf("%s\n",author);
		cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
		cse4589_print_and_log("%s",author);
		cse4589_print_and_log("[AUTHOR:END]\n");

	}else if (strcmp(buffer, "PORT\n") == 0){
		if (s_c == 's'){
			cse4589_print_and_log("[PORT:SUCCESS]\n");
			cse4589_print_and_log("PORT:%s\n",port);
			cse4589_print_and_log("[PORT:END]\n");
		}
	}else if (strcmp(buffer, "NUM\n") == 0){
		printf("[DEV MODE] Number of clients: %d\n",num_clients);
	}else if (strcmp(buffer, "IP\n") == 0){

		//if server prompt inputted IP, return external server ip
    	char extern_ip[BUFFER_SIZE];
		get_external_ip(extern_ip);
		//printf("%s\n",extern_ip);

		cse4589_print_and_log("[IP:SUCCESS]\n");
		cse4589_print_and_log("IP:%s\n",extern_ip);
		cse4589_print_and_log("[IP:END]\n");
							
	}else if (strcmp(buffer, "LIST\n") == 0){
		if (s_c == 's'){

			handle_LIST_command(0, 's');
		}
								
	}else if (strcmp(buffer, "STATISTICS\n") == 0){
		if (s_c == 's'){
			handle_STATISTICS_command();
	
		}
	}else if (strncmp(buffer, "EXIT",4) == 0){
		if (s_c == 'c'){
			struct client* curr = Clients;
			while(curr != NULL){
				if (curr->fd == fdaccept){
					curr->online = 0;
				}
				curr = curr->next;
			}
		}
	}else if (strncmp(buffer,"BLOCKED",7) == 0){
		if (s_c == 's'){
			cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
			//printf("[DEV MODE] blocked: %s\n",blocked_ip);
			int num_online = 0;
			struct client* c1 = Clients;
			while (c1 != NULL){
				///printf("%s\n",c1->ip_addr);
				//printf("%s\n",blocked_ip);
				if (strncmp(c1->ip_addr,blocked_ip,strlen(c1->ip_addr)) == 0){
					//printf("match");
					struct backlog* curr2 = c1->blocked;
					while (curr2 != NULL){
						num_online ++;
						curr2 = curr2->next;
					}
				}
				c1 = c1->next;
			}

			int ports[num_online];

			int j = 0;
			struct client* curr3 = Clients;
			while(curr3 != NULL){
				if (strncmp(curr3->ip_addr,blocked_ip,strlen(curr3->ip_addr)) == 0){
					struct backlog* curr4 = curr3->blocked;
					while (curr4 != NULL){
						struct client* curr5 = Clients;
						while (curr5 != NULL){
							if (strncmp(curr5->ip_addr,curr4->message,13) == 0){
								//printf("good");
								ports[j] = curr5->port;
								j++;
							}
							curr5 = curr5->next;
						}
						curr4 = curr4->next;
					}
				}
				curr3 = curr3->next;
			}

			//printf("num_online: %d\n",num_online);

			qsort(ports,num_online,sizeof(int),compare);

			char* result = (char*)malloc(sizeof(char)*BUFFER_SIZE*5);
			memset(result,0,BUFFER_SIZE*5);

			int id = 1;
			for (int m = 0; m < num_online; m++){
				if (ports[m] != -1){
					struct client* curr = Clients;
					while (curr != NULL){
						if (curr->port == ports[m]){
							char msg_send[BUFFER_SIZE];
							memset(msg_send,'\0',sizeof(msg_send));

							curr->hostname[strlen(curr->hostname)] = '\0';
							curr->ip_addr[strlen(curr->ip_addr)] = '\0';

							sprintf(msg_send,"%-5d%-35s%-20s%-8d\n",id,curr->hostname,curr->ip_addr,curr->port);

							strcat(result,msg_send);
							memset(msg_send,'\0',sizeof(msg_send));

							id++;

						}	
					
						curr = curr->next;
					}
				}
			}

			cse4589_print_and_log(result);


			cse4589_print_and_log("[BLOCKED:END]\n");

			free(result);
		}
	
	}else{
		//printf("ECHOing it back to the remote host ... ");
		//if(send(fdaccept, buffer, strlen(buffer),0) == strlen(buffer)){
			//printf("Done!\n");
		if (s_c == 's'){
			
			cse4589_print_and_log("[LIST:ERROR]\n");
            cse4589_print_and_log("[LIST:END]\n");
		}
	}
}

/////////////////////////


int build_substring_ext(char* cmd,int start,char* substr){ //get the ip and port strings in the LOGIN msg, sinc the string is of the form
//LOGIN <server_ip>:<server_port>, break them up since they are seperated by : and msgs ends with '\0'
    int k = 0;
    for (int i = start; i < strlen(cmd); i++){ 
        if (cmd[i] != ':' && cmd[i] != '\0'){
            substr[k] = cmd[i];
            k = k + 1;
        }else{
            return i+1;
        }
    }
    return -1;
}

void client_init_login(char* buffer,char* client_extern_ip,char* client_extern_hostname,int sock_index){ //handles the initial login
//of a client, parses their external ip and hostname sent to server from client on first login to server and creates a client struct
//for them with their info and inserts it into linked list
	char client_port[BUFFER_SIZE];
	memset(client_port,'\0',sizeof(client_port));
	int a = build_substring_ext(buffer,0,client_extern_ip);
	int b = build_substring_ext(buffer,a,client_extern_hostname);
	build_substring_ext(buffer,b,client_port);

	
	int c_port_int = atoi(client_port);


	struct client* curr = Clients;
	int exists = 0;


	//make sure to send client all backlog messages while they were gone, else just send success if there are none
	char* backlog_msg = (char*)malloc(sizeof(char)*MAX_MSG_SIZE*10);
	memset(backlog_msg,'\0',MAX_MSG_SIZE*10);
								
	while (curr != NULL){
		if (strncmp(curr->hostname,client_extern_hostname,strlen(client_extern_hostname)) == 0 &&
			strncmp(curr->ip_addr,client_extern_ip,strlen(client_extern_ip)) == 0 && curr->port == c_port_int){ //check if client already exists

			//printf("stored fd: %d, current fd: %d\n", curr->fd, sock_index);
			exists = 1;
			//printf("[DEV MODE] exists\n");
			curr->fd = sock_index;
			curr->online = 1;

			//if any msgs in backlog, send them to client
			struct backlog* curr2 = curr->bl;
			while (curr2 != NULL){
				strcat(backlog_msg,curr2->message);
				if (curr2->broadcast == 1){
					cse4589_print_and_log("\n[RELAYED:SUCCESS]\n");
					cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s",curr2->sender,"255.255.255.255",curr2->msg);
					cse4589_print_and_log("[RELAYED:END]\n");
				}else{
					cse4589_print_and_log("\n[RELAYED:SUCCESS]\n");
					cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s",curr2->sender,curr2->receiver,curr2->msg);
					cse4589_print_and_log("[RELAYED:END]\n");
				}
				curr2 = curr2->next;
			}
			curr->bl = NULL;
			break;
		}
		curr = curr->next;
	}

	//printf("%d\n",exists);
	if (exists == 0){
		add_client(sock_index,c_port_int,client_extern_ip,client_extern_hostname); 
		//printf("[DEV MODE] added\n");
		num_clients++;
		char* success_msg = "Successfully logged in";
		send(sock_index, success_msg, strlen(success_msg),0);
	}else{
		if (strlen(backlog_msg) > 0){
			//printf("[DEV MODE] backlog: %s\n",backlog_msg);
			//cse4589_print_and_log("[RELAYED:SUCCESS]\n");
			//cse4589_print_and_log("[RELAYED:END]\n");

			send(sock_index,backlog_msg,strlen(backlog_msg),0);
		}else{
			char* success_msg = "Successfully logged in";
			send(sock_index, success_msg, strlen(success_msg),0);
		}
	}
								//printf("sent logged in");
	//cse4589_print_and_log("Echod back successful login\n");

	memset(client_port,'\0',sizeof(client_port));
	free(backlog_msg);

}

int is_blocked(char* client_extern_ip, struct client* curr){ //checks if client is blocked by other client
	struct backlog* curr2 = curr->blocked;
	while (curr2 != NULL){
		if (strncmp(client_extern_ip,curr2->message,strlen(client_extern_ip)) == 0){
			return 1;
		}
		curr2 = curr2->next;
	}
	return 0;
}

void relay_message(char* buffer, int sock_index, int broadcast, char* ip, int relayed){ //breaks sent message into other client ip and message to be sent to client,
// gets the fd of the other client and sends them the message if they are online or if they are offline stores the message in their backlog
	char* recipient_ip = (char*)malloc(sizeof(char)*BUFFER_SIZE);
	char* send_msg = (char*)malloc(sizeof(char)*strlen(buffer)+1);
	memset(recipient_ip,0,BUFFER_SIZE);
	memset(send_msg,0,strlen(buffer)+1);

	//("%s\n",buffer);

	struct client* cur = Clients;
	while (cur != NULL){ //client who wants to send message, based off their sock_index file descriptor
		if (cur->fd == sock_index){
			break;
		}
		cur = cur->next;
	}

	if (broadcast == 1){
		build_substring_ext(buffer,2,send_msg);
		memcpy(recipient_ip,ip,strlen(ip));
	}else{
		int a = build_substring_ext(buffer,1,recipient_ip);
		build_substring_ext(buffer,a,send_msg);
		recipient_ip[strlen(recipient_ip)] = '\0';
		send_msg[strlen(send_msg)] = '\0';
		//printf("[DEV MODE] recipient: %s\n",recipient_ip);
		//printf("[DEV MODE] message: %s\n",send_msg);
	}


	int recipient_fd;

	char* whole_message = (char*)malloc(sizeof(char)*strlen(buffer)+1+BUFFER_SIZE);
	memset(whole_message,'\0',strlen(buffer)+1+BUFFER_SIZE);
	//strcat(whole_message,"\n[RECIEVED:SUCCESS]\n");

	//char* temp_message = (char*)malloc(sizeof(char)*BUFFER_SIZE+1);
	//memset(temp_message,'\0',BUFFER_SIZE+1);
	//sprintf(temp_message,"msg from:%s\n[msg]:%s",cur->ip_addr,send_msg);
	//strcat(temp_message,"[RECIEVED:END]\n");
	//strcat(whole_message,temp_message);

	sprintf(whole_message,"msg from:%s\n[msg]:%s",cur->ip_addr,send_msg);

	struct client* curr = Clients;
	while (curr != NULL){ //find the client with matching ip
		if (strncmp(recipient_ip,curr->ip_addr,strlen(recipient_ip)) == 0){
			int blocked = is_blocked(cur->ip_addr,curr);
			if (blocked == 1){
				recipient_fd = -1;
				printf("[DEV MODE] blocked\n");
				break;
			}

			//make sure to add 1 to msgs_sent to sender client and add 1 to msgs_recv to recpeient client if the
			//sender client is not blocked

			if (curr->online == 0 && blocked != 1){ //if client is offline, insert at end of backlog ll
				printf("[DEV MODE] offline: storing in backlog\n");
				struct backlog* new = (struct backlog*)malloc(sizeof(struct backlog));
				new->message = (char*)malloc(sizeof(char)*strlen(whole_message)+1+BUFFER_SIZE);
				memset(new->message,0,strlen(whole_message)+1+BUFFER_SIZE); //+1 for null terminator
				strcat(new->message,"[RECEIVED:SUCCESS]\n");
				strcat(new->message,whole_message);
				strcat(new->message,"[RECEIVED:END]\n"); //POSSIBLY REMOVE NEWLINE CHARACTER
				//memcpy(new->message,whole_message,strlen(whole_message));
				new->next = NULL;
				//curr->bl = new;

				curr->msgs_recv = curr->msgs_recv + 1;
				cur->msgs_sent = cur->msgs_sent + 1;

				if (broadcast == 1){
					new->broadcast = 1;
				}else{
					new->broadcast = 0;
				}

				//fix this part
				if (curr->bl == NULL){
					curr->bl = new;
				}else{
					struct backlog* cur3 = curr->bl;
					while (cur3->next != NULL){
						cur3 = cur3->next;
					}
					cur3->next = new;
				}

				new->receiver = (char*)malloc(sizeof(char)*BUFFER_SIZE);
				memset(new->receiver,0,BUFFER_SIZE);
				memcpy(new->receiver,recipient_ip,strlen(recipient_ip));
				new->sender = (char*)malloc(sizeof(char)*BUFFER_SIZE);
				memset(new->sender,0,BUFFER_SIZE);
				memcpy(new->sender,cur->ip_addr,strlen(cur->ip_addr));
				new->msg = (char*)malloc(sizeof(char)*BUFFER_SIZE);
				memset(new->msg,0,BUFFER_SIZE);
				memcpy(new->msg,send_msg,strlen(send_msg));

				recipient_fd = -1;
				break;
			}else if (curr->online == 1 && blocked != 1){
				printf("[DEV MODE] %s is online and not blocked\n", curr->ip_addr);
				recipient_fd = curr->fd; //get their file descriptor to send to them
				curr->msgs_recv = curr->msgs_recv + 1;
				cur->msgs_sent = cur->msgs_sent + 1;
				break;
			}else if (blocked == 1){
				recipient_fd = -1;
				break;
			}
		}
		curr = curr->next;
	}
	//if (strncmp(recipient_ip,cur->ip_addr,strlen(recipient_ip)) == 0){
		//recipient_fd = -1;
		//printf("[DEV MODE] cant send to self\n");
	//}
	if (recipient_fd != -1){
		char* complete_msg = (char*)malloc(sizeof(char)*strlen(whole_message)+1+BUFFER_SIZE);
		memset(complete_msg,'\0',strlen(whole_message)+1+BUFFER_SIZE);
		strcat(complete_msg,"[RECEIVED:SUCCESS]\n");
		strcat(complete_msg,whole_message);
		strcat(complete_msg,"[RECEIVED:END]\n");

		send(recipient_fd,complete_msg,strlen(complete_msg),0); //send message to client
		if (relayed == 0){
			if (broadcast == 1){
				cse4589_print_and_log("[RELAYED:SUCCESS]\n");
				cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s",cur->ip_addr,"255.255.255.255",send_msg);
				cse4589_print_and_log("[RELAYED:END]\n");	
			}else{
				cse4589_print_and_log("[RELAYED:SUCCESS]\n");
				cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s",cur->ip_addr,recipient_ip,send_msg);
				cse4589_print_and_log("[RELAYED:END]\n");
			}
		}

		free(complete_msg);
	}else{
		printf("[DEV MODE] not sent to client");
	}

	free(recipient_ip);
	free(send_msg);
	free(whole_message);
	//free(temp_message);
	
	fflush(stdout);

}

//////////////////////////////

int run_server(char* port){

	int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in client_addr;
	fd_set master_list, watch_list;

	//cse4589_init_log(port);

	server_socket = socket_bind_listen(port);
	if(listen(server_socket, 5) < 0){
		perror("Unable to listen on port");
	}

	
	/* ---------------------------------------------------------------------------- */
	
	/* Zero select FD sets */
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);
	
	/* Register the listening socket */
	FD_SET(server_socket, &master_list);
	/* Register STDIN */
	FD_SET(STDIN, &master_list);
	
	head_socket = server_socket;

	
	while(TRUE){
		memcpy(&watch_list, &master_list, sizeof(master_list));
		
		//printf("\n[PA1-Server@CSE489/589]$ ");
		printf("\n[PA1-Server@CSE489/589]$ ");
		fflush(stdout);
		
		/* select() system call. This will BLOCK */
		selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
		if(selret < 0)
			perror("select failed.");
		
		/* Check if we have sockets/STDIN to process */
		if(selret > 0){
			/* Loop through socket descriptors to check which ones are ready */
			for(sock_index=0; sock_index<=head_socket; sock_index+=1){
				
				if(FD_ISSET(sock_index, &watch_list)){
					
					/* Check if new command on STDIN */
					if (sock_index == STDIN){
						char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);
						
						memset(cmd, '\0', CMD_SIZE);
						if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
						
						//printf("\nI got: %s\n", cmd);
						//cse4589_print_and_log(cmd);
						//fflush(stdout);

						if (strncmp(cmd,"EXIT",4) == 0){
							close(server_socket);
							exit(1);
						}

						if (strncmp(cmd,"BLOCKED",7) == 0){
							char blocked_ip[BUFFER_SIZE];
							memset(blocked_ip,'\0',sizeof(blocked_ip));
							build_substring_ext(cmd,8,blocked_ip);
							//printf("[DEV MODE] blocked: %s\n",blocked_ip);
							server_client_cmds(0,cmd,port," ", 's',blocked_ip);
						}else{
							server_client_cmds(fdaccept,cmd,port," ",'s',""); //analyze commands for 's' since it was 
							//command inputted by server prompt, dont need client info since not dealing with client input
						}
						
						//Process PA1 commands here ...
						
						free(cmd);
					}
					/* Check if new client is requesting connection */
					else if(sock_index == server_socket){
						caddr_len = sizeof(client_addr);
						fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
						if(fdaccept < 0)
							perror("Accept failed.");
						
						//printf("\nRemote Host connected!\n");  
						printf("\n[DEV MODE] Remote Host connected!\n");
						fflush(stdout);                      
						
						/* Add to watched socket list */
						FD_SET(fdaccept, &master_list);
						if(fdaccept > head_socket) head_socket = fdaccept;


					}
					/* Read from existing clients */
					else{
						/* Initialize buffer to receieve response */
						char *buffer = (char*) malloc(sizeof(char)*MAX_MSG_SIZE);
						memset(buffer, '\0', MAX_MSG_SIZE);
						
						if(recv(sock_index, buffer, MAX_MSG_SIZE, 0) <= 0){
							struct client* curr = Clients; //if the client quits without exiting, set their
							//online status to 0 since they are now offline
							while (curr != NULL){
								if (curr->fd == sock_index){
									curr->online = 0;
								}
								curr = curr->next;
							}
							close(sock_index);
							//printf("Remote Host terminated connection!\n"); //when client kill conn, remove from linked list
							printf("[DEV MODE] Remote Host terminated connection!\n");

							/* Remove from watched list */
							FD_CLR(sock_index, &master_list);
						}
						else {
							//Process incoming data from existing clients here ...
							int count = 0;
							int detect_at = 0;
							for (int m = 0; m < strlen(buffer); m++){ //if client sent message with 6 dots know it holds
							//ip and hostname
								if (buffer[m] == '.'){
									count = count + 1;
								}
								if (buffer[m] == '@'){ //detects sent message
									detect_at = 1;
								}
							}
							if (buffer[0] == 'B'){
								detect_at = 2;
							}
							if (buffer[0] == 'U'){
								detect_at = 3;
							}
							if (buffer[0] == 'V' && buffer[1] == ':'){
								detect_at = 4;
							}

							char client_extern_ip[BUFFER_SIZE]; //stores the clients external ip that the client sent to server
							char client_extern_hostname[BUFFER_SIZE]; //stores the clients external hostname that client sent to server

							memset(client_extern_hostname,'\0',sizeof(client_extern_hostname));
							memset(client_extern_ip,'\0',sizeof(client_extern_ip));


							if (detect_at == 1){//if '@' detected in sent message, contains other client ip and message to send to client
								
								//printf("%s\n",buffer);

								int s_c = 0;
								//printf("[DEV MODE] %s\n",copy);
								for (int i = 0; i < strlen(buffer); i++){
									//printf("%c",buffer[i]);
									if (buffer[i] == '\\'){
										s_c = 1;
										break;
									}
								}
                				//char* tok = strtok(copy,"\\");
								//printf("[DEV MODE] %s\n",tok);
								if (s_c == 0){
									//printf("[DEV MODE] no backslash\n");
									relay_message(buffer,sock_index,0,"",0);
								}else{
                					//printf("%s\n",tok);
									char* tok = strtok(buffer,"\\");
									int s = 6;
                					while (tok != NULL){
                    					//printf("TOK: %s\n",tok);
										if (strlen(tok) == 0 || (tok[0] == 'n' && strlen(tok) == 1)){
											break;
										}
                    					char* send_ip = (char*)malloc(sizeof(char)*BUFFER_SIZE);
                    					char* send_msg = (char*)malloc(sizeof(char)*512);
                    					memset(send_ip,0,BUFFER_SIZE);
                    					memset(send_msg,0,512);
                    					int k = build_substring_serv(tok,s,send_ip);
										//printf("k=%d\n",k);
                    					s=6;
                    					//build_substring(msg,k,send_msg);
                    					int l = 0;
                    					while (tok[k] != '\0'){
                        					if (l > 256){
                            					break;
                        					}
                        					send_msg[l] = tok[k];
                        					l++;
                        					k++;
                    					}
										//printf("[DEV MODE] MSG: %s\n",send_msg);
										//printf("[DEV MODE] IP: %s\n",send_ip);
                    					if (strlen(send_msg) > 256){
                        					cse4589_print_and_log("[SEND:ERROR]\n");
                        					cse4589_print_and_log("[SEND:END]\n");
                        					//printf("%s\n",send_msg);
                        					free(send_msg);
                        					free(send_ip);
                    					}else{
                        					int msg_len = strlen(send_msg) + 1;
            
                        					char result[BUFFER_SIZE+msg_len];
                        					memset(result,0,sizeof(result));

                        					strcat(result,"@");
                        					strcat(result,send_ip);
                        					strcat(result,":");
                        					strcat(result,send_msg);
                        					strcat(result,"\n");
                        					strcat(result,"\0");
											//printf("[DEV MODE] RESULT: %s\n",result);
                        					//put '@' character at end of message to tell server that this is to be sent to client with specified ip
                        					//and to differentiate it from EXIT or LOGIN message
                        					//strcat(result,"\0");
                        					result[strlen(result)+1] = '\0';

                        					//printf("[DEV MODE] %s\n",result);
                        					//printf("[DEV MODE] size:%d\n",sizeof(result));
                        					//printf("[DEV MODE] strlenen:%d\n",strlen(result));

                        					//send(server,"@",1,0);
                        					//printf("sending...\n");
                        					//send(server,result,sizeof(result),0);

                        					free(send_msg);
                        					free(send_ip);
											relay_message(result,sock_index,0,"",0);
                    					}
                    					tok = strtok(NULL,"\\");
										//relay_message(result,sock_index,0,"",0);
									}
								}
								
							}else if(detect_at == 2){
								struct client* curr = Clients;
								while (curr != NULL){
									if (curr->fd == sock_index){
										char block_ip[BUFFER_SIZE];
										memset(block_ip,'\0',sizeof(block_ip));
										build_substring_ext(buffer,1,block_ip);
										block_ip[strlen(block_ip)] = '\0';
										printf("[DEV MODE] blocking: %s\n",block_ip);
										struct backlog* new = (struct backlog*)malloc(sizeof(struct backlog));
										new->message = (char*)malloc(sizeof(char)*BUFFER_SIZE);
										memset(new->message,0,BUFFER_SIZE);
										memcpy(new->message,block_ip,strlen(block_ip));
										new->next = NULL;
										new->next = curr->blocked;
										curr->blocked = new;
										break;
									}
									curr = curr->next;
								}
							
							}else if(detect_at == 3){
								struct client* curr = Clients;
								while (curr != NULL){
									if (curr->fd == sock_index){
										char block_ip[BUFFER_SIZE];
										memset(block_ip,'\0',sizeof(block_ip));
										build_substring_ext(buffer,1,block_ip);
										block_ip[strlen(block_ip)] = '\0';
										printf("[DEV MODE] unblocking: %s\n",block_ip);
										struct backlog* curr2 = curr->blocked;
										while (curr2 != NULL){
											if (strncmp(curr2->message,block_ip,strlen(block_ip)) == 0){
												printf("[DEV MODE] removing from blocked list\n");
												memcpy(curr2->message,"\0",strlen(curr2->message));
												break;
											}
											curr2 = curr2->next;
										}
										break;
									}
									curr = curr->next;
								}
							
							}else if (detect_at == 4){
								struct client* curr = Clients;
								int r = 0;
								while (curr != NULL){
									if (curr->fd != sock_index){
										relay_message(buffer,sock_index,1,curr->ip_addr,r);
										r = 1;
									}
									//r = 1;
									curr = curr->next;
								}
							
							
							}else if (count == 6){ //if 6 dots in message from client, sent the combined external ip and external hostname to create
							//client struct to insert into ll
								
								client_init_login(buffer,client_extern_ip,client_extern_hostname,sock_index);


							}else if(count == 3){ //if only 3, only was sent client ip, which i made only happen when client kills connection
							

								memcpy(client_extern_ip,buffer,strlen(buffer));
								//cse4589_print_and_log("Going to remove client w ip: %s\n",client_extern_ip);
								
								server_client_cmds(sock_index,"EXIT",port,client_extern_ip,'c',"");//remove client from linked list where client hostname matches hostname sent from client
								close(sock_index);
								//printf("Remote Host terminated connection!\n"); //when client kill conn, remove from linked list
								printf("[DEV MODE] Remote Host terminated connection!\n");

								FD_CLR(sock_index, &master_list);
								
							}else if(strncmp(buffer,"LIST",4) == 0){ //isolated LIST from function that handles client commands since
							//its been causing me lots of issues

								handle_LIST_command(sock_index, 'c');
							}else{
								//after connection with client and client sends ip and hostname we store, grab clients port and
								//run server_client_cmds with 'c' and all client info to analyze all future commands sent from client
								//int c_port = ntohs(client_addr.sin_port);
								int c_port;
								struct client* curr = Clients;
								while (curr != NULL){
									if (sock_index == curr->fd){
										c_port = curr->port;
									}
									curr = curr->next;
								}

								server_client_cmds(sock_index,buffer,port,client_extern_ip,'c',"");
							
							}
	

							//fflush(stdout);
						}
						
						free(buffer);
					}
				}
			}
		}
	}

	return 0;
}
