#include "main.h"

	char verbose=0;
	FILE *cfsFileHandle;
	FILE *matFileHandle;
	generalHeader header;
	channelInfo *channels;
	unsigned long *pointerTable;
	frame_generalHeader *frame_headers;
	frame_channelInfo *frame_channels;
	unsigned char *holding_buffer;

//Read frame into holding buffer
void read_in_frame(unsigned short frame_no){
	fseek(cfsFileHandle,(frame_headers+frame_no)->pointer_to_data,SEEK_SET);
	fread(holding_buffer,1,(frame_headers+frame_no)->data_size,cfsFileHandle);
}

//Copy interlaced channel from holding buffer into channel buffer
//Convert Int16 data to floats
void split_and_convert(float *outbuffer, unsigned char *inbuffer, short unsigned channel, short unsigned frame){
	frame_channelInfo *a=frame_channels+channel+frame*header.channels;
	channelInfo *b=channels+channel;
	unsigned char *current_point=inbuffer + a->first_byte_offset;
	float *outbuffer_current_point=outbuffer;
	for (unsigned long i=0; i < a->data_points; i++){
		float ans=(float) (*((short signed*) current_point) * a->yscale + a->yoffset);
		*outbuffer_current_point=ans;
		current_point+=b->bytespace;
		outbuffer_current_point++;
	}
}


//Write an array header to the matlab file
void write_matlab_array_header(char *name, unsigned long y, unsigned long x){
	unsigned long size=x*y*sizeof(float);
	unsigned long name_size=strlen(name);
	char padbuffer[8]={0,0,0,0,0,0,0,0};
	unsigned long i[12]={miMATRIX,48+name_size+size,miUINT32,8,mxSINGLE_CLASS,0,miINT32,8,x,y,miINT8,name_size};
	fwrite(&i,4,12,matFileHandle);
	fwrite(name,1,name_size,matFileHandle);
	unsigned char padding=8-name_size%8;
	if (padding!=8) fwrite(padbuffer,1,padding,matFileHandle);
}
	
//Write an array data element to the matlab file
void write_as_matlab_array(float *data, unsigned long size){
	unsigned long i[2];
	i[0]=7;
	i[1]=size;
	fwrite(&i,4,2,matFileHandle);
	fwrite(data,1,size,matFileHandle);
	char padbuffer[8]={0,0,0,0,0,0,0,0};
	unsigned char padding=8-size%8;
	if (padding!=8) fwrite(padbuffer,1,padding,matFileHandle);
}

//Write matlab file header
void write_matlab_header(){
	char desc_text[128];
	memset(&desc_text,0x20,128);
	sprintf(desc_text,"MATLAB 5.0 MAT-file created from a cfs file recorded at date:%8.8s time:%8.8s ",header.date,header.time);
	desc_text[124]=0;
	desc_text[125]=1;
	desc_text[126]='I';
	desc_text[127]='M';
	
	fwrite(&desc_text,1,128,matFileHandle);
}

