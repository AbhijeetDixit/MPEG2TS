#include <stdio.h>
#include <errno.h>
#include <string.h>

#define TS_SYNC_BYTE 0x47
#define TS_PACKET_LENGTH 188

extern int errno;

enum error_codes{
	SUCCESS,
	ERRONEOUS_SYNC_BYTE,
	NO_START_INDICATOR,
	NON_TABLE_PID
};

struct ts_header_t
{
	int start_indicator;
	int pid;	
};

struct table_header_t{
	int table_id;
	int ssi;
};

struct app_ctx_t{
	struct ts_header_t ts_header;
	struct table_header_t table_header;
	int version_number;
	int pmt_pid;
};

int main(int argc, char const *argv[])
{
	char filename[100];
	int state = 0;
	int ret;
	char ch;
	FILE *fp = fopen(filename "rb+");
	char ts_packet_buffer[188];
	int bytes_read = 0;
	struct app_ctx_t app_ctx;

	if(fp == NULL){
		printf("Error opening file : %s\n", strerror(errno));
		return 1;
	}
	
	ch = fgetc(fp);
	while(ch != 0x47){
		ch = fgetc(fp);
		if(ch == EOF){
			printf("No start code found in file\n");
			fclose(fp);
			return 1;
		}
	}
	
	bytes_read = fread(ts_packet_buffer, 188, 1, fp);
	while(bytes_read == TS_PACKET_LENGTH){
		int status = process_packet(ts_packet_header, &app_ctx);
		if(status == SUCCESS){
			int bytes_written;
			fseek(fp, -188, SEEK_CUR);
			bytes_written = fwrite(ts_packet_buffer, 188, 1, fp);
			if(bytes_written < TS_PACKET_LENGTH){
				printf("Error writing file : %s\n", strerror(errno));
				fclose(fp);
				return 1;
			}
		}
		else{
			printf("Error while processing packet. Error Code : %d\n", status);
		}
		bytes_read = fread(ts_packet_buffer, 188, 1, fp);
	}
	printf("processing complete\n");
	fclose(fp);
	return 0;
}

int process_packet(char *buffer, app_ctx_t* app_ctx){
	int bytes_processed = 0;
	
	if(buffer[bytes_processed++] != TS_SYNC_BYTE)
		return ERRONEOUS_SYNC_BYTE;

	app_ctx->ts_header.start_indicator = buffer[bytes_processed] & 0x40;
	if(app_ctx->ts_header.start_indicator != 1)
		return NO_START_INDICATOR;

	app_ctx->ts_header.pid = buffer[bytes_processed++] & 0x1F;
	app_ctx->ts_header.pid = buffer[bytes_processed++] & 0xFF;

}
