/**
*
* UBIT: breckenm || breckenm@buffalo.edu
* 
* References 
* @ref PA1 Demo Code
* @ref Beej's Guide to Network Programming
*/


#ifndef CLIENT_H_
#define CLIENT_H_

int is_upper(char* cmd);
int build_substring(char* cmd,int start,char* substr);
int connect_to_host(char* s_ip, char* s_port,int client_s);
void get_external_ip_client(char* extern_ip,char* extern_hn,int* port);
void get_client_list(int server);
void run_client(char* s_ip, char* s_port);

#endif