//Process cfs file on per-channel basis.
int read_by_channel(){	
	//Allocate arrays for shortest and longest frames in each channel
	unsigned long *shortest_frame=malloc(sizeof(long)*header.channels);
	unsigned long *longest_frame=malloc(sizeof(long)*header.channels);
	memset(shortest_frame,0xff,sizeof(long)*header.channels);
	memset(longest_frame,0x00,sizeof(long)*header.channels);

	float **channel_buffers;
	float **current_buffer_pointers;
	unsigned long largest_buffer=0;
	unsigned long *total_space=malloc(sizeof(long)*header.channels);
	
	//Check all frames are of same length and largest buffer size required
	for(short unsigned i=0;i<header.total_frames;i++){
		for(short unsigned j=0;j<header.channels;j++){
			unsigned long data_points=(frame_channels+i*header.channels+j)->data_points;
			if (*(longest_frame+j) < data_points)
				*(longest_frame+j)=data_points;
			if (*(shortest_frame+j) > data_points)
				*(shortest_frame+j)=data_points;			
		}
		if (largest_buffer<(frame_headers+i)->data_size)
			largest_buffer=(frame_headers+i)->data_size;
	}
	
	//Check legality and convert to bytes
	for(short unsigned j=0;j<header.channels;j++){
		if (*(longest_frame+j)!=*(shortest_frame+j)){
			fprintf(stderr,"\n\nFrames do not appear to be of the same length. Processing was aborted.\n\n");
			return 1;
		}
		*(total_space+j) = *(longest_frame+j) * sizeof(float) * header.total_frames;
	}
	
	
	//allocate buffers, set pointers to the start of them and current position pointers
	holding_buffer=malloc(largest_buffer);
	if (verbose)
			printf("\n\nTemporary buffer size: \t%ld KB\n",largest_buffer/1024);

	channel_buffers=malloc(header.channels*sizeof(char*));
	current_buffer_pointers=malloc(header.channels*sizeof(char*));

	//for each channel allocate a sufficiently large buffer
	for(short unsigned j=0;j<header.channels;j++){
		*(channel_buffers+j)=malloc(*(total_space+j));
		*(current_buffer_pointers+j)=*(channel_buffers+j);
		if (verbose)
			printf("Channel %d: \t%ld KB\n",j+1,*(total_space+j)/1024);
	}
	
	//read in frame at a time
	for(short unsigned i=0;i<header.total_frames;i++){
		read_in_frame(i);
		for(short unsigned j=0;j<header.channels;j++){
			split_and_convert(*(current_buffer_pointers+j),holding_buffer,j,i);
			*(current_buffer_pointers+j) += *(longest_frame+j);
		}
	}

	//loop over channels, writing out arrays
	write_matlab_header();
	//for(short unsigned j=0;j<header.channels;j++){
	for(short unsigned j=2;j<4;j++){
		char name[9];
		sprintf(name,"Channel%1d",j+1);
		write_matlab_array_header(name, header.total_frames, *(longest_frame+j));
		write_as_matlab_array(*(channel_buffers+j), *(total_space+j));
		printf(" height: %lu width: %lu byte: %lu \n",*(longest_frame+j),header.total_frames,*(total_space+j));
	}
	return 0;
}
	

int main (int argc, const char * argv[]) {

	//Open cfs file for reading
	cfsFileHandle=fopen(argv[1],"rb");
	if (cfsFileHandle==NULL){
		fprintf(stderr,"File %s could not be opened for reading.",argv[1]);
		return 1;
	}

	//open matlab file for writing
	matFileHandle=fopen(argv[2],"wb");
	if (matFileHandle==NULL){
		fprintf(stderr,"File %s could not be opened for writing.",argv[2]);
		return 1;
	}
	
	//Get general header
	fread(&header,sizeof(generalHeader),1,cfsFileHandle);
	if (strncmp(header.marker,"CEDFILE\"",8)!=0){
		fprintf(stderr,"From the header, this file does not appear to be a valid cfs file. \n\nProcessing was aborted.\n");
		return 2;
	}
	if (verbose)
		printf("Filename: %s\nSize: %ldKB\nData frames: %d\nChannels: %d\n",header.filename,header.size/1024,header.total_frames,header.channels);

	//Read channel info
	channels=malloc(header.channels*sizeof(channelInfo));
	fread(channels,sizeof(channelInfo),header.channels,cfsFileHandle);
	
	//Read pointer table
	pointerTable=malloc(header.total_frames*4);
	fseek(cfsFileHandle,header.pointer_table_offset,SEEK_SET);
	fread(pointerTable,4,header.total_frames,cfsFileHandle);
	
	//Read frame general headers and frame-specific channel info
	frame_headers=malloc(header.total_frames*sizeof(frame_generalHeader));
	frame_channels=malloc(header.total_frames*header.channels*sizeof(frame_channelInfo));
	//loop over all frames, reading data into memory.
	for(short unsigned i=0;i<header.total_frames;i++){
		fseek(cfsFileHandle,pointerTable[i],SEEK_SET);
		fread(frame_headers+i,1,sizeof(frame_generalHeader),cfsFileHandle);
		fread(frame_channels+i*header.channels,1,sizeof(frame_channelInfo)*header.channels,cfsFileHandle);
		if (verbose){
			printf("Frame %d:\t",i+1);
			for (frame_channelInfo *p=frame_channels+i*header.channels;p<frame_channels+(i+1)*header.channels;p++){
				printf("\t%ld",p->data_points);
			}
			printf("\n");
		}
	}
	//
	if (read_by_channel()) return 1;
	fclose(cfsFileHandle);
	fclose(matFileHandle);
    return 0;
}



