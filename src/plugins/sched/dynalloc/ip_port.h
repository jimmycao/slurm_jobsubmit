/*
 * ip_port.h
 *
 *  Created on: Dec 20, 2012
 *      Author: caoj7
 */

#ifndef IP_PORT_H_
#define IP_PORT_H_

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

#define IP_CONFIG_FILE_PATHNAME "/tmp/ip.config"

extern int get_local_eth0_ip(char **ip);

extern int write_local_ip_port(uint16_t port);

extern int read_ip_port(uint16_t *port, char **ip);

#endif /* IP_PORT_H_ */
