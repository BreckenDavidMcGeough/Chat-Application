/**
 * @assignment1
 * @author  breckenm@buffalo.edu
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/client.h"
#include "../include/server.h"


/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	if (argc < 3){
		cse4589_print_and_log("[LIST:ERROR]\n");
        cse4589_print_and_log("[LIST:END]\n");
	}else{
		int i = 0;
		int non_num = 0;
		while (argv[2][i] != '\0'){
			if (argv[2][i] < '0' || argv[2][i] > '9'){
				non_num = 1;
			}
			i++;
		}

		if (non_num == 1){
			cse4589_print_and_log("[LIST:ERROR]\n");
        	cse4589_print_and_log("[LIST:END]\n");
		}else{

			if (argv[1][0] == 'c'){
				run_client("127.0.0.1",argv[2]);
			}
			if (argv[1][0] == 's'){
				run_server(argv[2]);
			}
		}
	}
	return 0;
}
