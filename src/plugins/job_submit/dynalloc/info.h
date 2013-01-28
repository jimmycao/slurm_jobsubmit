/*****************************************************************************\
 *  info.h - get nodes information in slurm
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

#ifndef INFO_H_
#define INFO_H_

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

#include "slurm/slurm.h"
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
extern int get_total_nodes_slots(uint16_t *nodes, uint16_t *slots);

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
extern int get_free_nodes_slots(uint16_t *nodes, uint16_t *slots);

/**
 *	get available node list in slurm.
 *
 *	IN:
 *	OUT Parameter:
 *	RET OUT:
 *		hostlist_t: available node list in slurm
 *		NULL if slurm load node information error
 */
extern hostlist_t get_available_host_list_system();

/**
 *	get the range of available node list in slurm.
 *
 *	IN:
 *	OUT Parameter:
 *	RET OUT:
 *		a string indicating the range of available node list in slurm
 */
extern char* get_available_host_list_range_sytem();

/**
 *	get available node list within a given node list
 *
 *	IN:
 *		node_list: the given node list
 *	OUT Parameter:
 *	RET OUT
 *		available node list
 */
extern hostlist_t choose_available_from_node_list(const char *node_list);

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
extern char* get_hostlist_subset(const char *host_name_list, uint16_t node_num);

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
extern char* seperate_nodelist_with_comma(const char *node_list);

#endif /* INFO_H_ */
