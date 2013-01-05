/*****************************************************************************\
 *  msg.c - Message/communcation manager for dynalloc (resource dynamic allocation) plugin
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

#include "slurm/slurm.h"
#include "src/common/uid.h"
#include "src/slurmctld/locks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "info.h"
#include "allocate.h"
#include "msg.h"



#define _DEBUG 0

/* When a remote socket closes on AIX, we have seen poll() return EAGAIN
 * indefinitely for a pending write request. Rather than locking up
 * slurmctld's dynalloc interface, abort after MAX_RETRIES poll() failures. */
#define MAX_RETRIES 10

static bool thread_running = false;
static bool thread_shutdown = false;
static pthread_mutex_t thread_flag_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t msg_thread_id;
static char *err_msg;
static int   err_code;
static uint16_t sched_port;

#define SIZE 512

static void *	_msg_thread(void *no_data);
static void	_proc_msg(slurm_fd_t new_fd, char *msg);
static size_t	_read_bytes(int fd, char *buf, size_t size);
static char *	_recv_msg(slurm_fd_t new_fd);
static size_t	_send_msg(slurm_fd_t new_fd, char *buf, size_t size);
static void	_send_reply(slurm_fd_t new_fd, char *response);
static size_t	_write_bytes(int fd, char *buf, size_t size);

static void _operation(char *cmd_rcv, char *buf_rt);
static void _parse_allocate_params(char *cmd, uint32_t *request_node_num,
			char **node_range_list, char **flag, size_t *timeout);

/*****************************************************************************\
 * spawn message hander thread
\*****************************************************************************/
extern int spawn_msg_thread(void)
{
	pthread_attr_t thread_attr_msg;

	pthread_mutex_lock( &thread_flag_mutex );
	if (thread_running) {
		error("dynalloc thread already running, not starting another");
		pthread_mutex_unlock(&thread_flag_mutex);
		return SLURM_ERROR;
	}

	slurm_attr_init(&thread_attr_msg);
	if (pthread_create(&msg_thread_id, &thread_attr_msg,
			_msg_thread, NULL))
		fatal("pthread_create %m");
	else
		info("dynalloc: msg thread create successful!");


	slurm_attr_destroy(&thread_attr_msg);
	thread_running = true;
	pthread_mutex_unlock(&thread_flag_mutex);
	return SLURM_SUCCESS;
}

/*****************************************************************************\
 * terminate message hander thread
\*****************************************************************************/
extern void term_msg_thread(void)
{
	pthread_mutex_lock(&thread_flag_mutex);
	if (thread_running) {
		int fd;
		slurm_addr_t addr;

		thread_shutdown = true;

		/* Open and close a connection to the listening port.
		 * Allows slurm_accept_msg_conn() to return in
		 * _msg_thread() so that it can check the thread_shutdown
		 * flag.
		 */
		slurm_set_addr(&addr, sched_port, "localhost");
		fd = slurm_open_stream(&addr);
		if (fd != -1) {
			/* we don't care if the open failed */
			slurm_close_stream(fd);
		}

		debug2("waiting for dynalloc thread to exit");
		pthread_join(msg_thread_id, NULL);
		msg_thread_id = 0;
		thread_shutdown = false;
		thread_running = false;
		debug2("join of dynalloc thread successful");
	}
	pthread_mutex_unlock(&thread_flag_mutex);
}

