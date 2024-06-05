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
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>

#include "../include/logger.h"
//#include "../include/client.h"

#define TRUE 1
#define MSG_SIZE 256
#define BUFFER_SIZE 256


int logged_in = 0;  //global var that keeps track if client logged into server so they cant do it again causing weird issues
char list[BUFFER_SIZE*2];


int is_upper(char* cmd){ //this function makes sure that the input is all uppercase 
    for (int i = 0; i < strlen(cmd); i++){
        if ((cmd[i] < 'A' || cmd[i] > 'Z') && (cmd[i] != '\0' && cmd[i] != '\n')){
            return 1;
        }
    }
    return 0;
}

int build_substring(char* cmd,int start,char* substr){ //get the ip and port strings in the LOGIN msg, sinc the commadn is of the form
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


int connect_to_host(char* s_ip, char* s_port, int client_s){ //this function connects to the server at a certain port and ip
    struct addrinfo* info;
    struct addrinfo hints;

    /*Below sets the ai_family
    and ai_socktype of the hints structure and sets everything else to 0 
    since I know the ai_family and ai_socktype so they will be used to get 
    all other values in the info struct needed to connect when passing it 
	into getaddrinfo
*/
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(s_ip,s_port, &hints, &info) != 0){ 
    /* //if problem continues, hardcode in address 128.205.36.46 and port 4322
        
        Since there is only one address in this instance, the addrinfo pointer will point to an
        addrinfo struct with all the necessary information and it will be the only
        struct in the linked list
    */
        perror("getaddrinfo failed");
        exit(-1);
    } 
    
    //int s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    //if(s < 0){
        //perror("Failed to create socket");
        //exit(-1);
    //}

    if (connect(client_s, info->ai_addr, info->ai_addrlen) < 0){
        perror("Connect failed");
        exit(-1);
    }

    //freeaddrinfo(info);
    return client_s;
}

void get_external_ip_client(char* extern_ip,char* extern_hn,int* port){ //gets the external ip and hostname of the client
//works same was as getting external ip for server
//will send this data to server immediately upon successful connection
	struct sockaddr_in ser_addr;
	memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(53); // DNS port
    ser_addr.sin_addr.s_addr = inet_addr("8.8.8.8");

	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	connect(udp_socket,(struct sockaddr*)&ser_addr,sizeof(ser_addr));

	int ser_addr_size = sizeof(ser_addr);
	getsockname(udp_socket,(struct sockaddr*)&ser_addr,&ser_addr_size);


    char extern_hn_cpy[BUFFER_SIZE];
    getnameinfo((struct sockaddr*)&ser_addr,sizeof(ser_addr),extern_hn_cpy,sizeof(extern_hn_cpy),NULL,0,0); //get external hostname
    memcpy(extern_hn,extern_hn_cpy,strlen(extern_hn_cpy));

	inet_ntop(AF_INET,&(ser_addr.sin_addr),extern_ip,BUFFER_SIZE);  //get external ip 

    *port = ntohs(ser_addr.sin_port); //get external port

	close(udp_socket);
}

void get_client_list(int server){ //gets the list of clients from the server and stores it in the global list string
    memset(list,0,sizeof(list));


    char* list_msg = "LIST";    
    send(server, list_msg, strlen(list_msg), 0);
	
    char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE*5);
	//memset(buffer, '\0', BUFFER_SIZE*2);  
    memset(buffer,0,BUFFER_SIZE*5);

    //printf("%s\n",buffer);

    if (recv(server,buffer,BUFFER_SIZE*5,0) > 0){
        //printf(buffer);
        buffer[strlen(buffer)] = '\0';
        memcpy(list,buffer,strlen(buffer));
        //printf("%s: list", list);

    }        

    //printf(buffer);

    fflush(stdout);
    free(buffer);   
}


