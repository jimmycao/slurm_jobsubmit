/*****************************************************************************\
 *  info.c - get nodes information in slurm
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slurm/slurm.h"
#include "slurm/slurm_errno.h"
#include "src/common/log.h"

#include "info.h"

/**
 *	get total number of nodes and slots in slurm.
 *
 *	IN:
 *	OUT Parameter:
 *		nodes: number of nodes in slurm
 *		slots: number of slots in slurm
 *	RET OUT
 *		-1 if slurm load node information error
 *		0  successful
 */
int get_total_nodes_slots (uint16_t *nodes, uint16_t *slots)
{
	int i;
	node_info_msg_t *node_info_msg_t_ptr = NULL;

	*nodes = 0;
	*slots = 0;
	//get node info
	if(slurm_load_node((time_t)NULL, &node_info_msg_t_ptr, SHOW_ALL)){
		error("slurm_load_node");
		return SLURM_FAILURE;
	}

	*nodes = node_info_msg_t_ptr->record_count;

	for(i = 0; i < *nodes; i++)
		(*slots) += node_info_msg_t_ptr->node_array[i].cpus;

	return SLURM_SUCCESS;
}

/**
 *	get number of available nodes and slots in slurm.
 *
 *	IN:
 *	OUT Parameter:
 *		nodes: number of available nodes in slurm
 *		slots: number of available slots in slurm
 *	RET OUT
 *		-1 if slurm load node information error
 *		0  successful
 */
int get_free_nodes_slots (uint16_t *nodes, uint16_t *slots)
{

	node_info_msg_t *node_info_msg_t_ptr = NULL;
	int i;

	*nodes = 0;
	*slots = 0;

	//get node info
	if(slurm_load_node((time_t)NULL, &node_info_msg_t_ptr, SHOW_ALL)){
		error("slurm_load_node");
		return SLURM_FAILURE;
	}

	for(i = 0; i < node_info_msg_t_ptr->record_count; i++){
		if(NODE_STATE_IDLE == node_info_msg_t_ptr->node_array[i].node_state){
			(*nodes) ++;
			(*slots) += node_info_msg_t_ptr->node_array[i].cpus;
		}
	}
	return SLURM_SUCCESS;
}

/**
 *	get available node list in slurm.
 *
 *	IN:
 *	OUT Parameter:
 *	RET OUT:
 *		hostlist_t: available node list in slurm
 *		NULL if slurm load node information error
 */
hostlist_t get_available_host_list_system()
{
	node_info_msg_t *node_info_ptr = NULL;
	hostlist_t hostlist;
	int i;

	if(slurm_load_node((time_t)NULL, &node_info_ptr, SHOW_ALL)){
		error("slurm_load_node");
		return NULL;
	}

	hostlist = slurm_hostlist_create(NULL);
	for(i = 0; i < node_info_ptr->record_count;  i++){
		if(NODE_STATE_IDLE == node_info_ptr->node_array[i].node_state){
			 slurm_hostlist_push_host(hostlist,
					 node_info_ptr->node_array[i].name);
		}
	}

	slurm_free_node_info_msg (node_info_ptr);
	return hostlist;
}

/**
 *	get the range of available node list in slurm.
 *
 *	IN:
 *	OUT Parameter:
 *	RET OUT:
 *		a string indicating the range of available node list in slurm
 */
char* get_available_host_list_range_sytem()
{
	hostlist_t hostlist;
	char *range;

	hostlist = get_available_host_list_system();
	range = slurm_hostlist_ranged_string_malloc (hostlist);
	slurm_hostlist_destroy(hostlist);
	return range;
}

/**
 *	get available node list within a given node list
 *
 *	IN:
 *		node_list: the given node list
 *	OUT Parameter:
 *	RET OUT
 *		available node list
 */
hostlist_t choose_available_from_node_list(const char *node_list)
{
	char *hostname;
	hostlist_t given_hl;
	hostlist_t avail_hl_system;
	hostlist_t result_hl;

	given_hl = slurm_hostlist_create (node_list);
	avail_hl_system  = get_available_host_list_system();
	result_hl = slurm_hostlist_create(NULL);

	while((hostname = slurm_hostlist_shift(given_hl))){
		if(-1 != slurm_hostlist_find (avail_hl_system, hostname)) {
			slurm_hostlist_push_host(result_hl, hostname);
		}
	}

	slurm_hostlist_destroy(given_hl);
	slurm_hostlist_destroy(avail_hl_system);
	return result_hl;
}

/**
 *	get a subset node range with node_num nodes from a host_name_list
 *
 *	IN:
 *		host_name_list: the given host_name_list
 *		node_num: the number of host to choose
 *	OUT Parameter:
 *	RET OUT
 *		the subset node range, NULL if the node number of subset is
 *		larger than the node number of host_name_list
 */
char* get_hostlist_subset(const char *host_name_list, uint16_t node_num)
{
	hostlist_t hostlist;
	hostlist_t temp_hl;
	int sum;
	char *hostname;
	char *range;
	int i;

	hostlist = slurm_hostlist_create(host_name_list);
	sum = slurm_hostlist_count(hostlist);

	if(sum < node_num){
		error ("node_num > sum of host in hostlist");
		return NULL;
	}

	temp_hl = slurm_hostlist_create(NULL);

	for(i = 0; i < node_num; i++){
		hostname = slurm_hostlist_shift(hostlist);
		slurm_hostlist_push_host(temp_hl, hostname);
	}

	range = slurm_hostlist_ranged_string_malloc(temp_hl);

	slurm_hostlist_destroy(temp_hl);
	slurm_hostlist_destroy(hostlist);
	return range;
}

/**
 *	transform nodelist with regular expression into
 *	comma seperated nodelist, like:
 *	host[2-3] will be host2,host3
 *	host[3,5] will be host3,host5
 *
 *	IN:
 *		node_list: the given node_list
 *	OUT Parameter:
 *	RET OUT
 *		comma seperated nodelist
 */
char* seperate_nodelist_with_comma(const char *node_list)
{
	char *parsed_nodelist = NULL;
	char *tmp;
	char *nodename;
	hostlist_t given_hl;

	if(NULL == node_list)
		return NULL;

	given_hl = slurm_hostlist_create(node_list);

	while((nodename = slurm_hostlist_shift(given_hl))){
		if(NULL == parsed_nodelist)
			parsed_nodelist = strdup(nodename);
		else {
			asprintf(&tmp, "%s,%s", parsed_nodelist, nodename);
			free(parsed_nodelist);
			parsed_nodelist = tmp;
		}
	}

	return parsed_nodelist;
}
