#define _GNU_SOURCE
#include "includes.h"
#include "oas.h"

static int (*real_connect)(int socket, __CONST_SOCKADDR_ARG addr,
			   int length) = NULL;
extern int connect(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	openlog("preloaded oas", LOG_PID, LOG_USER);
	char *loglevel = getenv("OAS_LOGLEVEL");
	int orig_log_mask = setlogmask(LOG_UPTO(LOG_NOTICE));
	if (loglevel != NULL) {
		if (!strncasecmp("DEBUG", loglevel, 5)) {
			setlogmask(LOG_UPTO(LOG_DEBUG));
		} else if (!strncasecmp("INFO", loglevel, 4)) {
			setlogmask(LOG_UPTO(LOG_INFO));
		} else if (!strncasecmp("NOTICE", loglevel, 6)) {
			setlogmask(LOG_UPTO(LOG_NOTICE));
		} else if (!strncasecmp("WARN", loglevel, 4)) {
			setlogmask(LOG_UPTO(LOG_WARNING));
		} else if (!strncasecmp("ERR", loglevel, 3)) {
			setlogmask(LOG_UPTO(LOG_ERR));
		} else if (!strncasecmp("CRIT", loglevel, 4)) {
			setlogmask(LOG_UPTO(LOG_CRIT));
		} else if (!strncasecmp("ALERT", loglevel, 5)) {
			setlogmask(LOG_UPTO(LOG_ALERT));
		} else if (!strncasecmp("EMERG", loglevel, 5)) {
			setlogmask(LOG_UPTO(LOG_EMERG));
		}
	}

	real_connect = dlsym(RTLD_NEXT, "connect");
	const struct sockaddr *target = (*((struct sockaddr **)&__addr));
	short int family = target->sa_family;
	if (family == AF_INET6) {
		const struct sockaddr_in6 *target6 =
			(const struct sockaddr_in6 *)target;
		char targetaddrtext[50];
		char sourceaddrtext[50];
		inet_ntop(target6->sin6_family,
			  (const void *restrict)(&(target6->sin6_addr)),
			  targetaddrtext, 50);
		struct ifaddrs *iflist;
		if (getifaddrs(&iflist) == -1) {
			syslog(LOG_WARNING,
			       "could not retrieve available addresses");
		} else {
			struct ifaddrs *next = iflist;
			int bestscore = 10000;
			struct sockaddr_in6 *bestaddr = NULL;
			struct sockaddr_in6 *curr = NULL;
			while (next != NULL) {
				if (next->ifa_addr != NULL &&
				    next->ifa_addr->sa_family == AF_INET6) {
					curr = (struct sockaddr_in6 *)
						       next->ifa_addr;
					int score = oas_address_score(
						curr->sin6_addr.s6_addr);
					if (score < bestscore) {
						bestscore = score;
						bestaddr = curr;
						if (bestscore == 1)
							break; // no better match possible
					}
				}
				next = next->ifa_next;
			}
			// try to bind if address found
			if (bestscore < 10000 && bestaddr != NULL) {
				inet_ntop(AF_INET6,
					  (const void *restrict)(
						  &(bestaddr->sin6_addr)),
					  sourceaddrtext, 50);
				syslog(LOG_NOTICE,
				       "oas selected %s as source to connect to %s",
				       sourceaddrtext, targetaddrtext);
				int br = bind(
					__fd,
					((const struct sockaddr *)bestaddr),
					sizeof(struct sockaddr_in6));
				syslog(LOG_DEBUG,
				       "bind returns: %d with errno: %d", br,
				       errno);
			}
			freeifaddrs(iflist);
		}
	}
	setlogmask(orig_log_mask);
	closelog();
	return real_connect(__fd, __addr, __len);
}
