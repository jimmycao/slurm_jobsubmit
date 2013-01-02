/*
 * allocate.h
 *
 *  Created on: Sep 22, 2012
 *      Author: caoj7
 */

#ifndef ALLOCATE_H_
#define ALLOCATE_H_


#if HAVE_CONFIG_H
#  include "config.h"
#  if HAVE_INTTYPES_H
#    include <inttypes.h>
#  else
#    if HAVE_STDINT_H
#      include <stdint.h>
#    endif
#  endif  /* HAVE_INTTYPES_H */
#else   /* !HAVE_CONFIG_H */
#  include <inttypes.h>
#endif  /*  HAVE_CONFIG_H */

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
extern int allocate_node_rpc(uint32_t request_node_num, char *node_range_list,
					uint32_t *jobid, char *reponse_node_list,
					char *flag, time_t timeout);

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
extern int allocate_node(uint32_t request_node_num, char *node_range_list,
					uint32_t *jobid, char *reponse_node_list,
					char *flag, time_t timeout);

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
extern int cancel_job(uint32_t job_id, uid_t uid);

#endif /* ALLOCATE_H_ */
