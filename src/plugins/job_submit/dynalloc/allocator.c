/*
 * allocator.c
 *
 *  Created on: Jan 6, 2013
 *      Author: caoj7
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "allocate.h"
#include "info.h"
#include "argv.h"
#include "msg.h"


#define SIZE 2048

/*
 * Parse the msg(cmd) to obtain several parameters
 *
 * IN:
 * OUT Parameter:
 * RET OUT
 * 	-1  if
 * 	0
 */
static void _parse_job_params(char *cmd, char *orte_jobid,
				char *return_flag,	size_t *job_timeout)
{
	char *tmp;
	char *p_str;
	char *pos;

	tmp = strdup(cmd);
	p_str = strtok(tmp, " ");
	while(p_str){
		if(strstr(p_str, "jobid")){
			pos = strchr(p_str, '=');
						pos++;  /* step over the = */
			strcpy(orte_jobid, pos);
		}else if(strstr(p_str, "return")){
			pos = strchr(p_str, '=');
						pos++;  /* step over the = */
			strcpy(return_flag, pos);
		}else if(strstr(p_str, "timeout")){
			pos = strchr(p_str, '=');
						pos++;  /* step over the = */
			*job_timeout = atol(pos);
		}
		p_str = strtok(NULL, " ");
	}
		/* cleanup */
		free(tmp);
}

//app=0 np=5 N=2 node_list=vm1,vm2,vm3 flag=optional

static void _parse_app_params(char *cmd, char *appid, uint32_t *np,
								uint32_t *request_node_num,
								char *node_range_list, char *flag)
{
	char *tmp;
	char *p_str;
	char *pos;

	tmp = strdup(cmd);
	p_str = strtok(tmp, " ");
	while(p_str){
		if(strstr(p_str, "app")){
			pos = strchr(p_str, '=');
						pos++;  /* step over the = */
			strcpy(appid, pos);
		}else if(strstr(p_str, "np")){
			pos = strchr(p_str, '=');
						pos++;  /* step over the = */
			*np = atoi(pos);
		}else if(strstr(p_str, "N=")){
			pos =  strchr(p_str, '=');
                        pos++;  /* step over the = */
			*request_node_num = atoi(pos);
		}else  if(strstr(p_str, "node_list")){
			pos = strchr(p_str, '=');
                        pos++;  /* step over the = */
            strcpy(node_range_list, pos);
		}else  if(strstr(p_str, "flag")){
			pos = strchr(p_str, '=');
                        pos++;  /* step over the = */
            strcpy(flag, pos);
		}
		p_str = strtok(NULL, " ");
	}
        /* cleanup */
        free(tmp);
}


static int allocate_app_op(char *msg_app, size_t app_timeout,
							char *app_resp_msg)
{
	char appid[16];
	uint32_t  np = 0;
	uint32_t  request_node_num = 0;
	char node_range_list[SIZE] = "";
	char flag[16] = "mandatory";  /* if not specified, by default */
	/* out params */
	uint32_t slurm_jobid;
	char resp_node_list[SIZE];
	char tasks_per_node[SIZE];
	//
	char *seperated_nodelist;
	int rc;


	_parse_app_params(msg_app, appid, &np, &request_node_num,
									node_range_list, flag);

	printf("---------------------------------\n");
	printf("jimmy-9-2: msg_app = %s\n", msg_app);
	printf("jimmy-9-2: appid = %s\n", appid);
	printf("jimmy-9-2: np = %u\n", np);
	printf("jimmy-9-2: request_node_num = %u\n", request_node_num);
	printf("jimmy-9-2: node_range_list = %s\n", node_range_list);
	printf("jimmy-9-2: flag = %s\n", flag);
	printf("jimmy-9-2: app_timeout = %u\n", app_timeout);


	rc = allocate_node_rpc(np, request_node_num, node_range_list, flag,
			app_timeout, &slurm_jobid, resp_node_list, tasks_per_node);

	if(rc == 0){
		seperated_nodelist = seperate_nodelist_with_comma(resp_node_list);
		sprintf(app_resp_msg,
			"app=%s slurm_jobid=%u allocated_node_list=%s tasks_per_node=%s",
			 appid, slurm_jobid, seperated_nodelist, tasks_per_node);
	} else{
		sprintf(app_resp_msg, "app=%s allocate failure", appid);
	}
	printf("jimmy-9-2: app_resp_msg = %s\n", app_resp_msg);
	return 0;
}


