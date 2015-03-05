#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <netdb.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <linux/sockios.h>

#include "sctp_util.h"

#define LOCAL_PORT 5050
#define BUFLEN ((63 * 1024) - 40)
#define PPID 1111

int main(int argc, char *argv[])
{
	struct protoent *pe;
	int s;
	int size;
	socklen_t len;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int used;
	int flags;
	int fh;
	struct sockaddr_in toaddr;
	struct sockaddr_in fromaddr;
	struct sctp_event_subscribe ses;
	char *data;
	char *hdr = "hdr";
	int mypid = getpid();
	char namebuf[BUFSIZ];
	struct timeval tv1;
	struct timeval tv2;
	double t;
	double mb = 0;

    struct iovec iov[2];
    //struct sctp_sndrcvinfo *sinfo;
    //struct sctp_status sctp_stat;
    struct sctp_sndrcvinfo rinfo;

	pe = getprotobyname("sctp");
	s = socket(AF_INET, SOCK_SEQPACKET, pe->p_proto);
	if (s < 0) {
		printf("error creating local socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	memset(&ses, 0, sizeof (ses));
	ses.sctp_data_io_event = 1;
	ses.sctp_association_event = 1;
	ses.sctp_send_failure_event = 1;
	ses.sctp_sender_dry_event = 1;
	setsockopt(s, IPPROTO_SCTP, SCTP_EVENTS, &ses, sizeof (ses));

	len = sizeof(size);
    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &size, &len) < 0) {
    	printf("getsockopt error: %s\n", strerror(errno));
    	exit(EXIT_FAILURE);
    }
    printf("SO_SNDBUF = %d\n", size);

	memset(&toaddr, 0, addr_len);
	toaddr.sin_family = AF_INET;
	toaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	toaddr.sin_port = htons(LOCAL_PORT);

	data = (char *)malloc(BUFLEN);

	iov[0].iov_base = (void *)hdr;
	iov[0].iov_len = strlen(hdr);
	iov[1].iov_base = (void *)data;

	fh = open(argv[1]);
	used = read(fh, data, BUFLEN);
	mb += used / 1024.0 / 1024;

	memset(&fromaddr, 0, sizeof(struct sockaddr_in));
	flags = 0;
	sprintf(namebuf, "client-%d:", mypid);

	gettimeofday(&tv1, NULL);
	while (used > 0) {
		iov[1].iov_len = used;
		if (sctp_sendmsgav(s, iov, 2, (struct sockaddr *)&toaddr, addr_len, htonl(PPID),
				0, 0, 0, 0) < 0) {
			printf("error sending message: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		printf("%s sent %d bytes\n", namebuf, used);

//		do {
//			if (stcp_get_event_type(data) == SCTP_SENDER_DRY_EVENT)
//				break;
//			if (sctp_recvmsg(s, data, BUFLEN, (struct sockaddr *)&fromaddr, &addr_len,
//					&rinfo, &flags) < 0) {
//				printf("error receiving message: %s\n", strerror(errno));
//				exit(EXIT_FAILURE);
//			}
//			sctp_handle_event(namebuf, data);
//		} while (flags & MSG_NOTIFICATION);
		used = read(fh, (void *)data, BUFLEN);
		mb += used / 1024.0 / 1024;
	}
	gettimeofday(&tv2, NULL);
	t = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec) * 1e-6;
	printf("elapsed time %.3f\n", t);
	printf("speed is %.2f mb/sec\n", mb / t);
//	if (sctp_recvmsg(s, data, BUFLEN, (struct sockaddr *)&fromaddr, &addr_len,
//			&rinfo, &flags) < 0) {
//		printf("error receiving message: %s\n", strerror(errno));
//		exit(EXIT_FAILURE);
//	}

//	len = sizeof(sctp_stat);
//	memset(&sctp_stat, 0, len);
//	if (sctp_opt_info(s, sinfo->sinfo_assoc_id, SCTP_STATUS, &sctp_stat, &len) < 0) {
//    	printf("sctp_opt_info error: %s\n", strerror(errno));
//    	exit(EXIT_FAILURE);
//    }
//    printf("pending = %d\n", sctp_stat.sstat_penddata);
	free(data);
	exit(EXIT_SUCCESS);
}
