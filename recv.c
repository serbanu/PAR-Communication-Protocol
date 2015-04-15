#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

//Function that computes parity of a byte
int compute_byte_parity(char c);
//Function that computes parity of a string
int compute_checksum(char* c, int len);
//Function that converts an int to a char
char convert(int seq_no);
//Computes the timestamp
void my_get_time(FILE *log);
//Convert from base 10 to base 2
void convert_to_base_2(FILE* log, unsigned char ptr);
//Print to log file about a sent message
void print_to_log_sent(FILE* log, frame f);
//Print to log file about a received message
void print_to_log_recv(FILE* log, frame f);

int main(int argc,char** argv){
  init(HOST,PORT);
	
	//Declarations	
	frame f;
	msg s, r, *t;
	int seq = 1;
	char checksum;
	char *filename;
	FILE *file, *log;
	char finish[12] = "End of file";

	//Open the log file
	log = fopen("log.out", "a");
	
	if (recv_message(&r) < 0) {
		perror("[RECEIVER] Oops");
		return -1;
	} else {
		memset(f.payload, 0, sizeof(f.payload));
  	memcpy(&f, r.payload, r.len);
		print_to_log_recv(log, f);
	}
	
	int size_so_far = strlen(f.payload);
	filename = calloc(size_so_far, sizeof(char));
	
	
	while(1) {
		checksum = convert(compute_checksum(f.payload, strlen(f.payload)));
		if(checksum == f.checksum) {
			fprintf(log, "Checksum-ul local se potriveste; Trimit ACK-CORRECT\n");
			strcat(filename, f.payload);
			memset(f.payload, 0, sizeof(f.payload));
			memset(s.payload, 0, sizeof(s.payload));
			f.seq_no = convert(seq);
			f.checksum = convert(checksum);
			s.len = sizeof(f);
			memcpy(s.payload, &f, s.len);
			send_message(&s);
			break;
		} else {
				fprintf(log, "Checksum-ul local nu se potriveste; Trimit ACK-CORRUPT\n");
				memset(f.payload, 0, sizeof(f.payload));
				memset(s.payload, 0, sizeof(s.payload));
				f.seq_no = convert(seq - 1);
				f.checksum = convert(checksum);
				s.len = sizeof(f);
				memcpy(s.payload, &f, s.len);
				send_message(&s);
			
				if (recv_message(&r) < 0) {
					perror("[RECEIVER] Oops");
					return -1;
				} else {
					memset(f.payload, 0, sizeof(f.payload));
					memcpy(&f, r.payload, r.len);
					print_to_log_recv(log, f);
				}
			}
		fprintf(log, "=================================================\n");
	}
	fprintf(log, "=================================================\n");
	seq++;
	seq %= 10;
	t = receive_message_timeout(1000);
	if(t == NULL) {
		fprintf(log,"[RECV] Received the name of the file in one message\n");
	} else {
			memset(f.payload, 0, strlen(f.payload));
			memcpy(&f, t->payload, t->len);
	  	size_so_far += strlen(f.payload);
	  	filename = realloc(filename, size_so_far);
			memset(f.payload, 0, sizeof(f.payload));
			memset(s.payload, 0, sizeof(s.payload));
			f.seq_no = convert(seq);
			f.checksum = checksum;
			s.len = sizeof(f);
			memcpy(s.payload, &f, s.len);
  		send_message(&s);
			seq++;
			seq %= 10;
	}
	
	//Open the file to write to received content
	
		filename = realloc(filename, size_so_far + 5);
		strcat(filename, ".out");
		file = fopen(filename, "w");
	
	while(1) {
		//Reinitialize the payload
		memset(t->payload, 0, sizeof(t->payload));
		
		//Waiting to receive a message
		t = receive_message_timeout(5000);
		//While a message did not come, resend the same message
		while(!t) {
			my_get_time(log);
			fprintf(log, "Message timeout. Resending the ACK...\n");
			send_message(&s);
			t = receive_message_timeout(5000);
		}

		//If I've received a message, I check to see whether it's a proper one or not
		memset(f.payload, 0, sizeof(f.payload));
		memcpy(&f, t->payload, t->len);
		checksum = convert(compute_checksum(f.payload, strlen(f.payload)));
		
		if(f.seq_no == convert(seq)) {
			print_to_log_recv(log, f);
			if(f.checksum == checksum) {
				if(strcmp(f.payload, finish) == 0) {
					fprintf(log, "%s", f.payload);
					memset(f.payload, 0, sizeof(f.payload));
					memset(s.payload, 0, sizeof(s.payload));
					f.seq_no = convert(seq);
					f.checksum = checksum;
					s.len = sizeof(f);
					memcpy(s.payload, &f, s.len);
					send_message(&s);
					break;
				}
				
				//Send ACK-CORRECT
				fprintf(log, "Sequence number-ul se potriveste;");
				fprintf(log, "Checksum-ul local se potriveste; Trimit ACK-CORRECT\n");
				fprintf(file, "%s", f.payload);
				memset(f.payload, 0, sizeof(f.payload));
				memset(s.payload, 0, sizeof(s.payload));
				f.seq_no = convert(seq);
				f.checksum = checksum;
				s.len = sizeof(f);
				memcpy(s.payload, &f, s.len);
				send_message(&s);
				seq++;
				seq %= 10;
			} else {
				fprintf(log, "Sequence number-ul se potriveste;");
				fprintf(log, "Checksum-ul local nu se potriveste; Trimit ACK-CORRUPT\n");
				memset(f.payload, 0, sizeof(f.payload));
				memset(s.payload, 0, sizeof(s.payload));
				f.seq_no = convert(seq);
				f.checksum = checksum;
				s.len = sizeof(f);
				memcpy(s.payload, &f, s.len);
				send_message(&s);
			}
			fprintf(log, "=================================================\n");
		} else {
			print_to_log_recv(log, f);
			fprintf(log, "Duplicat!\n");
			fprintf(log, "Retrimit ACK ...\n");
			fprintf(log, "=================================================\n");
			
			memset(f.payload, 0, sizeof(f.payload));
			memset(s.payload, 0, sizeof(s.payload));
			f.seq_no = convert(seq++);
			seq %= 10;
			f.checksum = convert(checksum);
			s.len = sizeof(f);
			memcpy(s.payload, &f, s.len);
  		send_message(&s);
		}
	}
	fprintf(log, "=================================================\n");
	
	return 0;
}

