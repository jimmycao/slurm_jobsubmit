/*
 * deallocate.c
 *
 *  Created on: Jan 22, 2013
 *      Author: caoj7
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slurm/slurm.h"
#include "slurm/slurm_errno.h"

#include "src/common/log.h"
#include "src/slurmctld/state_save.h"
#include "src/slurmctld/locks.h"

#include "deallocate.h"
#include "argv.h"

#define SIZE 256

int deallocate(slurm_fd_t new_fd, char *msg)
{
	char **jobid_argv = NULL, **tmp_jobid_argv;
	char *pos;
	/* params to complete a job allocation */
	uint32_t slurm_jobid;
	uid_t uid = 0;
	bool job_requeue = false;
	bool node_fail = false;
	uint32_t job_return_code = NO_VAL;
	int  rc = SLURM_SUCCESS;

	char tmp_buf[SIZE];
	char send_buf[SIZE] = "";


	jobid_argv = argv_split(msg, ':');
	/* jobid_argv will be freed */
	tmp_jobid_argv = jobid_argv;

	while(*tmp_jobid_argv){
		/* to identify the slurm_job */
		if(NULL != (pos = strstr(*tmp_jobid_argv, "slurm_jobid="))){
			pos = pos + strlen("slurm_jobid=");  /* step over */
			sscanf(pos, "%u", &slurm_jobid);
		}

		if(NULL != (pos = strstr(*tmp_jobid_argv, "job_return_code="))){
			pos = pos + strlen("job_return_code=");  /* step over*/
			sscanf(pos, "%u", &job_return_code);
		}

		/* Locks: Write job, write node */
		slurmctld_lock_t job_write_lock = {
			NO_LOCK, WRITE_LOCK, WRITE_LOCK, NO_LOCK
		};
		lock_slurmctld(job_write_lock);
		rc = job_complete(slurm_jobid, uid, job_requeue,
							node_fail, job_return_code);
		unlock_slurmctld(job_write_lock);

		/* return result */
		if (rc) {
			info("deallocate JobId=%u: %s ",
					slurm_jobid, slurm_strerror(rc));
			sprintf(tmp_buf,
					"slurm_jobid=%d  deallocation failed",
					slurm_jobid);
		} else {
			debug2("deallocate JobId=%u ", slurm_jobid);
			(void) schedule_job_save();	/* Has own locking */
			(void) schedule_node_save();	/* Has own locking */
			sprintf(tmp_buf,
					"slurm_jobid=%d deallocation successful",
					slurm_jobid);
		}

		/* to create the send_buf if more than one job to deallocate */
		if(0 == strlen(send_buf)){
			strcpy(send_buf, tmp_buf);
		}else{
			strcat(send_buf, ":");
			strcat(send_buf, tmp_buf);
		}
		/*step to the next */
		tmp_jobid_argv++;
	}
	/* free app_argv */
	argv_free(jobid_argv);

	send_reply(new_fd, send_buf);

	return SLURM_SUCCESS;
}