/*****************************************************************************\
 * message hander thread
\*****************************************************************************/
static void *_msg_thread(void *no_data)
{
	slurm_fd_t sock_fd = -1, new_fd;
	slurm_addr_t cli_addr;
	char *msg;
	slurm_ctl_conf_t *conf;
	int i;
	/* Locks: Write configuration, job, node, and partition */
	slurmctld_lock_t config_write_lock = {
		WRITE_LOCK, WRITE_LOCK, WRITE_LOCK, WRITE_LOCK };

	conf = slurm_conf_lock();
	sched_port = conf->js_dynallocport;
	slurm_conf_unlock();

	/* Wait until configuration is completely loaded */
	lock_slurmctld(config_write_lock);
	unlock_slurmctld(config_write_lock);

	/* If JobSubmitDynAllocPort is already taken, keep trying to open it
	 * once per minute. Slurmctld will continue to function
	 * during this interval even if nothing can be scheduled. */
	for (i=0; (!thread_shutdown); i++) {
		if (i > 0)
			sleep(60);
                fprintf(stderr, "Attempting to open dynalloc port %d\n", sched_port);
		sock_fd = slurm_init_msg_engine_port(sched_port);
		if (sock_fd != SLURM_SOCKET_ERROR)
			break;
		error("dynalloc: slurm_init_msg_engine_port %u %m",
			sched_port);
		error("dynalloc: Unable to communicate with ORTE RAS");
	}

	/* Process incoming RPCs until told to shutdown */
	while (!thread_shutdown) {
		if ((new_fd = slurm_accept_msg_conn(sock_fd, &cli_addr))
				== SLURM_SOCKET_ERROR) {
			if (errno != EINTR)
				error("dyalloc: slurm_accept_msg_conn %m");
			continue;
		}

		if (thread_shutdown) {
			close(new_fd);
			break;
		}
		/* It would be nice to create a pthread for each new
		 * RPC, but that leaks memory on some systems when
		 * done from a plugin.
		 * FIXME: Maintain a pool of pthreads and reuse them. */
		err_code = 0;
		err_msg = "";

		fprintf(stderr, "===============================\n");
		msg = _recv_msg(new_fd);
		if (msg) {
			_proc_msg(new_fd, msg);
			xfree(msg);
		}
		slurm_close_accepted_conn(new_fd);
	}
	verbose("dynalloc: message engine shutdown");
	if (sock_fd > 0)
		(void) slurm_shutdown_msg_engine(sock_fd);
	pthread_exit((void *) 0);
	return NULL;
}

static size_t 	_read_bytes(int fd, char *buf, size_t size)
{
	size_t bytes_remaining, bytes_read;
	char *ptr;
	struct pollfd ufds;
	int rc;

	bytes_remaining = size;
	size = 0;
	ufds.fd = fd;
	ufds.events = POLLIN;
	ptr = buf;
	while (bytes_remaining > 0) {
//		rc = poll(&ufds, 1, 10000);	/* 10 sec timeout *///default
		rc = poll(&ufds, 1, 100);  //0.1sec, jimmy
		if (rc == 0)		/* timed out */
			break;
		if ((rc == -1) &&	/* some error */
		    ((errno== EINTR) || (errno == EAGAIN)))
			continue;
		if ((ufds.revents & POLLIN) == 0) /* some poll error */
			break;

		bytes_read = read(fd, ptr, bytes_remaining);
		if (bytes_read <= 0)
			break;
		bytes_remaining -= bytes_read;
		size += bytes_read;
		ptr += bytes_read;
	}

	return size;
}

static size_t 	_write_bytes(int fd, char *buf, size_t size)
{
	size_t bytes_remaining, bytes_written;
	char *ptr;
	struct pollfd ufds;
	int rc, retry_cnt = 0;

	bytes_remaining = size;
	size = 0;
	ptr = buf;
	ufds.fd = fd;
	ufds.events = POLLOUT;
	while (bytes_remaining > 0) {
//		rc = poll(&ufds, 1, 10000);	/* 10 sec timeout */ //default
		rc = poll(&ufds, 1, 100); //0.1sec, jimmy
		if (rc == 0)		/* timed out */
			break;
		if ((rc == -1) &&	/* some error */
		    ((errno== EINTR) || (errno == EAGAIN))) {
			if ((retry_cnt++) >= MAX_RETRIES) {
				error("dynalloc: repeated poll errors for "
				      "write: %m");
				break;
			}
			continue;
		}
		if ((ufds.revents & POLLOUT) == 0) /* some poll error */
			break;

		bytes_written = write(fd, ptr, bytes_remaining);
		if (bytes_written <= 0)
			break;
		bytes_remaining -= bytes_written;
		size += bytes_written;
		ptr += bytes_written;
	}

	return size;
}

/*****************************************************************************\
 * Read a message (request) from specified file descriptor
 *
 * RET - The message which must be xfreed or
 *       NULL on error
\*****************************************************************************/
static char * 	_recv_msg(slurm_fd_t new_fd)
{
	char *buf;
	buf = xmalloc(SIZE + 1);	/* need '\0' on end to print */
	if (_read_bytes((int) new_fd, buf, SIZE) <= 0) {
		err_code = -246;
		err_msg = "unable to read message data";
		error("dynalloc: unable to read data message");
		xfree(buf);
		return NULL;
	}

	info("dynalloc msg recv:%s", buf);

	return buf;
}

