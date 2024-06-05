/**
*
* UBIT: breckenm || breckenm@buffalo.edu
* 
* References 
* @ref PA1 Demo Code
* @ref Beej's Guide to Network Programming
*/


#ifndef SERVER_H_
#define SERVER_H_

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256
#define MAX_CLIENTS 20
#define MAX_MSGS 10
#define MAX_MSG_SIZE 1000

struct backlog{
	char* message;
	char* sender;
	char* receiver;
	char* msg;
	int broadcast;
	struct backlog* next;
};

struct client{
	int port;
	int fd;
	int online;
	int msgs_recv;
	int msgs_sent;
	char hostname[BUFFER_SIZE];
	char ip_addr[BUFFER_SIZE];
	struct backlog* bl; //will be an array of size MAX_MSGS and hold strings of max size BUFFER_SIZE
	struct client* next; 
	struct backlog* blocked;
};

void add_client(int c_fd,int port, char* ip, char* hostname);
int socket_bind_listen(char* port);
int compare(const void * a, const void * b);
void handle_STATISTICS_command();
void handle_LIST_command(int fd, char type);
void server_cmds(char* cmd);
void get_external_ip(char* extern_ip);
int build_substring_serv(char* cmd,int start,char* substr);
void server_client_cmds(int fdaccept,char* buffer, char* port, char* client_extern_ip,char s_c, char* blocked_ip);
int build_substring_ext(char* cmd,int start,char* substr);
void client_init_login(char* buffer,char* client_extern_ip,char* client_extern_hostname,int sock_index);
int is_blocked(char* client_extern_ip, struct client* curr);
void relay_message(char* buffer, int sock_index,int broadcast,char* ip, int relayed);
int run_server(char* port);

#endif