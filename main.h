#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct generalHeader{
	char marker[8];
	unsigned char filename_length;
	char filename[13];
	unsigned long size;
	char time[8];
	char date[8];
	unsigned short channels;
	unsigned short filevars;
	unsigned short dsvars;
	unsigned short bytesize_file_header;
	unsigned short bytesize_frame_header;
	unsigned long last_frame_offset;
	unsigned short total_frames;
	unsigned short disk_block_size_rounding;
	char comment[74];
	unsigned long pointer_table_offset;
	char reserved[40];
}__attribute__((__packed__));

struct channelInfo{
	char name_length;
	char name[21];
	char yaxisunits_length;
	char yaxisunits[9];
	char xaxisunits_length;
	char xaxisunits[9];
	char data_type;
	char kind;
	unsigned short bytespace;
	unsigned short matrix_next_channel;
}__attribute__((__packed__));

struct frame_generalHeader{
	unsigned long pointer_to_previous;
	unsigned long pointer_to_data;
	unsigned long data_size;
	short flags;
	char reserved[16];
}__attribute__((__packed__));

struct frame_channelInfo{
	unsigned long first_byte_offset;
	unsigned long data_points;
	float yscale;
	float yoffset;
	float xinc;
	float xoffset;
}__attribute__((__packed__));

typedef struct generalHeader generalHeader;
typedef struct channelInfo channelInfo;
typedef struct frame_generalHeader frame_generalHeader;
typedef struct frame_channelInfo frame_channelInfo;

typedef unsigned long matlabTypeTag;
enum {
	miINT8=0x01,
	miUINT8=0x02,
	miINT16=0x03,
	miUINT16=0x04,
	miINT32=0x05,
	miUINT32=0x06,
	miSINGLE=0x07,
	miDOUBLE=0x09,
	miINT64=0x0c,
	miUINT64=0x0d,
	miMATRIX=0x0e,
	miUTF8=0x10
};

typedef unsigned long matlabArrayType;
enum {
	mxCELL_CLASS=0x01,
	mxSTRUCT_CLASS=0x02,
	mxOBJECT_CLASS=0x03,
	mxCHAR_CLASS=0x04,
	mxSPARSE_CLASS=0x05,
	mxDOUBLE_CLASS=0x06,
	mxSINGLE_CLASS=0x07,
	mxINT8_CLASS=0x08,
	mxUINT8_CLASS=0x09,
	mxINT16_CLASS=0x0a,
	mxUINT16_CLASS=0x0b
};

