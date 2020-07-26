/*
 * === Network functions ===
 *
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "data.h"
#include "net.h"

#include "libfuncs/libfuncs.h"

int ai_family = AF_UNSPEC;
extern int timeout;

static char *my_inet_ntop(int family, struct sockaddr *addr, char *dest, int dest_len) {
	struct sockaddr_in  *addr_v4 = (struct sockaddr_in  *)addr;
	struct sockaddr_in6 *addr_v6 = (struct sockaddr_in6 *)addr;
	switch (family) {
		case AF_INET:
			return (char *)inet_ntop(AF_INET, &addr_v4->sin_addr, dest, dest_len);
			break;
		case AF_INET6:
			return (char *)inet_ntop(AF_INET6, &addr_v6->sin6_addr, dest, dest_len);
			break;
		default:
			memset(dest, 0, dest_len);
			strcpy(dest, "unknown");
			return dest;
	}
}

int connect_client(int socktype, const char *hostname, const char *service) {
	struct addrinfo hints, *res;
	int n;

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = ai_family;
	hints.ai_socktype = socktype;

	d("Connecting to server %s port %s\n", hostname, service);

	n = getaddrinfo(hostname, service, &hints, &res);

	if (n < 0) {
		fprintf(stderr, "ERROR: getaddrinfo(%s): %s\n", hostname, gai_strerror(n));
		return -1;
	}

	int sockfd = -1;
	struct addrinfo *ressave = res;
	char str_addr[INET6_ADDRSTRLEN] = { 0 };
	while (res) {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd > -1) {
			my_inet_ntop(res->ai_family, res->ai_addr, str_addr, sizeof(str_addr));
			if (do_connect(sockfd, res->ai_addr, res->ai_addrlen, timeout * 1000) < 0) {
				fprintf(stderr, "ERROR: Cant connect to server %s port %s (addr=%s) | %s\n",
					hostname, service, str_addr, strerror(errno));
				close(sockfd);
				sockfd = -1;
			} else {
				break; // connected
			}
		} else {
			fprintf(stderr, "ERROR: Could not create socket: %s\n", strerror(errno));
			sleep(1); // 1 second between socket creation (after error)
			return -1;
		}
		res = res->ai_next;
	}
	freeaddrinfo(ressave);

	if (sockfd < 0)
		return -1;

	if (socktype == SOCK_STREAM) {
		int flag = 1;
		setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
	}

	d("Connected to server %s port %s (addr=%s fd=%d).\n",
		hostname, service, str_addr, sockfd);

	set_sock_nonblock(sockfd);

	return sockfd;
}
