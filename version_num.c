/* This program changes the version number of PAT and PMT in a given TS file */
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
	NON_TABLE_PID,
	ERRONEOUS_SECTION_LENGTH,
	INFO_MISMATCH
};

struct ts_header_t
{
	int start_indicator;
	int pid;	
};

struct table_header_t{
	int table_pid;
	int ssi;
	int section_length;
};

struct table_section_t{
	int version_number;
	int cni;
};

struct app_ctx_t{
	struct ts_header_t ts_header;
	struct table_header_t table_header;
	struct table_section_t table_section;
	int is_pat;
	int is_pmt;
	int pmt_pid;
};

int main(int argc, char const *argv[])
{
	char filename[100];
	int state = 0;
	int ret;
	char ch;
	FILE *fp; 
	char ts_packet_buffer[188];
	int bytes_read = 0;
	struct app_ctx_t app_ctx;
	
	printf("Enter filename\n");
	scanf("%s", filename);

	fp = fopen(filename, "rb+");
	if(fp == NULL){
		printf("Error opening file : %s\n", strerror(errno));
		return 1;
	}
	
	ch = fgetc(fp);
	while(ch != 0x47){
		ch = fgetc(fp);
		printf("ch = %x\n");
		if(ch == EOF){
			printf("No start code found in file\n");
			fclose(fp);
			return 1;
		}
	}
	
	fseek(fp, -1, SEEK_CUR);
	bytes_read = fread(ts_packet_buffer, 1, 188, fp);
	printf("bytes read = %d\n", bytes_read);
	while(bytes_read == TS_PACKET_LENGTH){
		int status = process_packet(ts_packet_buffer, &app_ctx);
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
		bytes_read = fread(ts_packet_buffer,1, 188, fp);
		printf("bytes read = %d\n", bytes_read);
	}
	printf("processing complete\n");
	fclose(fp);
	return 0;
}

int process_packet(char *buffer,struct app_ctx_t* app_ctx){
	int bytes_processed = 0;
	char new_byte = 0;
	
	/* Parsing TS packet header	*/
	if(buffer[bytes_processed++] != TS_SYNC_BYTE)
		return ERRONEOUS_SYNC_BYTE;

	app_ctx->ts_header.start_indicator = (buffer[bytes_processed] & 0x40)>>6;
	if(app_ctx->ts_header.start_indicator != 1)
		return NO_START_INDICATOR;

	app_ctx->ts_header.pid = buffer[bytes_processed++] & 0x1F;
	app_ctx->ts_header.pid = app_ctx->ts_header.pid << 5;
	app_ctx->ts_header.pid = buffer[bytes_processed++] & 0xFF;
	printf("ts header pid = %d\n", app_ctx->ts_header.pid);
	if(app_ctx->ts_header.pid == 0x0000){
		app_ctx->is_pat = 1;
	}
	if(app_ctx->pmt_pid != 0 && app_ctx->ts_header.pid == app_ctx->pmt_pid){
		app_ctx->is_pmt = 1;
	}
	else{
		return NON_TABLE_PID;
	}

	bytes_processed++;	/* We don't care about the things included in last byte */

	/* Parsing the table header	*/
	app_ctx->table_header.table_pid = buffer[bytes_processed++];
	printf("table pid = %d\n", app_ctx->table_header.table_pid);
	if(app_ctx->is_pat && app_ctx->table_header.table_pid != 0x00)
		return INFO_MISMATCH;
	else if(app_ctx->is_pmt && app_ctx->table_header.table_pid != 0x02)
		return INFO_MISMATCH;
	app_ctx->table_header.ssi = buffer[bytes_processed] & 0x80;
	
	app_ctx->table_header.section_length = buffer[bytes_processed++] & 0x03;
	app_ctx->table_header.section_length = app_ctx->table_header.section_length << 2;
	app_ctx->table_header.section_length |= buffer[bytes_processed++];
	if(app_ctx->table_header.section_length > 1021 || 
		app_ctx->table_header.section_length < 0)
		return ERRONEOUS_SECTION_LENGTH;
	
	/* Parsing table section data	*/
	bytes_processed += 2;
	
	app_ctx->table_section.version_number = (buffer[bytes_processed] & 0x3E)<<1;
	app_ctx->table_section.cni = buffer[bytes_processed] & 0x01;
	
	new_byte |= 0x03;	/* Reserved bytes that are always set to 1 */
	new_byte <<= 3;
	new_byte |= (app_ctx->table_section.version_number+1);
	new_byte <<= 5;
	new_byte |= (app_ctx->table_section.cni);
	buffer[bytes_processed] = new_byte;

	if(app_ctx->is_pat){
		bytes_processed += 4;
		app_ctx->pmt_pid = buffer[bytes_processed++] & 0x1F;
		app_ctx->pmt_pid != buffer[bytes_processed++];
		app_ctx->is_pat = 0;
	}
	else{
		app_ctx->is_pmt = 0;
	}
	return SUCCESS;	
}
