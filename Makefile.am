bin_PROGRAMS = broker client

broker_SOURCES = broker.c sctp_util.c
broker_LDADD = -levfibers -lev -lsctp

client_SOURCES = client.c sctp_util.c
client_LDADD = -levfibers -lev -lsctp