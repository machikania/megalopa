/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

#ifdef __DEBUG

#include <xc.h>
#include "api.h"
#include "main.h"
#include "compiler.h"

/*
	Enable following line when debugging binary object.
*/
//#include "debugdump.h"


// Pseudo reading config setting for debug mode
unsigned int g_DEVCFG1=0xFF7F4DDB;

// Construct jump assembly in boot area.
const unsigned int _debug_boot[] __attribute__((address(0xBFC00000))) ={
	0x0B401C00,//   j           0x9d007000
	0x00000000,//   nop         
};

// Use DEBUG.HEX as file name of this program.
const unsigned char _debug_filename[] __attribute__((address(FILENAME_FLASH_ADDRESS))) ="DEBUG.HEX";

static const char initext[];
static const char bastext[];
static const char class1text[];
static const char class2text[];
static const char hextext[];

static char* readtext;
static int filepos;

/*
	Debug dump
	In debugdump.h:
		__DEBUGDUMP is defined.
		__DEBUGDUMP_FREEAREA is defined as start address of free area (1st argument of set_free_area() function)
		const unsigned char dump[] is initialized.
*/
#ifdef __DEBUGDUMP
int debugDump(){
	int i;
	for(i=0;i<sizeof dump;i++){
		RAM[i]=dump[i];
	}

	g_objpos=(__DEBUGDUMP_FREEAREA-(unsigned int)g_object)/4;

	// Initialize parameters
	g_pcg_font=0;
	g_use_graphic=0;
	g_graphic_area=0;
	clearscreen();
	setcursor(0,0,7);	

	printstr("BASIC "BASVER"\n");
	wait60thsec(15);

	printstr("Compiling...");

	// Initialize the other parameters
	// Random seed
	g_rnd_seed=0x92D68CA2; //2463534242
	// Clear variables
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_mem[i]=0;
		g_var_size[i]=0;
	}
	// Clear key input buffer
	for(i=0;i<256;i++){
		ps2keystatus[i]=0;
	}
	// Reset data/read.
	reset_dataread();

	// Assign memory
	set_free_area((void*)(g_object+g_objpos),(void*)(&RAM[RAMSIZE]));
	// Execute program
	// Start program from the beginning of RAM.
	// Work area (used for A-Z values) is next to the object code area.
	start_program((void*)(&(RAM[0])),(void*)(&g_var_mem[0]));
	printstr("\nOK\n");
	g_use_graphic=0;

	return 1;
}
#else
int debugDump(){
	return 0;
}
#endif

/*
    Override libsdfsio functions.
    Here, don't use SD card, but the vertual files 
    (initext[] and bastext[]) are used. 
*/

FSFILE fsfile;

size_t FSfread(void *ptr, size_t size, size_t n, FSFILE *stream){
	char b;
	size_t ret=0;
	if (!readtext) return 0;
	while(b=readtext[filepos]){
		filepos++;
		((char*)ptr)[ret]=b;
		ret++;
		if (n<=ret) break;
	}
	return ret;
}
FSFILE* FSfopen(const char * fileName, const char *mode){
	int i;
	for(i=0;i<13;i++){
		if (fileName[i]=='.') break;
	}
	if (i==13) {
		// Unknown file name
		// Force BAS file
		readtext=(char*)&bastext[0];
	} else if (fileName[i+1]=='I' && fileName[i+2]=='N' && fileName[i+3]=='I') {
		// INI file
		readtext=(char*)&initext[0];
	} else if (fileName[i+1]=='H' && fileName[i+2]=='E' && fileName[i+3]=='X') {
		// HEX file
		readtext=(char*)&hextext[0];
	} else if (fileName[i+1]=='B' && fileName[i+2]=='A' && fileName[i+3]=='S') {
		// Select BAS file
		if (fileName[i-6]=='C' && fileName[i-5]=='L' && fileName[i-4]=='A' &&
			fileName[i-3]=='S' && fileName[i-2]=='S') {
			if (fileName[i-1]=='1') readtext=(char*)&class1text[0];
			else if (fileName[i-1]=='2') readtext=(char*)&class2text[0];
			else readtext=(char*)&bastext[0];
		} else {
			readtext=(char*)&bastext[0];
		}
		// Try debugDump.
		if (debugDump()) return 0;
	} else {
		readtext=0;
		return 0;
	}
	filepos=0;
	return &fsfile;
}
int FSfeof( FSFILE * stream ){
	return readtext[filepos]?1:0;
}
int FSfclose(FSFILE *fo){
	return 0;
}
int FSInit(void){
	return 1;
}
int FSremove (const char * fileName){
	return 0;
}
size_t FSfwrite(const void *ptr, size_t size, size_t n, FSFILE *stream){
	return 0;
}

int FindFirst (const char * fileName, unsigned int attr, SearchRec * rec){
	return 0;
}
int FindNext (SearchRec * rec){
	return 0;
}
int FSmkdir (char * path){
	return 0;
}
char * FSgetcwd (char * path, int numchars){
	return 0;
}
int FSchdir (char * path){
	return 0;
}
long FSftell (FSFILE * fo){
	return 0;
}
int FSfseek(FSFILE *stream, long offset, int whence){
	filepos=offset;
	return 0;
}
/*
    ps2init() is not called.
    Instead, not_ps2init_but_init_Timer1() is called.
    Timer1 is used to update drawcount and drawing gloval variables.
*/

