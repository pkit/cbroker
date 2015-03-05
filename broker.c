#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <ev.h>
#include <evfibers/fiber.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <fcntl.h>

#include "sctp_util.h"

#define LOCAL_PORT 5050
#define REMOTE_PORT 5051
#define BUFLEN ((64 * 1024) - 1)
#define SOBUFSIZE 212992
#define BUFWIDTH (1 << 2)

static void read_data(struct fbr_context *fctx, void *_arg)
{
	int fd = *(int *)_arg;
	int data_len;
	socklen_t peer_len;
	struct sockaddr_in peer_addr;
	fbr_id_t id;
	int flags = 0;
	struct sctp_sndrcvinfo rinfo;
	char buf[BUFSIZ];

	peer_len = sizeof(peer_addr);
	memset(&rinfo, 0, sizeof(struct sctp_sndrcvinfo));
	for (;;) {
		data_len = fbr_sctp_recvmsg(fctx, fd, &buf, BUFSIZ, &peer_addr, &peer_len,
				&rinfo, &flags);
		if (data_len < 0)
			err(EXIT_FAILURE, "recvmsg failed");

		printf("broker-%d: data received, len = %d eor = %d\n",
				rinfo.sinfo_assoc_id, data_len, (flags & MSG_EOR));
		if (flags & MSG_EOR)
			break;
	}
}

static void rcv_local(struct fbr_context *fctx, void *_arg)
{
	int fd = *(int *)_arg;
	int data_len;
	socklen_t peer_len;
	struct sockaddr_in peer_addr;
	fbr_id_t id;
	int flags = 0;
	struct sctp_sndrcvinfo rinfo;
	char *buf[BUFWIDTH];
	char namebuf[BUFSIZ];
	fbr_id_t read_data_id;
	int i = 0;

	peer_len = sizeof(peer_addr);
	memset(&rinfo, 0, sizeof(struct sctp_sndrcvinfo));
	for (i = 0; i < 4; ++i)
		buf[i] = (char *)malloc(BUFLEN);
	i = 0;
	for (;;) {
		data_len = fbr_sctp_recvmsg(fctx, fd, buf[i], BUFLEN, &peer_addr, &peer_len,
				&rinfo, &flags);
		if (data_len < 0)
			err(EXIT_FAILURE, "recvmsg failed");
		if (flags & MSG_NOTIFICATION) {
			sprintf(namebuf, "broker-%d", rinfo.sinfo_assoc_id);
			sctp_handle_event(namebuf, buf[i]);
		} else {
			printf("broker-%d: data received, len = %d eor = %d\n",
					rinfo.sinfo_assoc_id, data_len, (flags & MSG_EOR));
//			read_data_id = fbr_create(fctx, "read_data", read_data, &fd, 0);
//			if (fbr_id_isnull(read_data_id))
//				errx(EXIT_FAILURE, "unable to create read_data fiber");
//			fbr_transfer(fctx, read_data_id);
		}
		i = (++i) & (BUFWIDTH - 1);
	}
}

int main(int argc, char *argv[])
{
	struct fbr_context fbr;
	struct protoent *pe;
	int sl, sr;
	struct sockaddr_in sar;
	fbr_id_t rcv_local_id;
	struct sctp_event_subscribe ses;
	int sflags;
	int sndrcvbufsize = SOBUFSIZE;

	fbr_init(&fbr, EV_DEFAULT);
	signal(SIGPIPE, SIG_IGN);
	pe = getprotobyname("sctp");
	sl = socket(AF_INET, SOCK_SEQPACKET, pe->p_proto);
	if (sl < 0) {
		sl = errno;
		printf("error creating local socket: %s\n", strerror(sl));
		exit(EXIT_FAILURE);
	}

	memset(&ses, 0, sizeof (ses));
	ses.sctp_data_io_event = 1;
	ses.sctp_association_event = 1;
	ses.sctp_send_failure_event = 1;
	ses.sctp_sender_dry_event = 1;
	setsockopt(sl, IPPROTO_SCTP, SCTP_EVENTS, &ses, sizeof (ses));

	setsockopt(sl, SOL_SOCKET, SO_SNDBUF, &sndrcvbufsize, sizeof(int));
	setsockopt(sl, SOL_SOCKET, SO_RCVBUF, &sndrcvbufsize, sizeof(int));

	memset(&sar, 0, sizeof(struct sockaddr_in));
	sar.sin_family = AF_INET;
	sar.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sar.sin_port = htons(LOCAL_PORT);
	bind(sl, (struct sockaddr *)&sar, sizeof(struct sockaddr_in));

	sflags = fcntl(sl, F_GETFL);
	sflags |= O_NONBLOCK;
	fcntl(sl, F_SETFL, sflags);

	listen(sl, 10);
	printf("listening\n");

	rcv_local_id = fbr_create(&fbr, "rcv_local", rcv_local, &sl, 0);
	if (fbr_id_isnull(rcv_local_id))
		errx(EXIT_FAILURE, "unable to create rcv_local fiber");
	fbr_transfer(&fbr, rcv_local_id);

	ev_run(EV_DEFAULT, 0);

	close(sl);
}
