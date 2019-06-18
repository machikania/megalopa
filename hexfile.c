/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include "compiler.h"
#include "api.h"

/*
 Intel HEX format example
 :040bf000ffffffcf35
    +--------------------- Byte count
    |    +---------------- Address
    |    |  +------------- Record type (00:Data, 01:EOF, 04: Extended linear addres
    |    |  |        +---- Data
    |    |  |        |  +- Checksum
    |    |  |        |  |
 : 04 0bf0 00 ffffffcf 35
 : 02 0000 04 1fc0     1b
 : 00 0000 01          FF

	Record types:
		case 0:  // data
		case 4:  // extended linear address
		case 1:  // EOF

*/

/*
	Example of HEX file

:020000040000fa
:1080000000001c3c707f9c2721e09903e0ffbd2706
:108010001c00bfaf1000bcaf0e0080101880828f14
:10802000000044ac1c80828f80ff42241c80848f1f
:10803000000040a0010042242b184400fdff6054c2
:10804000000040a02080998f09f820030000000064
:108050001000bc8f2480828f1c00bf8f0800e003bb
:048060002000bd2718
:020000040000fa
:107f80000000000000000080a47f00a0008000a08e
:107f9000708000a0fc8000a0000001a0788000a0fc
:047fa00000000000dd
:020000040000fa
:108070000800e0030000000000001c3cf87e9c2784
:1080800021e09903040080542880828f2880828f09
:108090000800e003d48042240800e003e480422486
:1080a00000001c3cd07e9c2721e09903e0ffbd2707
:1080b0001c00bfaf1000bcaf2c80998f09f82003c3
:1080c000000000001000bc8f1c00bf8f0800e00300
:0480d0002000bd27a8
:020000040000fa
:1080d40048656c6c6f20576f726c6421000000005f
:1080e4005468697320697320612074657374000097
:0880f400544553540000000044
:020000040000fa
:1080fc0040010000800000001c8100a00000000076
:020000040000fa
:10810c0000000000222222222222222222222222cb
:020000040000fa
:0c811c00f48000a0a08000a00000000083
:00000001FF
*/

// Use the same global vars in file.c
extern FSFILE* g_fhandle;
extern char* g_fbuff;
extern int g_size;
extern int g_filepoint;
// Vars used only in this file
static unsigned char g_checksum;

void hex_read_file(int blocklen){
	int i;
	if (blocklen==512) {
		// This is first read. Initialize parameter(s).
		g_srcpos=0;
		g_filepoint=0;
	} else if (g_size<512) {
		// Already reached the end of file.
		return;
	} else {
		// Shift buffer and source position 256 bytes.
		for(i=0;i<256;i++) g_fbuff[i]=g_fbuff[i+256];
		g_srcpos-=256;
		g_filepoint+=256;
	}
	// Read 512 or 256 bytes from SD card.
	g_size=512-blocklen+FSfread((void*)&g_fbuff[512-blocklen],1,blocklen,g_fhandle);
	// All lower cases
	for(i=512-blocklen;i<512;i++){
		if ('A'<=g_fbuff[i] && g_fbuff[i]<='F') g_fbuff[i]+=0x20;
	}
}

int hex_read_byte(){
	unsigned char b1,b2;
	b1=g_fbuff[g_srcpos++];
	b2=g_fbuff[g_srcpos++];
	if ('0'<=b1 && b1<='9') {
		b1-='0';
	} else if ('a'<=b1 && b1<='f') {
		b1-='a';
		b1+=0x0a;
	} else {
		return -1;
	}
	if ('0'<=b2 && b2<='9') {
		b2-='0';
	} else if ('a'<=b2 && b2<='f') {
		b2-='a';
		b2+=0x0a;
	} else {
		return -1;
	}
	b1=(b1<<4)|b2;
	g_checksum+=b1;
	return b1;
}

