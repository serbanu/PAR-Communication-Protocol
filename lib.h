#ifndef LIB
#define LIB

#define MASK			1 << 7
#define	BITS_NO		8

typedef struct {
	char checksum;
	char seq_no;
	char payload[60];
} frame;

typedef struct {
  int len;
  char payload[1400];
} msg;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout);

#endif

