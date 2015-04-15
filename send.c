#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define get_min(a,b) ((a) < (b) ? (a) : (b))
#define my_timeout 5000

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
	msg s, *r;
	int seq = 1;	
	//int fd;
	int random;
	int min_bytes;
	int fd;
	char buffer[60];
	char finish[12] = "End of file";

	FILE *log;
	
	//Open the log file
	log = fopen("log.out", "a");
	
	//Generate `random` number of bytes
	srand(time(NULL));
	random = rand() % 60; 

	//Get the minimum number of bytes to send for the file name
	min_bytes = get_min(random, strlen(argv[1]));

	//If the minimum number of bytes is equal to the random generated number
	if(min_bytes == random) {
	
		//We must send the name of the file in 2 steps
		//First, we send `min_bytes` bytes of the name
		while(1) {
			//Reasamblez si retrimit mesajul
			memset(f.payload, 0, sizeof(f.payload));
			memset(s.payload, 0, sizeof(s.payload));
			memcpy(f.payload, argv[1], min_bytes);
			f.checksum = convert(compute_checksum(f.payload, min_bytes));
			f.seq_no = convert(seq);
			memcpy(s.payload, &f, sizeof(f));
			s.len = sizeof(f);
			send_message(&s);
			print_to_log_sent(log, f);
			fprintf(log, "=================================================\n");
			sleep(1);
			r = receive_message_timeout(my_timeout);
			while(!r) {
				send_message(&s);
				r = receive_message_timeout(my_timeout);
			}
			memset(f.payload, 0, sizeof(f.payload));
			memcpy(&f, r->payload, r->len);
			if(f.seq_no == convert(seq)) {
				print_to_log_recv(log, f);
				fprintf(log, "=================================================\n");
				break;
			}
			print_to_log_recv(log, f);
			fprintf(log, "ACK-CORRUPT!\n");
			fprintf(log, "Retrimit mesajul ...\n");
			fprintf(log, "=================================================\n");
			break;
		}
		
		seq++;	//Increment the sequence number
		seq %= 10;
		
		//Then, we send the rest of the bytes
		while(1) {
			//Reasamblez si retrimit mesajul
			memset(f.payload, 0, sizeof(f.payload));
			memset(s.payload, 0, sizeof(s.payload));
			memcpy(f.payload, argv[1], min_bytes);
			int remaining_bytes = strlen(argv[1]) - min_bytes;
			char rem[remaining_bytes];
			int i = 0;
			for(i = 0; i < remaining_bytes; i++) {
				rem[i] = argv[1][i + min_bytes];
			} 
			memcpy(f.payload, rem, remaining_bytes);
			f.checksum = convert(compute_checksum(f.payload, remaining_bytes));
			f.seq_no = convert(seq);
			memcpy(s.payload, &f, sizeof(f));
			s.len = sizeof(f);
			send_message(&s);
			print_to_log_sent(log, f);
			fprintf(log, "=================================================\n");
			
			sleep(1);
			r = receive_message_timeout(my_timeout);
			while(!r) {
				send_message(&s);
				r = receive_message_timeout(my_timeout);
			}
			memset(f.payload, 0, sizeof(f.payload));
			memcpy(&f, r->payload, r->len);
			if(f.seq_no == convert(seq)) {
				print_to_log_recv(log, f);
				fprintf(log, "=================================================\n");
				break;
			}
			print_to_log_recv(log, f);
			fprintf(log, "ACK-CORRUPT!\n");
			fprintf(log, "Retrimit mesajul ...\n");
			fprintf(log, "=================================================\n");
			break;
		}
		seq++;
		seq %= 10;
	} else {	//Else, if the minimum number of bytes is equal to the length of the filename
		//Send the message just once and also, wait just once for the ACK
		while(1) {
			//Reasamblez si retrimit mesajul
			memset(f.payload, 0, sizeof(f.payload));
			memset(s.payload, 0, sizeof(s.payload));
			memcpy(f.payload, argv[1], min_bytes);
			f.checksum = convert(compute_checksum(f.payload, min_bytes));
			f.seq_no = convert(seq);
			memcpy(s.payload, &f, sizeof(f));
			s.len = sizeof(f);
			send_message(&s);
			print_to_log_sent(log, f);
			fprintf(log, "=================================================\n");
			r = receive_message_timeout(my_timeout);
			while(!r) {
				send_message(&s);
				r = receive_message_timeout(my_timeout);
			}
			memset(f.payload, 0, sizeof(f.payload));
			memcpy(&f, r->payload, r->len);
			if(f.seq_no == convert(seq - 1)) {
				print_to_log_recv(log, f);
				fprintf(log, "=================================================\n");
				break;
			}
			print_to_log_recv(log, f);
			fprintf(log, "ACK-CORRUPT!\n");
			fprintf(log, "Retrimit mesajul ...\n");
			fprintf(log, "=================================================\n");
		}
		seq++;
		seq %= 10;
	}

	//Opening the file to send it's content
	fd = open(argv[1], O_RDONLY);
	int bytes_read = 0;
	//int iterations_until_timeout = 0;
	random = rand() % 60;

	//While there are more characters to read from file
	while((bytes_read = read(fd, buffer, random)) != 0) {
		//Reinitialize the payload
		memset(f.payload, 0, sizeof(f.payload));
		memset(s.payload, 0, sizeof(s.payload));
		memset(r->payload, 0, sizeof(r->payload));
		
		//Copy the content previously read
		memcpy(f.payload, buffer, bytes_read);
		//Set the sequence number
		f.seq_no = convert(seq);
		//Compute the checksum
		f.checksum = convert(compute_checksum(buffer, bytes_read));
		//Set the length of the message
		s.len = sizeof(f);
		//Copy the content of the frame
		memcpy(s.payload, &f, s.len);
		//Send the message
		send_message(&s);
		
		print_to_log_sent(log, f);
		fprintf(log, "=================================================\n");
		
		
		//Waiting to receive a reply
		r = receive_message_timeout(my_timeout);
		//While a reply did not come, resend the same message
		while(!r) {
			send_message(&s);
			print_to_log_sent(log, f);			
			fprintf(log, "=================================================\n");
			r = receive_message_timeout(my_timeout);
		}

		//If I've received a message, I check to see whether it's a proper one or not
		memset(f.payload, 0, sizeof(f.payload));
		memcpy(&f, r->payload, r->len);
		
		if(f.seq_no == convert(seq)) {
			print_to_log_recv(log, f);
			fprintf(log, "=================================================\n");
			seq++;
			seq %= 10;
		} else {
			print_to_log_recv(log, f);
			fprintf(log, "ACK-CORRUPT!\n");
			fprintf(log, "Retrimit mesajul ...\n");
			fprintf(log, "=================================================\n");
		}
	}
	
	while(1) {
		memset(f.payload, 0, sizeof(f.payload));
		memset(s.payload, 0, sizeof(s.payload));
		memcpy(f.payload, finish, strlen(finish));
		f.seq_no = convert(seq);
		f.checksum = convert(compute_checksum(finish, strlen(finish)));
		s.len = sizeof(f);
		memcpy(s.payload, &f, s.len);
		send_message(&s);
	
		print_to_log_sent(log, f);
		fprintf(log, "=================================================\n");
		
		//Waiting to receive a reply
		r = receive_message_timeout(my_timeout);
		//While a reply did not come, resend the same message
		while(!r) {
			send_message(&s);
			print_to_log_sent(log, f);			
			fprintf(log, "=================================================\n");
			r = receive_message_timeout(my_timeout);
		}
		
		//If I've received a message, I check to see whether it's a proper one or not
		memset(f.payload, 0, sizeof(f.payload));
		memcpy(&f, r->payload, r->len);
		
		if(f.seq_no == convert(seq)) {
			print_to_log_recv(log, f);
			fprintf(log, "=================================================\n");
			seq++;
			seq %= 10;
			break;
		} else {
			print_to_log_recv(log, f);
			fprintf(log, "ACK-CORRUPT!\n");
			fprintf(log, "Retrimit mesajul ...\n");
			fprintf(log, "=================================================\n");
		}
	}
	
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
		fprintf(log, "[%s-%s-%s %s] [SEND]  ", time_buf[2], "03", time_buf[4], time_buf[3]);
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
	fprintf(log, "Am trimis urmatorul pachet:\n");
	fprintf(log, "Seq No: %c\n", f.seq_no);
	fprintf(log, "Payload: %s\n", f.payload);
	fprintf(log, "Checksum: ");
	convert_to_base_2(log, f.checksum);
}

void print_to_log_recv(FILE* log, frame f) {
	my_get_time(log);
	fprintf(log, "Am primit urmatorul pachet:\n");
	fprintf(log, "Seq No: %c\n", f.seq_no);
	fprintf(log, "Checksum: ");
	convert_to_base_2(log, f.checksum);
}
