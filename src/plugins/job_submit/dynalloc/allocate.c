/*****************************************************************************\
 *  allocate.c  - dynamic resource allocation
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

#include <signal.h>

#include "slurm/slurm.h"
#include "slurm/slurm_errno.h"

#include "src/common/log.h"
#include "src/common/bitstring.h"
#include "src/common/node_conf.h"
#include "src/common/xmalloc.h"

#include "src/slurmctld/slurmctld.h"
#include "src/slurmctld/node_scheduler.h"
#include "src/slurmctld/locks.h"
#include "src/slurmctld/state_save.h"

#include "allocate.h"
#include "info.h"

#define SIZE 8192

static int get_nodelist_optional(uint16_t request_node_num,
								char *node_range_list,
								char *final_req_node_list);

static int get_nodelist_mandatory(uint16_t request_node_num,
								char *node_range_list,
								char *final_req_node_list,
								time_t timeout, int hint);

/**
 *	select n nodes from the given node_range_list.
 *
 *	optional means trying best to allocate node from
 *	node_range_list, allocation should include all nodes
 *	in the given list that are currently available. If
 *	that isn't enough to meet the request_node_num,
 * 	then take  any other nodes that are available to
 * 	fill out the requested number.
 *
 *	IN:
 *		request_node_num: requested node num
 *		node_range_list: specified node range to select from
 *	OUT Parameter:
 *		final_req_node_list
 *	RET OUT
 *		-1 if requested node number is larger than available
 *		0  successful, final_req_node_list is returned
 */
static int get_nodelist_optional(uint16_t request_node_num,
								char *node_range_list,
								char *final_req_node_list)
{
	hostlist_t avail_hl_system;  //available hostlist in slurm
	hostlist_t avail_hl_pool;	//available hostlist in the given node pool
	hostlist_t hostlist;
	char *avail_pool_range;
	int avail_pool_num;
	int extra_needed_num;
	char *subset;
	char *hostname;
	int i;

	/* if request_node_num is not specified,
	 * then node_range_list will be final_req_node_list.
	 */
	if(request_node_num == 0 && 0 != strlen(node_range_list)){
		strcpy(final_req_node_list, node_range_list);
		return 0;
	}

	/* get all available hostlist in SLURM system */
	avail_hl_system = get_available_host_list_system();

	if(request_node_num > slurm_hostlist_count(avail_hl_system))
		return -1;

	avail_hl_pool = choose_available_from_node_list(node_range_list);
	avail_pool_range = slurm_hostlist_ranged_string_malloc(avail_hl_pool);
	avail_pool_num = slurm_hostlist_count(avail_hl_pool);

	if(request_node_num <= avail_pool_num) {
		subset = get_hostlist_subset(avail_pool_range,
									request_node_num);
		strcpy(final_req_node_list, subset);
	}else{ /* avail_pool_num < reqeust_node_num <= avail_node_num_system */
		hostlist = slurm_hostlist_create(avail_pool_range);
		extra_needed_num = request_node_num - avail_pool_num;

		for(i = 0; i < extra_needed_num;){
			hostname = slurm_hostlist_shift(avail_hl_system);
			if(slurm_hostlist_find(hostlist, hostname) == -1){
				slurm_hostlist_push_host(hostlist, hostname);
				i++;
			}
		}

		strcpy(final_req_node_list,
				slurm_hostlist_ranged_string_xmalloc(hostlist));
	}
	return 0;
}

/**
 *	select n nodes from the given node_range_list
 *
 *	mandatory means all nodes must be allocated
 *	from node_range_list
 *
 *	IN:
 *		request_node_num: requested node num
 *		node_range_list: specified node range to select from
 *		timeout: timeout
 *		hint: to indicate this function is first called or iteration
 *	OUT Parameter:
 *		final_req_node_list
 *	RET OUT
 *		-1 if requested node number is larger than available or timeout
 *		0  successful, final_req_node_list is returned
 */
static int get_nodelist_mandatory(uint16_t request_node_num,
								char *node_range_list,
								char *final_req_node_list,
								time_t timeout, int hint)
{
	hostlist_t avail_hl;
	char *avail_node_range;
	char *subset;

	/* if request_node_num is not specified,
	 * then node_range_list will be final_req_node_list.
	 */
	if(request_node_num == 0 && 0 != strlen(node_range_list)){
		strcpy(final_req_node_list, node_range_list);
		return 0;
	}

	avail_hl = choose_available_from_node_list(node_range_list);
	avail_node_range = slurm_hostlist_ranged_string_malloc(avail_hl);

	if(request_node_num <= slurm_hostlist_count(avail_hl)) {
		subset = get_hostlist_subset(avail_node_range, request_node_num);
		strcpy(final_req_node_list, subset);
	}else{
		if(timeout == 0 && hint == 1){
			sleep(10);
		}else{
			sleep(10);
			timeout -= 10;
			hint = 0;
			if(timeout < 0)
				return -1;
		}
		return get_nodelist_mandatory(request_node_num, node_range_list,
								final_req_node_list, timeout, hint);
	}
	return 0;
}