//msg = allocate jobid=100 return=all timeout=10:app=0 np=5 N=2 node_list=vm1,vm2,vm3 flag=optional timeout=20:app=1 N=2

extern int allocate_job_op(slurm_fd_t new_fd, char *msg)
{
	char orte_jobid[16];
	char return_flag[16];
	size_t job_timeout = 15; //if not specified, by default

	char send_buf[SIZE];
	char **app_argv = NULL, **tmp_app_argv;
	size_t app_timeout;
	uint32_t app_count = 1;
	char app_resp_msg[SIZE];
	char **all_resp_msg_argv = NULL, **tmp_all_resp_msg_argv;

	app_argv = argv_split(msg, ':');
	/* app_count dose not include the first part (job info) */
	app_count = argv_count(app_argv) - 1;
	/* app_argv will be freed */
	tmp_app_argv = app_argv;
	while(*tmp_app_argv){
		if(strstr(*tmp_app_argv, "allocate")){
			_parse_job_params(*tmp_app_argv, orte_jobid,
								return_flag, &job_timeout);
			printf("------------------------\n");
			printf("jimmy-9-1: *argv = %s\n", *tmp_app_argv);
			printf("jimmy-9-1: orte_jobid = %s\n", orte_jobid);
			printf("jimmy-9-1: return_flag = %s\n", return_flag);
			printf("jimmy-9-1: timeout = %lu\n", job_timeout);
		}else if(strstr(*tmp_app_argv, "app")){
			app_timeout = job_timeout / app_count;
			allocate_app_op(*tmp_app_argv, app_timeout, app_resp_msg);
			if(0 == strcmp(return_flag, "all") && 0 != strlen(app_resp_msg)){
				argv_append_nosize(&all_resp_msg_argv, app_resp_msg);
			}else if(0 != strlen(app_resp_msg)){
				/* if return_flag != "all",
				 * each app's allocation will be sent individually
				 */
				sprintf(send_buf, "jobid=%s:%s", orte_jobid, app_resp_msg);
				info("BBB: send to client: %s", send_buf);
				send_reply(new_fd, send_buf);
			}
		}
		tmp_app_argv++;
	}
	/* free app_argv */
	argv_free(app_argv);

	if(0 == strcmp(return_flag, "all")){
		sprintf(send_buf, "jobid=%s", orte_jobid);
		/* all_resp_msg_argv will be freed*/
		tmp_all_resp_msg_argv = all_resp_msg_argv;
		while(*tmp_all_resp_msg_argv){
			sprintf(send_buf, "%s:%s", send_buf, *tmp_all_resp_msg_argv);
			tmp_all_resp_msg_argv++;
		}
		/* free all_resp_msg_argv */
		argv_free(all_resp_msg_argv);

		info("BBB: send to client: %s", send_buf);
		send_reply(new_fd, send_buf);
	}

	return 0;
}

/*****************************************************************************\
 * allocate operation, parse the msg(cmd), make allocation, then send the result
\*****************************************************************************/

//extern void allocate_op(slurm_fd_t new_fd, char *msg)
//{
//
//
//
//
//
//			//if not specified, by default
//
//
//	char reponse_node_list[SIZE], *seperated_nodelist;
//	int rc, i;
//	char **argv, **rt_argv;
//	int count;
//	bool return_all_flag;
//	char send_buf[SIZE];

//
//
////		argv = argv_split(cmd_rcv, ':');
////		if(NULL == argv){
////			strcpy(buf_rt, "allocate failure, bad resource request");
////			return;
////		}
////		allocate jobid=100 return=all timeout=10
//
////		while(*argv){
////			if(strstr(*argv, "allocate")){
////				_parse_job_allocate_params(*argv, &orte_jobid, &return_all_flag, &timeout);
////			}else if(strstr(*argv, "app")){
////				_parse_app_allocate_params();
////			}
////		}
//
//
//		_parse_job_allocate_params(msg, &orte_jobid, &appid, &request_node_num,
//							&node_range_list, &np, &flag, &timeout);
////===============================
//
//		rc = allocate_node_rpc(request_node_num, node_range_list, &slurm_jobid,
//										reponse_node_list, flag, timeout);
//
//
//
//		if(rc == 0){
//			seperated_nodelist = seperate_nodelist_with_comma(reponse_node_list);
//			sprintf(send_buf, "slurm_jobid=%u allocated_node_list=%s",
//					slurm_jobid, seperated_nodelist);
//		} else
//			strcpy(send_buf, "allocate failure, timeout or request too many nodes");
////	}
//}