/*****************************************************************************\
 * Send a message (response) to specified file descriptor
 *
 * RET - Number of data bytes written (excludes header)
\*****************************************************************************/
static size_t	_send_msg(slurm_fd_t new_fd, char *buf, size_t size)
{
	size_t data_sent;

	if (slurm_get_debug_flags())
		info("dynalloc msg send:%s", buf);

	data_sent = _write_bytes((int) new_fd, buf, size);
	if (data_sent != size) {
		error("dynalloc: unable to write data message (%lu of %lu) %m",
			(long unsigned) data_sent, (long unsigned) size);
	}

	return data_sent;
}

/*****************************************************************************\
 * Parse the msg(cmd) to several parameters
 *
 * RET - parameters
\*****************************************************************************/
static void _parse_allocate_params(char *cmd, uint32_t *request_node_num,
                                   char **node_range_list, char **flag, size_t *timeout)
{
	char *tmp;
	char *p_str;
	char *pos;

	tmp = strdup(cmd);
	p_str = strtok(tmp, " ");
	while(p_str){
		if(strstr(p_str, "N=")){
			pos =  strchr(p_str, '=');
                        pos++;  /* step over the = */
			*request_node_num = atoi(pos);
		}else if(strstr(p_str, "node_list")){
			pos = strchr(p_str, '=');
                        pos++;  /* step over the = */
			*node_range_list = strdup(pos);
		}else if(strstr(p_str, "flag")){
			pos = strchr(p_str, '=');
                        pos++;  /* step over the = */
			*flag = strdup(pos);
		}else if(strstr(p_str, "timeout")){
			pos = strchr(p_str, '=');
                        pos++;  /* step over the = */
			*timeout = atol(pos);
		}
		p_str = strtok(NULL, " ");
	}
        /* cleanup */
        free(tmp);
}

/*****************************************************************************\
 * Parse the msg(cmd), then make operation accordingly (query or allocate)
 *
 * RET - msg for sending
\*****************************************************************************/
static void _operation(char *cmd_rcv, char *buf_rt)
{
	uint16_t nodes, slots;
	uint32_t request_node_num = 0;
	char *node_range_list = "";  	//if not specified, by default
	char *flag = "optional";		//if not specified, by default
	size_t timeout = 15;			//if not specified, by default
	uint32_t jobid;
	char reponse_node_list[SIZE], *seperated_nodelist;
	int rc;

	//identify the cmd
	if(!strcasecmp(cmd_rcv, "get total nodes and slots")){
		rc = get_total_nodes_slots(&nodes, &slots);
		if(rc == 0)
			sprintf(buf_rt, "total_nodes=%d total_slots=%d", nodes, slots);
		else
			strcpy(buf_rt, "query failure");
	}else if(!strcasecmp(cmd_rcv, "get available nodes and slots")){
		rc = get_free_nodes_slots(&nodes, &slots);
		if(rc == 0)
			sprintf(buf_rt, "avail_nodes=%d avail_slots=%d", nodes, slots);
		else
			strcpy(buf_rt, "query failure");
	}else if(!strncasecmp(cmd_rcv, "allocate", 8)){
		_parse_allocate_params(cmd_rcv, &request_node_num,
					&node_range_list, &flag, &timeout);

		rc = allocate_node(request_node_num, node_range_list, &jobid,
										reponse_node_list, flag, timeout);

		if(rc == 0){
			seperated_nodelist = seperate_nodelist_with_comma(reponse_node_list);
			sprintf(buf_rt, "slurm_jobid=%u allocated_node_list=%s", jobid, seperated_nodelist);
		} else
			strcpy(buf_rt, "allocate failure, timeout or request too many nodes");
	}
}

/*****************************************************************************\
 * process and respond to a request
\*****************************************************************************/
static void	_proc_msg(slurm_fd_t new_fd, char *msg)
{
	char send_buf[SIZE];
	info("AAA: received from client: %s", msg);

	if (new_fd < 0)
		return;

	if (!msg)
		strcpy(send_buf, "NULL request, failure");
	else
		_operation(msg, send_buf);

	info("BBB: send to client: %s", send_buf);

	_send_reply(new_fd, send_buf);
	return;
}

static void	_send_reply(slurm_fd_t new_fd, char *response)
{
	_send_msg(new_fd, response, strlen(response)+1);
}