/**
 *	select n nodes from the given node_range_list through rpc
 *
 *  if (flag == mandatory), all requested nodes must be allocated
 *  from node_list; else if (flag == optional), try best to allocate
 *  node from node_list, and the allocation should include all
 *  nodes in the given list that are currently available. If that
 *  isn't enough to meet the node_num_request, then take any other
 *  nodes that are available to fill out the requested number.
 *
 *	IN:
 *		request_node_num: requested node num
 *		node_range_list: specified node range to select from
 *		flag: optional or mandatory
 *		timeout: timeout
 *		hint: to indicate this function is first called or iteration
 *	OUT Parameter:
 *		jobid: slurm jobid
 *		reponse_node_list:
 *	RET OUT
 *		-1 if requested node number is larger than available or timeout
 *		0  successful, final_req_node_list is returned
 */

//allocate_node_rpc(np, request_node_num, node_range_list, flag,
//			app_timeout, &slurm_jobid, resp_node_list, tasks_per_node);

int allocate_node_rpc(uint32_t np, uint32_t request_node_num,
					char *node_range_list, char *flag, time_t timeout,
					uint32_t *slurm_jobid, char *reponse_node_list,
					char *tasks_per_node)
{
	job_desc_msg_t job_desc_msg;
	resource_allocation_response_msg_t *job_alloc_resp_msg;
	char final_req_node_list[SIZE] = "";
	int rc;

	slurm_init_job_desc_msg (&job_desc_msg);
	printf("+++++after init++++++++++++++\n");
	printf("jimmy-7-1: job_desc_msg.num_tasks = %u\n", job_desc_msg.num_tasks);
	printf("jimmy-7-1: job_desc_msg.min_nodes = %u\n", job_desc_msg.min_nodes);
	printf("jimmy-7-1: job_desc_msg.req_nodes = %s\n", job_desc_msg.req_nodes);

	job_desc_msg.user_id = getuid();
	job_desc_msg.group_id = getgid();
	job_desc_msg.contiguous = 0;

	if(np != 0)
		job_desc_msg.num_tasks = np;

	if(NULL == node_range_list || 0 == strlen(node_range_list)){
		if(request_node_num != 0)
			job_desc_msg.min_nodes = request_node_num;
	}else{
		if(!strcasecmp(flag, "mandatory")){
			rc = get_nodelist_mandatory(request_node_num, node_range_list,
										final_req_node_list, timeout, 1);
			if(rc == -1){
				error ("timeout!");
				return -1;
			}else if(0 != strlen(final_req_node_list)){
				job_desc_msg.req_nodes = final_req_node_list;
			}
		} else {  /* flag == "optional" */
			rc = get_nodelist_optional(request_node_num,
									node_range_list, final_req_node_list);
			if(rc == -1){
				if(request_node_num != 0)
					job_desc_msg.min_nodes = request_node_num;
			} else if(0 != strlen(final_req_node_list)) {
					job_desc_msg.req_nodes = final_req_node_list;
			}
		}
	}

	printf("+++++++++++++++++++\n");
	printf("jimmy-8-1: job_desc_msg.num_tasks = %u\n", job_desc_msg.num_tasks);
	printf("jimmy-8-1: job_desc_msg.min_nodes = %u\n", job_desc_msg.min_nodes);
	printf("jimmy-8-1: job_desc_msg.req_nodes = %s\n", job_desc_msg.req_nodes);

	job_alloc_resp_msg = slurm_allocate_resources_blocking(&job_desc_msg,
											timeout, NULL);
	if (!job_alloc_resp_msg) {
		error("allocate failure, timeout or request too many nodes");
		return -1;
	}

	info ("allocate [ node_list = %s ] to [ job_id = %u ]",
			job_alloc_resp_msg->node_list, job_alloc_resp_msg->job_id);

	*slurm_jobid = job_alloc_resp_msg->job_id;
	strcpy(reponse_node_list, job_alloc_resp_msg->node_list);
	//jimmy, to do
	strcpy(tasks_per_node, "2,1(x3)");

	/* free the allocated resource msg */
	slurm_free_resource_allocation_response_msg(job_alloc_resp_msg);


	//kill the job, just for test
	if (slurm_kill_job(job_alloc_resp_msg->job_id, SIGKILL, 0)) {
		 error ("ERROR: kill job %d\n", slurm_get_errno());
		 return -1;
	}

	return 0;
}