void run_client(char* s_ip, char* s_port){

    int server;
    char extern_ip[BUFFER_SIZE]; //store the external ip of the client
    char extern_hn[BUFFER_SIZE]; //store the external hostname of the client
    memset(extern_ip,0,sizeof(extern_ip)); //always initialize strings to contain all 0s or null terminators, learned this the hard way
    memset(extern_hn,0,sizeof(extern_hn));

    int extern_port; //store the external port of the client


    char* in_s_ip = (char*)malloc(sizeof(char)*BUFFER_SIZE); //store the ip of the server specificed in lOGIN command
    char* in_s_port = (char*)malloc(sizeof(char)*BUFFER_SIZE); //store port server is liseting on sepcificed in lOGIN command
    memset(in_s_ip,0,BUFFER_SIZE);
    memset(in_s_port,0,BUFFER_SIZE);


    get_external_ip_client(extern_ip,extern_hn,&extern_port); //get client external ip and hostname



    struct addrinfo* info;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL,s_port, &hints, &info) != 0){ 
        perror("getaddrinfo failed");
        exit(-1);
    } 
    
    int client_s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(client_s < 0){
        perror("Failed to create socket");
        exit(-1);
    }

    if (bind(client_s, info->ai_addr, info->ai_addrlen) < 0){
        perror("Bind failed");
    }

    //start loop of listening for commands from prompt, sending to server, and listeing for responses from server whil
    //connection is open
	while(TRUE){
       
        printf("\n[PA1-Client@CSE489/589]$ ");
		fflush(stdout);
        char *msg = (char*) malloc(sizeof(char)*1000);
        memset(msg, '\0', 1000);

        int recieved = 0;
        
        if (logged_in == 1 || logged_in == 0){
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server, &readfds);
            FD_SET(0, &readfds); 

            int server_act = select(server+1, &readfds, NULL, NULL, NULL);

            int no_input = 1;
            if (server_act < 0){
                perror("Select failed");
                exit(-1);
            }
            if (FD_ISSET(0, &readfds)){
                no_input = 0;
                //char *msg = (char*) malloc(sizeof(char)*800000);
		        memset(msg, '\0', 1000);
		        if(fgets(msg, 999, stdin) == NULL) //Mind the newline character that will be written to msg
			        exit(-1);
            }
            if (FD_ISSET(server, &readfds) && no_input == 1){
                char *buffer = (char*) malloc(sizeof(char)*1000);    
                memset(buffer, '\0', 1000);
                recv(server, buffer, 1000, 0);
                //cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                //printf("\n");
                cse4589_print_and_log("%s", buffer);
                //cse4589_print_and_log("[RECEIVED:END]\n");
                //printf("[PA1-Client@CSE489/589]$ ");
                recieved = 1;
                fflush(stdout);
                free(buffer);
            }

        }
        //////////


		//printf("\n[PA1-Client@CSE489/589]$ ");
		//fflush(stdout);
		
		//char *msg = (char*) malloc(sizeof(char)*800000);
		//memset(msg, '\0', 800000);
		//if(fgets(msg, 800000-1, stdin) == NULL) //Mind the newline character that will be written to msg
			//exit(-1);

        
        //if(is_upper(msg) == 1){ //always make sure upper case input
            //exit(-1);
        //}

        if (strncmp(msg,"LOGIN",5) == 0 && logged_in == 0){ //if LOGIN cmd inputted in client prompt, connect using the 
        //inputted ip and port
            logged_in = 1;
            int k = build_substring(msg,6,in_s_ip);
            build_substring(msg,k,in_s_port);
            in_s_port[strlen(in_s_port)-1] = '\0';
            //printf("%s: in_s_port\n",in_s_port);
            //printf("%s: in_s_ip\n", in_s_ip);

    	    server = connect_to_host(in_s_ip, in_s_port,client_s); //change to input vars when fixed bug
            char result[BUFFER_SIZE];
            memset(result,'\0',sizeof(result));
            
            strcat(result,extern_ip);
            strcat(result,":");
            strcat(result,extern_hn);
            strcat(result,":");
            strcat(result,s_port);
            strcat(result,"\0");

            //printf("%s\n",result);
            
            send(server,result,sizeof(result),0);
            
            char *buffer1 = (char*) malloc(sizeof(char)*1000);
		    memset(buffer1, '\0', 1000);
		
		    recv(server, buffer1, 1000, 0);
			if (strncmp(buffer1,"Successfully logged in",22) == 0){
                printf("[DEV MODE] %s",buffer1);
            }else{
                //cse4589_print_and_log("[RECIEVED:SUCCESS]\n");
                cse4589_print_and_log("%s",buffer1);
                //cse4589_print_and_log("[RECIEVED:END]\n");
            }

            fflush(stdout);

            get_client_list(server);

            free(buffer1);
        
        }else if (strcmp(msg,"EXIT\n") == 0){
            if (logged_in == 1){
                send(server, extern_ip, strlen(extern_ip), 0);
            }
            //fflush(stdout);
            free(msg);
            free(in_s_ip);
            free(in_s_port);
            freeaddrinfo(info);
            exit(0);
        }else if(strcmp(msg,"LOGOUT\n") == 0 && logged_in==1){
            if (logged_in == 1){
                send(server, extern_ip, strlen(extern_ip), 0);
                logged_in = 0;
                //free(msg);
                //free(in_s_ip);
                //free(in_s_port);    
            }
        }else if(strcmp(msg,"PORT\n") == 0){
            cse4589_print_and_log("[PORT:SUCCESS]\n");
		    cse4589_print_and_log("PORT:%s\n",s_port);
            cse4589_print_and_log("[PORT:END]\n");
            
        }else if(strncmp(msg,"SEND",4) == 0 && logged_in==1){ //&& make sure logged_in == 1
            int nl = 0;
            for (int i = 0; i < strlen(msg)-1; i++){
                if (msg[i] =='\\' && msg[i+1] == 'n'){
                    //printf("ya its that");
                    nl = 1;
                    break;
                }
            }
            //printf("%d\n",nl);
            //if (msg[strlen(msg)-1] == '\n'){
                //msg[strlen(msg)-1] = '\0';
            //}
            if (nl == 1){

                msg[strlen(msg)-1] = '\0';
                
                char* send_msg = (char*)malloc(sizeof(char)*1000);
                memset(send_msg,0,1000);
                strcat(send_msg,"@");
                strcat(send_msg,msg);
                strcat(send_msg,"\0");

                send(server,send_msg,strlen(send_msg),0);
                free(send_msg);
            }else{
                char* send_ip = (char*)malloc(sizeof(char)*BUFFER_SIZE);
                char* send_msg = (char*)malloc(sizeof(char)*512);
                memset(send_ip,0,BUFFER_SIZE);
                memset(send_msg,0,512);
                int k = build_substring(msg,5,send_ip);
                //build_substring(msg,k,send_msg);
                int l = 0;
                while (msg[k] != '\0'){
                    if (l > 256){
                        break;
                    }
                    send_msg[l] = msg[k];
                    l++;
                    k++;
                }
                if (strlen(send_msg) > 256){
                    cse4589_print_and_log("[SEND:ERROR]\n");
                    cse4589_print_and_log("[SEND:END]\n");
                    //printf("%s\n",send_msg);
                    free(send_msg);
                    free(send_ip);
                }else{
                    int msg_len = strlen(send_msg) + 1;
            
                    char result[512];
                    memset(result,0,sizeof(result));

                    strcat(result,"@");
                    strcat(result,send_ip);
                    strcat(result,":");
                    strcat(result,send_msg);
                    strcat(result,"\0");
                    //put '@' character at end of message to tell server that this is to be sent to client with specified ip
                     //and to differentiate it from EXIT or LOGIN message
                    //strcat(result,"\0");
                    result[strlen(result)+1] = '\0';

                    //printf("[DEV MODE] %s\n",result);
                    //printf("[DEV MODE] size:%d\n",sizeof(result));
                    //printf("[DEV MODE] strlenen:%d\n",strlen(result));

                    //send(server,"@",1,0);
                    send(server,result,sizeof(result),0);

                    free(send_msg);
                    free(send_ip);
                }
            }
    
        }else if(strncmp(msg,"BROADCAST",9) == 0 && logged_in == 1){
            char* send_msg = (char*)malloc(sizeof(char)*512);
            memset(send_msg,0,512);
            int k = 10;
            //build_substring(msg,k,send_msg);
            int l = 0;
            while (msg[k] != '\0'){
                if (l > 256){
                    break;
                }
                send_msg[l] = msg[k];
                l++;
                k++;
            }
            if (strlen(send_msg) > 256){
                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
                printf("%s\n",send_msg);
                free(send_msg);
            }else{
                int msg_len = strlen(send_msg) + 1;
            
                char result[BUFFER_SIZE+msg_len];
                memset(result,0,sizeof(result));

                strcat(result,"V:");
                strcat(result,send_msg);
                strcat(result,"\0");
                //put '@' character at end of message to tell server that this is to be sent to client with specified ip
                //and to differentiate it from EXIT or LOGIN message
                //strcat(result,"\0");
                result[strlen(result)+1] = '\0';

                printf("[DEV MODE] %s\n",result);
                printf("[DEV MODE] size:%d\n",sizeof(result));
                printf("[DEV MODE] strlenen:%d\n",strlen(result));

                //send(server,"@",1,0);
                send(server,result,sizeof(result),0);
            
                free(send_msg);
            }        
        }else if(strcmp(msg,"REFRESH\n") == 0 && logged_in==1){ //make it so 
        //LIST is called whenever client logs in and stores result to save for later when they invoke LIST
        //REFRESH gets current updated clients list
            if (logged_in == 1){
                cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                cse4589_print_and_log("[REFRESH:END]\n");
                memset(list,0,sizeof(list));
                get_client_list(server);
                //cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                //cse4589_print_and_log("[REFRESH:END]\n");
            }else{
                cse4589_print_and_log("[REFRESH:ERROR]\n");
                cse4589_print_and_log("[REFRESH:END]\n");
            }

        }else if(strcmp(msg,"LIST\n") == 0 && logged_in == 1){
            if (logged_in == 1){
                cse4589_print_and_log("[LIST:SUCCESS]\n");
                cse4589_print_and_log("%s",list); 
                cse4589_print_and_log("[LIST:END]\n");
            }else{
                cse4589_print_and_log("[LIST:ERROR]\n");
                cse4589_print_and_log("[LIST:END]\n");
            }
        
        }else if(strcmp(msg,"AUTHOR\n") == 0){

            char* author = "I, breckenm, have read and understood the course academic integrity policy.\n";
		    cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
		    cse4589_print_and_log("%s",author);
		    cse4589_print_and_log("[AUTHOR:END]\n");

        }else if(strcmp(msg,"IP\n") == 0){

            cse4589_print_and_log("[IP:SUCCESS]\n");
		    cse4589_print_and_log("IP:%s\n",extern_ip);
		    cse4589_print_and_log("[IP:END]\n");

        }else if(strncmp(msg,"BLOCK",5) == 0 && logged_in == 1){
            char* block_ip = (char*)malloc(sizeof(char)*BUFFER_SIZE);
            memset(block_ip,0,BUFFER_SIZE);
            int k = build_substring(msg,6,block_ip);
            block_ip[strlen(block_ip)] = '\0';
            char result[BUFFER_SIZE];
            memset(result,0,sizeof(result));
            strcat(result,"B");
            strcat(result,block_ip);
            strcat(result,"\0");
            send(server,result,sizeof(result),0);
            printf("[DEV MODE] %s\n",result);
            free(block_ip);
        
        }else if(strncmp(msg,"UNBLOCK",7) == 0 && logged_in == 1){
            char* unblock_ip = (char*)malloc(sizeof(char)*BUFFER_SIZE);
            memset(unblock_ip,0,BUFFER_SIZE);
            int k = build_substring(msg,8,unblock_ip);
            unblock_ip[strlen(unblock_ip)] = '\0';
            char result[BUFFER_SIZE];
            memset(result,0,sizeof(result));
            strcat(result,"U");
            strcat(result,unblock_ip);
            strcat(result,"\0");
            send(server,result,sizeof(result),0);
            printf("[DEV MODE] %s\n",result);
            free(unblock_ip);
        
        }else if(strcmp(msg,"GET\n") == 0){
            char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
		    memset(buffer, '\0', BUFFER_SIZE);

            if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){
			    printf("[DEV MODE] Message recieved: %s", buffer);
			    fflush(stdout);
            }

            free(buffer);
        }else{
            if (recieved == 0){
                //cse4589_print_and_log("[COMMAND:ERROR]\n");
                //cse4589_print_and_log("[COMMAND:END]\n");
                printf("[COMMAND:ERROR]\n");
                printf("[COMMAND:END]\n");
            }
        }
        //fflush(stdout);
		free(msg);
	}

    free(in_s_ip);
    free(in_s_port);

    //fflush(stdout);
}
