#ifndef __sctp_util_h__
#define __sctp_util_h__

void
sctp_handle_event(const char *id, void *buf);

int
sctp_sendmsgav(int s, struct iovec *iovp, size_t iov_len, struct sockaddr *to,
	     socklen_t tolen, uint32_t ppid, uint32_t flags,
	     uint16_t stream_no, uint32_t timetolive, uint32_t context);

uint16_t
stcp_get_event_type(void *buf);

#endif
