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
#include "main.h"

/*
	CMPDATA_USECLIB structure
		type:      CMPDATA_USECLIB (8)
		len:       2
		data16:    n/a (0)
		record[1]: clib name as integer

	CMPDATA_CLIBFUNC structure
		type:      CMPDATA_CLIBFUNC (9)
		len:       4
		data16:    n/a (0)
		record[1]: clib name as integer
		record[2]: func name as integer
		record[3]: pointer to function
*/

/*
 * g_data_clib[] contains the data from MachiKania compiler for this library
 */
void* g_data_clib_var[]={
	0, // will be g_gp, g_data[0][0],"0"
};

const void* const g_data_clib_core[]={
	lib_calloc_memory,//  g_data[1][0],"0"
	lib_delete,       //  g_data[1][1],"4"
};

const void* const g_data_clib_file[]={
#ifdef __DEBUG
	0 // Disabled in debug mode
#else
	FSInit,   // g_data[2][0],"0"
	FSfopen,  // g_data[2][1],"4"
	FSfclose, // g_data[2][2],"8"
	FSfread,  // g_data[2][3],"12"
	FSfwrite, // g_data[2][4],"16"
	FSfeof,   // g_data[2][5],"20"
	FSftell,  // g_data[2][6],"24"
	FSfseek,  // g_data[2][7],"28"
	FSrewind, // g_data[2][8],"32"
	FindFirst,// g_data[2][9],"36"
	FindNext, // g_data[2][10],"40"
	FSmkdir,  // g_data[2][11],"44"
	FSgetcwd, // g_data[2][12],"48"
	FSchdir,  // g_data[2][13],"52"
	FSremove, // g_data[2][14],"56"
	FSrename, // g_data[2][15],"60"
#endif
};

const void* const g_data_clib_video[]={
	start_composite,// g_data[3][0],"0"
	stop_composite, // g_data[3][1],"4"
	printchar,      // g_data[3][2],"8"
	printstr,       // g_data[3][3],"12"
	printnum,       // g_data[3][4],"16"
	printnum2,      // g_data[3][5],"20"
	cls,            // g_data[3][6],"24"
	vramscroll,     // g_data[3][7],"28"
	setcursorcolor, // g_data[3][8],"32"
	setcursor,      // g_data[3][9],"36"
	set_palette,    // g_data[3][10],"40"
	set_bgcolor,    // g_data[3][11],"44"
};

const void* const g_data_clib_graphic[]={
	g_pset,         // g_data[4][0],"0"
	g_putbmpmn,     // g_data[4][1],"4"
	g_clrbmpmn,     // g_data[4][2],"8"
	g_gline,        // g_data[4][3],"12"
	g_hline,        // g_data[4][4],"16"
	g_circle,       // g_data[4][5],"20"
	g_circlefill,   // g_data[4][6],"24"
	g_boxfill,      // g_data[4][7],"28"
	g_putfont,      // g_data[4][8],"32"
	g_printstr,     // g_data[4][9],"36"
	g_printnum,     // g_data[4][10],"40"
	g_printnum2,    // g_data[4][11],"44"
	g_color,        // g_data[4][12],"48"
};

const void* const g_data_clib_keyboard[]={
	shiftkeys, //  g_data[5][0],"0"
	ps2readkey,//  g_data[5][1],"4"
};

const void* const g_data_clib[]={
	&g_data_clib_var[0],     // g_data[0],"0"
	&g_data_clib_core[0],    // g_data[1],"4"
	&g_data_clib_file[0],    // g_data[2],"8"
	&g_data_clib_video[0],   // g_data[3],"12"
	&g_data_clib_graphic[0], // g_data[4],"16"
	&g_data_clib_keyboard[0],// g_data[5],"20"
};

// CLIB name used (name as 31 bit integer)
static unsigned int g_clib;

void init_clib(){
	// Initialize g_data_clib_var[]
	g_data_clib_var[0]=(void*)g_gp;
}

char* useclib_statement(){
	int i;
	int* cmpdata;
	do {
		next_position();
		i=check_var_name();
		if (i<65536) return ERR_SYNTAX;
		// Check if the clib already exists
		cmpdata_reset();
		while(cmpdata=cmpdata_find(CMPDATA_USECLIB)){
			if (cmpdata[1]==i) {
				// The clib was already defined.
				i=0;
				break;
			}
		}
		if (i) {
			// Load new file to define clib.
			g_clib=i;
			return ERR_COMPILE_CLIB;
		}
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
		} else {
			break;
		}
	} while(1);
	return 0;
}

void* call_clib_init(void** data, void* address){
	// Store gp and ra
	asm volatile("#":::"s0");
	asm volatile("#":::"ra");
	asm volatile("addu $t9,$a1,$zero");
	asm volatile("addu $s0,$gp,$zero");
	asm volatile("jalr $ra,$t9");
	asm volatile("addu $gp,$s0,$zero");
}

