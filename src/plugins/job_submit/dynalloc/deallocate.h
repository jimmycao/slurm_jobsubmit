/*
 * deallocate.h
 *
 *  Created on: Jan 22, 2013
 *      Author: caoj7
 */

#ifndef DEALLOCATE_H_
#define DEALLOCATE_H_

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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "msg.h"

extern int deallocate(slurm_fd_t new_fd, char *msg);

#endif /* DEALLOCATE_H_ */