char* hex_read_line(){
	int i,j;
	// Initialize checksum
	g_checksum=0;
	// Maintain at least 256 characters in cache.
	if (256<=g_srcpos) hex_read_file(256);
	// Read a hex file line
	if (g_fbuff[g_srcpos++]!=':') return ERR_HEX_ERROR;
	// Read size
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.size=(unsigned char)i;
	// Read address
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.address=(unsigned short)(i<<8);
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.address|=(unsigned short)(i);
	// Ready type
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.type=(unsigned char)i;
	// Read data
	for(j=0;j<g_hexline.size;j++){
		i=hex_read_byte();
		if (i<0) return ERR_HEX_ERROR;
		g_hexline.data[j]=(unsigned char)i;
	}
	// Read checksum
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	if (g_checksum) return ERR_HEX_ERROR;
	// All done. Remove enter.
	if (g_fbuff[g_srcpos]=='\r') g_srcpos++;
	if (g_fbuff[g_srcpos]=='\n') g_srcpos++;
	return 0;
}

static char g_hex_line[47]=":";
void hex_construct_byte(unsigned char data,int pos){
	g_hex_line[pos+0]="0123456789abcdef"[data>>4];
	g_hex_line[pos+1]="0123456789abcdef"[data&15];
	g_checksum+=data;
}
char* hex_construct_line(){
	int i;
	g_checksum=0;
	// Write size
	hex_construct_byte(g_hexline.size,1);
	// Write address
	hex_construct_byte(g_hexline.address>>8,3);
	hex_construct_byte(g_hexline.address&0xff,5);
	// Write type
	hex_construct_byte(g_hexline.type,7);
	// Write data
	for(i=0;i<g_hexline.size;i++){
		hex_construct_byte(g_hexline.data[i],9+i*2);
	}
	// Write checksum
	hex_construct_byte(0-g_checksum,9+i*2);
	// All done. Add CRLF and 0x00
	g_hex_line[11+i*2]=0x0d;
	g_hex_line[12+i*2]=0x0a;
	g_hex_line[13+i*2]=0;
	return (char*)&g_hex_line[0];
}

void hex_reinit_file(){
	// Go to point 0
	FSfseek(g_fhandle,0,SEEK_SET);
	// Initialize parameters
	g_srcpos=0;
	g_filepoint=0;
	// Read first 512 bytes
	hex_read_file(512);
}

char* hex_init_file(char* buff,char* filename){
	// Open file
	g_fhandle=FSfopen(filename,"r");
	if (!g_fhandle) {
		return ERR_UNKNOWN;
	}
	// Initialize parameters
	g_fbuff=buff;
	g_source=buff;
	g_srcpos=0;
	g_filepoint=0;
	// Read first 512 bytes
	hex_read_file(512);
	return 0;
}

void hex_close_file(){
	FSfclose(g_fhandle);
}

char* hex_write(FSFILE* fhandle){
	int i,j;
	char* str;
	// Construct the line string.
	str=hex_construct_line();
	// Determine string length
	for(i=0;str[i];i++);
	j=FSfwrite(str,1,i,fhandle);
	if (j<i) return ERR_FILE;
	return 0;
}


char* hex_write_address(FSFILE* fhandle,unsigned short addr){
	int i,j;
	char* str;
	addr&=0x7fff; // Write 0x1d00 instead of 0x9d00.
	g_hexline.size=2;
	g_hexline.type=4;
	g_hexline.address=0;
	g_hexline.data[0]=addr>>8;
	g_hexline.data[1]=addr&0xff;
	return hex_write(fhandle);
}

char* hex_write_data_16(FSFILE* fhandle,unsigned short addr,unsigned int* object){
	int i,j;
	char* str;
	g_hexline.size=16;
	g_hexline.type=0;
	g_hexline.address=addr;
	for(i=0;i<4;i++){
		g_hexline.data[i*4+0]=object[i];
		g_hexline.data[i*4+1]=object[i]>>8;
		g_hexline.data[i*4+2]=object[i]>>16;
		g_hexline.data[i*4+3]=object[i]>>24;
	}
	return hex_write(fhandle);
}

char* hex_write_eof(FSFILE* fhandle){
	int i;
	i=FSfwrite(":00000001FF\x0d\x0a",1,13,fhandle);
	if (i<13) return ERR_FILE;
	return 0;
}

