#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <evfibers/fiber.h>

ssize_t
sctp_sendmsgav(int s, struct iovec *iovp, size_t iov_len, struct sockaddr *to,
	     socklen_t tolen, uint32_t ppid, uint32_t flags,
	     uint16_t stream_no, uint32_t timetolive, uint32_t context)
{
	struct msghdr outmsg;
	char outcmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
	struct cmsghdr *cmsg;
	struct sctp_sndrcvinfo *sinfo;

	outmsg.msg_name = to;
	outmsg.msg_namelen = tolen;
	outmsg.msg_iov = iovp;
	outmsg.msg_iovlen = iov_len;

	outmsg.msg_control = outcmsg;
	outmsg.msg_controllen = sizeof(outcmsg);
	outmsg.msg_flags = 0;

	cmsg = CMSG_FIRSTHDR(&outmsg);
	cmsg->cmsg_level = IPPROTO_SCTP;
	cmsg->cmsg_type = SCTP_SNDRCV;
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

	outmsg.msg_controllen = cmsg->cmsg_len;
	sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
	memset(sinfo, 0, sizeof(struct sctp_sndrcvinfo));
	sinfo->sinfo_ppid = ppid;
	sinfo->sinfo_flags = flags;
	sinfo->sinfo_stream = stream_no;
	sinfo->sinfo_timetolive = timetolive;
	sinfo->sinfo_context = context;

	return sendmsg(s, &outmsg, 0);
}

ssize_t
fbr_sctp_recvmsg(struct fbr_context * fctx, int s, void *msg, size_t len,
		struct sockaddr *from, socklen_t *fromlen, struct sctp_sndrcvinfo *sinfo,
		int *msg_flags)
{
	int error;
	struct iovec iov;
	struct msghdr inmsg;
	char incmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
	struct cmsghdr *cmsg = NULL;

	memset(&inmsg, 0, sizeof (inmsg));

	iov.iov_base = msg;
	iov.iov_len = len;

	inmsg.msg_name = from;
	inmsg.msg_namelen = fromlen ? *fromlen : 0;
	inmsg.msg_iov = &iov;
	inmsg.msg_iovlen = 1;
	inmsg.msg_control = incmsg;
	inmsg.msg_controllen = sizeof(incmsg);

	error = fbr_recvmsg(fctx, s, &inmsg, msg_flags ? *msg_flags : 0);
	if (error < 0)
		return error;

	if (fromlen)
		*fromlen = inmsg.msg_namelen;
	if (msg_flags)
		*msg_flags = inmsg.msg_flags;

	if (!sinfo)
		return error;

	for (cmsg = CMSG_FIRSTHDR(&inmsg); cmsg != NULL;
				 cmsg = CMSG_NXTHDR(&inmsg, cmsg)){
		if ((IPPROTO_SCTP == cmsg->cmsg_level) &&
		    (SCTP_SNDRCV == cmsg->cmsg_type))
			break;
	}

        /* Copy sinfo. */
	if (cmsg)
		memcpy(sinfo, CMSG_DATA(cmsg), sizeof(struct sctp_sndrcvinfo));

	return (error);
}

uint16_t
stcp_get_event_type(void *buf) {
	union sctp_notification  *snp;
	snp = buf;
	return snp->sn_header.sn_type;
}

void
sctp_handle_event(const char *id, void *buf)
{
    struct sctp_assoc_change *sac;
    struct sctp_send_failed  *ssf;
    struct sctp_paddr_change *spc;
    struct sctp_remote_error *sre;
    union sctp_notification  *snp;
    char    addrbuf[INET6_ADDRSTRLEN];
    const char   *ap;
    struct sockaddr_in  *sin;
    struct sockaddr_in6  *sin6;

    snp = buf;

    switch (snp->sn_header.sn_type) {
    case SCTP_ASSOC_CHANGE:
        sac = &snp->sn_assoc_change;
        printf("%s assoc_change: state=%hu, error=%hu, instr=%hu "
            "outstr=%hu\n", id, sac->sac_state, sac->sac_error,
            sac->sac_inbound_streams, sac->sac_outbound_streams);
        break;
    case SCTP_SEND_FAILED:
        ssf = &snp->sn_send_failed;
        printf("%s sendfailed: len=%hu err=%d\n", id, ssf->ssf_length,
            ssf->ssf_error);
        break;
    case SCTP_PEER_ADDR_CHANGE:
        spc = &snp->sn_paddr_change;
        if (spc->spc_aaddr.ss_family == AF_INET) {
            sin = (struct sockaddr_in *)&spc->spc_aaddr;
            ap = inet_ntop(AF_INET, &sin->sin_addr, addrbuf,
                INET6_ADDRSTRLEN);
        } else {
            sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
            ap = inet_ntop(AF_INET6, &sin6->sin6_addr, addrbuf,
                INET6_ADDRSTRLEN);
        }
        printf("%s intf_change: %s state=%d, error=%d\n", id, ap,
            spc->spc_state, spc->spc_error);
        break;
    case SCTP_REMOTE_ERROR:
        sre = &snp->sn_remote_error;
        printf("%s remote_error: err=%hu len=%hu\n", id,
            ntohs(sre->sre_error), ntohs(sre->sre_length));
        break;
    case SCTP_SHUTDOWN_EVENT:
        printf("%s shutdown event\n", id);
        break;
    case SCTP_SENDER_DRY_EVENT:
    	printf("%s sender dry event\n", id);
    	break;
    default:
        printf("%s unknown event type: %hu\n", id, snp->sn_header.sn_type);
        break;
    }
}
