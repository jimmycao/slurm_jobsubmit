/*****************************************************************************\
 *  allocator.c  - dynamic resource allocation
 *****************************************************************************
 *  Copyright (C) 2012-2013 Los Alamos National Security, LLC.
 *  Written by Jimmy Cao <Jimmy.Cao@emc.com>, Ralph Castain <rhc@open-mpi.org>
 *  All rights reserved.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.schedmd.com/slurmdocs/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "allocate.h"
#include "info.h"
#include "argv.h"
#include "msg.h"


#define SIZE 2048

static void _parse_job_params(char *cmd, char *orte_jobid,
				char *return_flag,	size_t *job_timeout);

static void _parse_app_params(char *cmd, char *appid, uint32_t *np,
								uint32_t *request_node_num,
								char *node_range_list, char *flag);

static int _allocate_app_op(char *msg_app, size_t app_timeout,
							char *app_resp_msg);

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


static int _allocate_app_op(char *msg_app, size_t app_timeout,
							char *app_resp_msg)
{
	char appid[16];
	uint32_t  np = 0;
	uint32_t  request_node_num = 0;
	char node_range_list[SIZE] = "";
	char flag[16] = "mandatory";  /* if not specified, by default */
	/* out params */
	uint32_t slurm_jobid;
	char resp_node_list[SIZE], *seperated_nodelist;
	char tasks_per_node[SIZE];
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
		sprintf(app_resp_msg, "app=%s allocate_failure", appid);
	}
	printf("jimmy-9-2: app_resp_msg = %s\n", app_resp_msg);
	return 0;
}


//msg = allocate jobid=100 return=all timeout=10:app=0 np=5 N=2 node_list=vm1,vm2,vm3 flag=optional timeout=20:app=1 N=2

extern int allocate_job_op(slurm_fd_t new_fd, char *msg)
{
	char orte_jobid[16] = "";
	char return_flag[16] = "";
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
			_allocate_app_op(*tmp_app_argv, app_timeout, app_resp_msg);
			if(0 == strcmp(return_flag, "all") && 0 != strlen(app_resp_msg)){
				argv_append_nosize(&all_resp_msg_argv, app_resp_msg);
			}else if(0 != strlen(app_resp_msg)){
				/* if return_flag != "all",
				 * each app's allocation will be sent individually */
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