int compute_byte_parity(char c) {
	unsigned char mask = MASK;
	int i, result = (c & mask) != 0;
	mask >>= 1;
	
	for (i = 0; i < BITS_NO - 1; i++) {
		result ^= (c & mask) != 0;
		mask >>= 1;
	}
	
	return result;
}

int compute_checksum(char *data, int len) {
	int i, result = compute_byte_parity(data[0]);
	for (i = 1; i < len; i++) {
		result ^= compute_byte_parity(data[i]);
	}
	
	return result;
}

char convert(int my_int) {
	return (char) (((int) '0') + my_int); 
}

void my_get_time(FILE* log) {
	time_t rawtime;
  struct tm * timeinfo;
	int i = 0;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
	char* time_array = calloc(strlen(asctime(timeinfo)), sizeof(char));
	char** time_buf = calloc(5, sizeof(char*));
	for(i = 0; i < 5; i++) {
		time_buf[i] = calloc(10, sizeof(char));
	}
	
	strcat(time_array, asctime(timeinfo));	
	char *tok = strtok(time_array, " \n");
	i = 0;
	while(tok != NULL) {
		time_buf[i++] = tok;
		tok = strtok(NULL, " \n");
	}
	
	if(strcmp(time_buf[1], "Mar") == 0) {
		fprintf(log, "[%s-%s-%s %s] [RECV]  ", time_buf[2], "03", time_buf[4], time_buf[3]);
	}
}

void convert_to_base_2(FILE* log, unsigned char ptr) {
	unsigned char b = ptr;
	unsigned char byte;
	int i = 0;

	for(i = 7; i >= 0; i--) {
		byte = b & (1 << i);
		byte >>= i;
		fprintf(log, "%u", byte);
	}
	fprintf(log, "\n");
}

void print_to_log_sent(FILE* log, frame f) {
	my_get_time(log);
	fprintf(log, "Am trimis urmatorul pachet [ACK]:\n");
	fprintf(log, "Seq No: %c\n", f.seq_no);
	fprintf(log, "Checksum: %c\n", f.seq_no);
}

void print_to_log_recv(FILE* log, frame f) {
	my_get_time(log);
	fprintf(log, "Am primit urmatorul pachet:\n");
	fprintf(log, "Seq No: %c\n", f.seq_no);
	fprintf(log, "Payload: %s\n", f.payload);
	fprintf(log, "Checksum: ");
	convert_to_base_2(log, f.checksum);
}