/**
 *	select n nodes from the given node_range_list directly through
 *	"job_allocate" in slurmctld/job_mgr.c
 *
 *  if (flag == mandatory), all requested nodes must be allocated
 *  from node_list; else if (flag == optional), try best to allocate
 *  node from node_list, and the allocation should include all
 *  nodes in the given list that are currently available. If that
 *  isn't enough to meet the node_num_request, then take any other
 *  nodes that are available to fill out the requested number.
 *
 *	IN:
 *		request_node_num: requested node num
 *		node_range_list: specified node range to select from
 *		flag: optional or mandatory
 *		timeout: timeout
 *		hint: to indicate this function is first called or iteration
 *	OUT Parameter:
 *		jobid: slurm jobid
 *		reponse_node_list:
 *	RET OUT
 *		-1 if requested node number is larger than available or timeout
 *		0  successful, final_req_node_list is returned
 */
int allocate_node(uint32_t request_node_num, char *node_range_list,
					uint32_t *jobid, char *reponse_node_list,
					char *flag, time_t timeout)
{
	char final_req_node_list[SIZE];
	int rc, error_code;

	job_desc_msg_t job_desc_msg;
	struct job_record *job_ptr;
	bool job_waiting = false;
	uid_t uid;

	slurm_init_job_desc_msg (&job_desc_msg);
	uid = getuid();
	job_desc_msg.user_id = uid;
	job_desc_msg.group_id = getgid();
	job_desc_msg.contiguous = 0;
	job_desc_msg.immediate = 0;

	if(!strcmp(node_range_list, "")){
		job_desc_msg.min_nodes = request_node_num;
	}else{
		if(!strcasecmp(flag, "mandatory")){
			rc = get_nodelist_mandatory(request_node_num, node_range_list,
										final_req_node_list, timeout, 1);
			if(rc == -1){
				error ("timeout!");
				return -1;
			}
			job_desc_msg.req_nodes = final_req_node_list;
		} else {  /* flag == "optional" */
			rc = get_nodelist_optional(request_node_num,
									node_range_list, final_req_node_list);
			if(rc == -1){
				job_desc_msg.min_nodes = request_node_num;
			}
			else{
				job_desc_msg.req_nodes = final_req_node_list;
			}
		}
	}

	rc = validate_job_create_req(&job_desc_msg);
	if(rc != 0){
		error("invalid job request.");
		return -1;
	}

	error_code = job_allocate(&job_desc_msg, job_desc_msg.immediate,
						  false, //will run
						  NULL, // will_run_response_msg_t
						  true, //allocate
						  job_desc_msg.user_id, &job_ptr);

	if ((error_code == ESLURM_REQUESTED_PART_CONFIG_UNAVAILABLE) ||
		(error_code == ESLURM_RESERVATION_NOT_USABLE) ||
		(error_code == ESLURM_QOS_THRES) ||
		(error_code == ESLURM_NODE_NOT_AVAIL) ||
		(error_code == ESLURM_JOB_HELD))
		job_waiting = true;

	if ((error_code == SLURM_SUCCESS) ||
		((job_desc_msg.immediate == 0) && job_waiting)) {
		xassert(job_ptr);

		/* notice: allocated node list is in 'job_ptr->job_id' */
		/* not 'job_ptr->alloc_node' */

		if(job_ptr->job_id > 0 && job_ptr->nodes == NULL){
			/* job is pending, so cancel the job */
			cancel_job(job_ptr->job_id, uid);
			return -1;
		}else{  /* allocate successful */
			strcpy(reponse_node_list, job_ptr->nodes);
			*jobid = job_ptr->job_id;
			info ("allocate [ allocated_node_list = %s ] to [ slurm_jobid = %u ]",
							job_ptr->nodes, job_ptr->job_id);
			//jimmy, just for test
			cancel_job(job_ptr->job_id, uid);

			return 0;
		}
	}else{
		return -1;
	}
}

/**
 *	cancel a job
 *
 *	IN:
 *		job_id: slurm jobid
 *		uid: user id
 *	OUT Parameter:
 *	RET OUT
 *		-1 failure
 *		0  successful
 */
int cancel_job(uint32_t job_id, uid_t uid)
{
	int rc;
	/* Locks: Read config, write job, write node */
	slurmctld_lock_t job_write_lock = {
		READ_LOCK, WRITE_LOCK, WRITE_LOCK, NO_LOCK };

	lock_slurmctld(job_write_lock);
	rc = job_signal(job_id, SIGKILL,
							0, uid, false);
	unlock_slurmctld(job_write_lock);

	if (rc) { /* cancel failure */
		info("Signal %u JobId=%u by UID=%u: %s",
				SIGKILL, job_id, uid,
				slurm_strerror(rc));
		return -1;
	} else { /* cancel successful */
		info("sched: Cancel of JobId=%u by UID=%u",
				job_id, uid);
		slurmctld_diag_stats.jobs_canceled++;

		/* Below function provides its own locking */
		schedule_job_save();
		return 0;
	}
}