char* clib_main(){
	char* err;
	unsigned int begin,end,addr;
	void* clibdata;
	int* functions;
	int* got;
	int i,opos,adjust;
	char* clib;
	int record[3];
	// Insert CMPDATA_USELIB
	record[0]=g_clib;
	err=cmpdata_insert(CMPDATA_USECLIB,0,&record[0],1);
	if (err) return err;
	// Determine begin and end addresses
	begin=0x000fffff;
	end=0;
	while(1){
		err=hex_read_line();
		if (err) return err;
		if (g_hexline.type==1) {
			// EOF
			break;
		} else if (g_hexline.type==4) {
			// extended linear address
			addr=g_hexline.data[0];
			addr=addr<<8;
			addr|=g_hexline.data[0];
			addr=addr<<16;
		} else if (g_hexline.type==0) {
			// data
			addr&=0xffff0000;
			addr|=g_hexline.address;
			if (addr<0x00100000) {
				// Address is valid only when <0x00100000 (in the RAM)
				if (addr<begin) begin=addr;
				if (end<=addr) {
					end=addr+g_hexline.size;
				}
			}
		} else {
			// Unknown type
			return ERR_HEX_ERROR;
		}
	}
	if (end<begin) {
		hex_close_file();
		return "Wrong HEX file for CLIB";
	}
	hex_reinit_file();
	// Assign and clear region
	check_obj_space((end-begin+3)>>2);
	clib=(char*)&g_object[g_objpos];
	got=(int*)&g_object[g_objpos];
	for(i=0;i<((end-begin+3)>>2);i++) g_object[g_objpos++]=0;
	// Load binary from HEX file
	while(1){
		err=hex_read_line();
		if (err) return err;
		if (g_hexline.type==1) {
			// EOF
			break;
		} else if (g_hexline.type==4) {
			// extended linear address
			addr=g_hexline.data[0];
			addr=addr<<8;
			addr|=g_hexline.data[0];
			addr=addr<<16;
		} else if (g_hexline.type==0) {
			// data
			addr&=0xffff0000;
			addr|=g_hexline.address;
			for(i=0;i<g_hexline.size;i++){
				clib[addr-begin+i]=g_hexline.data[i];
			}
		} else {
			// Unknown type
			return ERR_HEX_ERROR;
		}
	}
	// Calculate the adjustment value.
	// Note that the address of clib_init() is 0x8000.
	adjust=(int)clib+0x8000-begin-0xA0008000;
	// Modify Global Offset Table (GOT)
	for(i=0;i<(0x8000-begin)>>2;i++){
		if ((got[i]&0xFFF00000)==0xA0000000) {
			got[i]+=adjust;
		}
	}
	// Call the clib_init() to get the address of clibdata array.
	clibdata=call_clib_init((void**)&g_data_clib[0],(void*)(0xA0008000+adjust));
	// Check version
	if (((int*)clibdata)[0]>SYSVERI) {
		return "Newer version of C library than MachiKania cannot be used.";
	}
	// Get functions array
	clibdata=(void*)(((int*)clibdata)[2])+adjust;
	functions=clibdata;
	// Construct CMPDATA_CLIBFUNC records.
	while(clib=(char*)functions[0]){
		clibdata=(void*)functions[1];
		functions+=2;
		// clib is function name as string
		// clibdata is address of C function
		clib+=adjust;
		clibdata+=adjust;
		i=str_to_name_int(clib);
		if (i<65536) return "Wrong C library function name";
		record[0]=g_clib;
		record[1]=i;
		record[2]=(int)clibdata;
		err=cmpdata_insert(CMPDATA_CLIBFUNC,0,&record[0],3);
		if (err) return err;
	}
	// Initial assembly is a jump statement to jump to the the following routine.
	g_object[0]=0x08000000 | ((((int)(&g_object[g_objpos]))&0x0FFFFFFF)>>2); // j xxxxxxxx
	// Insert initialization code
	g_object[g_objpos++]=0x3C050000|((((unsigned int)(0xA0008000+adjust))>>16)&0x0000FFFF); // lui   a1,xxxx
	g_object[g_objpos++]=0x34A50000|(((unsigned int)(0xA0008000+adjust))&0x0000FFFF);       // ori a1,a1,xxxx
	call_quicklib_code(call_clib_init,ASM_ADDIU_A0_ZERO_ | 0);
	// All done. Shift g_object as the beginning of main BASIC code.
	g_object+=g_objpos;
	g_objpos=0;
	return 0;
}

