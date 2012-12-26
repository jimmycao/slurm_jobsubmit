/*
 * ip_port.c
 *
 *  Created on: Dec 20, 2012
 *      Author: caoj7
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>

#include "slurm/slurm_errno.h"
#include "ip_port.h"

#define SIZE 256

int get_local_eth0_ip(char **ip)
{
	struct ifaddrs *ifaddr, *ifa;
	int rc;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) < 0) {
		error("getifaddrs");
		return -1;
	}

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        if (ifa->ifa_addr == NULL)
            continue;
        rc = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
        				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if((strcmp(ifa->ifa_name,"eth0") == 0) &&
        		(ifa->ifa_addr->sa_family == AF_INET)) {
            if (rc != 0) {
                error("getnameinfo() failed");
                return -1;
            }
            *ip = strdup(host);
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}


int write_local_ip_port(uint16_t port)
{
	FILE *fp;
	char *str;
	char *ip;

	if(fp = fopen(IP_CONFIG_FILE_PATHNAME, "w"), fp == NULL){
		error("fopen failure");
		return -1;
	}

	asprintf(&str, "port=%u\n", port);
	fputs(str, fp);
	get_local_eth0_ip(&ip);
	asprintf(&str, "ip=%s\n", ip);
	fputs(str, fp);

	fclose(fp);
	return 0;
}


int read_ip_port(uint16_t *port, char **ip)
{
	FILE *fp;
	char line[SIZE];
	char *pos;
	char *tmp;

	if(fp = fopen(IP_CONFIG_FILE_PATHNAME, "r"), fp == NULL){
		error("fopen failure");
		return -1;
	}

	while(fgets(line, SIZE, fp) != NULL){
		if(strncmp(line, "ip", 2) == 0){
			tmp = strstr(line, "=") + 1;
			/*instead '\n' with '\0' */
			tmp[strlen(tmp)-1] = '\0';
//			asprintf(ip, "%s", tmp);
			*ip = strdup(tmp);
		}else if(strncmp(line, "port", 4) == 0){
			pos = strstr(line, "=");
			*port = atoi(pos+1);
		}
	}

	fclose(fp);
	return 0;
}
