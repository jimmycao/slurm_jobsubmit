/*
 * allocator.h
 *
 *  Created on: Jan 6, 2013
 *      Author: caoj7
 */

#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

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
#include "msg.h"


extern int allocate_job_op(slurm_fd_t new_fd, char *msg);

#endif /* ALLOCATOR_H_ */