char* useclib_begin(char* buff){
	int i,j,cwd_id;
	char* err;
	char* clibname;
	char filename[11];
	int* record;
	// Remove a objects before USECLIB statement
	g_objpos=0;
	// Insert twp NOP assemblies. This will be replaced by branch statement.
	check_obj_space(2);
	g_object[g_objpos++]=0x00000000; // nop
	g_object[g_objpos++]=0x00000000; // nop
	// Construct HEX file name to open
	clibname=resolve_label(g_clib);
	for(i=0;filename[i]=clibname[i];i++);
	filename[i++]='.';
	filename[i++]='H';
	filename[i++]='E';
	filename[i++]='X';
	filename[i++]=0;
	// Open file in current directory
	err=hex_init_file(buff,filename);
	if (!err) {
		// HEX file found in current directory.
		err=clib_main();
		hex_close_file();
	} else {
		// Hex file not found in current directory.
		// Find it in LIB directory.
		if (!FSgetcwd(buff,256)) return ERR_UNKNOWN;
		for(i=0;buff[i];i++);
		cwd_id=cmpdata_get_id();
		if (!cwd_id) return ERR_UNKNOWN;
		err=cmpdata_insert(CMPDATA_TEMP,cwd_id,(int*)(&buff[0]),(i+1+3)>>2);
		if (err) return err;
		// Change current directory to class library directory
		for(i=0;buff[i]="\\LIB\\"[i];i++);
		for(j=0;buff[i++]=clibname[j];j++);
		buff[i]=0;
		FSchdir(buff);
		err=hex_init_file(buff,filename);
		if (!err) {
			// HEX file found in LIB directory
			err=clib_main();
			hex_close_file();
		} else {
			err=ERR_NO_CLIB;
		}
		// Restore current dirctory
		cmpdata_reset();
		while(record=cmpdata_find(CMPDATA_TEMP)){
			if ((record[0]&0xffff)==cwd_id) break;
		}
		if (!record) return ERR_UNKNOWN;
		FSchdir((char*)(&record[1]));
		cmpdata_delete(record);
	}
	return err;
}

void lib_clib(int* params, void* address){
	// Store gp and ra
	asm volatile("#":::"s0");
	asm volatile("#":::"ra");
	asm volatile("addu $t9,$a1,$zero");
	asm volatile("addu $s0,$gp,$zero");
	asm volatile("lw $a3,16($a0)");
	asm volatile("lw $a2,12($a0)");
	asm volatile("lw $a1,8($a0)");
	asm volatile("lw $a0,4($a0)");
	asm volatile("jalr $ra,$t9");
	asm volatile("addu $gp,$s0,$zero");
}

char* clib_method(char type){
	char* err;
	int* record;
	int clib,func;
	int numparams,stack,spos;
	// CLIB name
	next_position();
	spos=g_srcpos;
	clib=check_var_name();
	if (clib<65536) return ERR_SYNTAX;
	// Check if the clib exists
	cmpdata_reset();
	while (record=cmpdata_find(CMPDATA_USECLIB)){
		if (record[1]==clib) break;
	}
	if (!record) {
		g_srcpos=spos;
		return ERR_NO_CLIB;
	}
	if (g_source[g_srcpos++]!=':') return ERR_SYNTAX;
	if (g_source[g_srcpos++]!=':') return ERR_SYNTAX;
	// Function name
	func=check_var_name();
	if (func<65536) return ERR_SYNTAX;
	cmpdata_reset();
	while (record=cmpdata_find(CMPDATA_CLIBFUNC)){
		if (record[1]==clib && record[2]==func) break;
	}
	if (!record) return "Function not found in the C library";
	// Check type and following syntax
	if (type==',') {
		// This is CLIB(CLASS:METHOD) style
	} else {		
		// Type is either 0 (integer), $ (string), or # (float)
		// This is CLASS::METHOD() style
		if (type) {
			if (g_source[g_srcpos++]!=type) return ERR_SYNTAX;
		}
		if (g_source[g_srcpos++]!='(') return ERR_SYNTAX;
	}
	// Construct parameter(s)
	next_position();
	numparams=0;
	if (','==type && g_source[g_srcpos]!=',') {
		// This is CLIB(CLASS:METHOD) style
		// no parameter
	} else if (','!=type && g_source[g_srcpos]==')') {
		// This is CLASS::METHOD() style
		// no parameter
	} else {
		// There is parameter(s)
		do {
			numparams++;
			if (4<numparams) return ERR_SYNTAX;
			else if (1==numparams) {
				check_obj_space(1);
				stack=g_objpos++;
			}
			if (g_source[g_srcpos]==',') g_srcpos++;
			g_object[stack]=0x27BD0000|(65536-numparams*4);                     // addiu sp,sp,-xx
			err=get_stringFloatOrValue();
			if (err) return err;
			check_obj_space(1);
			g_object[g_objpos++]=0xAFA20000|(numparams*4);                      // sw    v0,xx(sp)
			next_position();
		} while (g_source[g_srcpos]==',');
	}
	// Call CLIB
	check_obj_space(2);
	g_object[g_objpos++]=0x3C050000|(((unsigned int)record[3])>>16);        // lui   a1,xxxx
	g_object[g_objpos++]=0x34A50000|(((unsigned int)record[3])&0x0000FFFF); // ori   a1,a1,xxxx
	call_quicklib_code(lib_clib,ASM_ADDU_A0_SP_ZERO);	// All done
	check_obj_space(1);
	if (numparams) g_object[g_objpos++]=0x27BD0000|(numparams*4);           // addiu sp,sp,xx
	return 0;
}

char* clib_statement(){
	return clib_method(',');
}