int not_ps2init_but_init_Timer1(){
	PR1=0x0FFF;
	TMR1=0;
	IFS0bits.T1IF=0;
	T1CON=0x8000;
	// Timer1 interrupt: priority 4
	IPC1bits.T1IP=4;
	IPC1bits.T1IS=0;
	IEC0bits.T1IE=1;

	return 0;
}

#pragma interrupt timer1Int IPL4SOFT vector 4

void timer1Int(){
	IFS0bits.T1IF=0;
	if (drawing) {
		drawing=0;
		drawcount++;
	} else {
		drawing=1;
	}
}

/*
    initext[] and bastext[] are vertual files 
    as "MACHIKAN.INI" and "DEBUG.BAS".
*/


static const char initext[]=
"#PRINT\n"
"#PRINT\n";

static const char bastext[]=
"useclib TCLIB\n"
"print hex$(TCLIB::TEST(1))\n"
"\n"
"\n"
"\n"
"\n";

static const char class1text[]=
"FIELD T1\n"
"method T2\n"
" return T1+100\n"
"\n"
"\n";

static const char class2text[]=
"FIELD T3\n"
"method T6\n"
" return T3+100\n"
"\n"
"\n"
"\n"
"\n";

static const char hextext[]=
":020000040000fa\n"
":1080000000001c3cf07e9c2721e09903e0ffbd2787\n"
":108010001c00bfaf1000bcaf030080101c80828f1b\n"
":1080200005000010000044ac2080998f09f820035f\n"
":10803000000000001000bc8f2480828f1c00bf8fc6\n"
":088040000800e0032000bd2749\n"
":020000040000fa\n"
":107f00000000000000000080bc8100a0347f00a0c1\n"
":107f1000ec8100a0c08100a0d08100a0a08100a0c1\n"
":107f2000000001a0848100a0808000a0000000006b\n"
":047f3000000000004d\n"
":020000040000fa\n"
":1080800000001c3c707e9c2721e09903e0ffbd2787\n"
":108090001c00bfaf1800b0af1000bcaf06008010ce\n"
":1080a0001c80908f01000224120082102880998f7a\n"
":1080b000150000103080828f0000048e2c80998f74\n"
":1080c00009f82003000000001000bc8f3080848f6e\n"
":1080d0003c8184240000058e3480998f09f82003a8\n"
":1080e000000000001000bc8f3080828f070000105d\n"
":1080f0001c81422409f82003000000001000bc8ffe\n"
":10810000030000101c00bf8f2c8142241c00bf8f75\n"
":0c8110001800b08f0800e0032000bd271d\n"
":020000040000fa\n"
":10811c0048656c6c6f20576f726c64210000000016\n"
":10812c00546869732069732061207465737400004e\n"
":10813c00434c494220746573742e000054455354cb\n"
":04814c00000000002f\n"
":020000040000fa\n"
":1081500000001c3ca07d9c2721e09903e0ffbd2787\n"
":108160001c00bfaf1000bcaf3880998f09f8200306\n"
":10817000000000001000bc8f1c00bf8f0800e0034f\n"
":048180002000bd27f7\n"
":020000040000fa\n"
":108184000000a38c0c00a28c0c00428c0800400060\n"
":0c81940000007c8c0800e00300000000ec\n"
":020000040000fa\n"
":1081a0000000838c0c00828c1800428c0800400078\n"
":0c81b00000007c8c0800e00300000000d0\n"
":020000040000fa\n"
":1081bc00d08100a04001000080000000e08100a000\n"
":0481cc0000000000af\n"
":020000040000fa\n"
":1081d0001880828f0000428c0800e00323102203e5\n"
":020000040000fa\n"
":0c81e000488100a0508100a000000000b9\n"
":020000040000fa\n"
":0881ec000800e00300000000a0\n"
":00000001FF\n"
;

/*
    Test function for constructing assemblies from C codes.
*/

static const void* debugjumptable[]={
	FSfread,
	FSfopen,
};

#define for2(x,y,z) for(x=y;x<=z;x++)

int _debug_test(int a0, int a1, int a2, int a3, int param4, int param5){
	asm volatile(".set noreorder");
	asm volatile("addiu $v0,$zero,4772");
	asm volatile("loop:");
	asm volatile("bne $v0,$zero,loop");
	asm volatile("addi $v0,$v0,-1");
	asm volatile("wait");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	a2&=0xFFFFFFFC;
	return a2+a3;
}

int _debug_test2(int a0, int a1, int a2, int a3, int a4){
	int v0;
	v0=v0+a0;
	v0=v0*a1;
	v0=v0 & a3;
	v0=v0 | a4;
	return v0;
}

/*
	Break point used for debugging object code.

g_object[g_objpos++]=0x0000000d;// break 0x0

*/

#endif // __DEBUG
