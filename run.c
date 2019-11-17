/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"
#include "editor.h"
#include "main.h"

char* printdec(int num){
	char str[11];
	int i;
	if (num<0) {
		printchar('-');
		num=0-num;
	}
	for(i=10;0<i;i--){
		if (num==0 && i<10) break;
		str[i]='0'+rem10_32(num);
		num=div10_32(num);
	}
	for(i++;i<11;i++) {
		printchar(str[i]);
	}
}

#define RUNMODE_COMPILE_AND_RUN 0
#define RUNMODE_COMPILE_ONLY    1
#define RUNMODE_COPY_AND_RUN    2

int runbasic(char *appname,int mode){
// BASICソースのコンパイルと実行
// appname 実行するBASICソースファイル
// mode 0:コンパイルと実行、1:コンパイルのみで終了、2:コンパイル済みオブジェクトを実行
//
// 戻り値
//　　0:正常終了
//　　-1:ファイルエラー
//　　-2:リンクエラー
//　　1以上:コンパイルエラーの発生行（行番号ではなくファイル上の何行目か）
	int i,j;
	char* buff;
	char* err;

	// Set grobal pointer
	g_gp=get_gp();
	// Set buffer positions
	buff=(char*)&(RAM[RAMSIZE-512]);
	// Set object positions
	g_object=(int*)(&RAM[0]);
	g_objpos=0;
	g_objmax=g_object+(RAMSIZE-512)/4; // Buffer area excluded.
	// Clear object area
	for(i=0;i<RAMSIZE/4;i++) g_object[i]=0x00000000;

	if (mode!=RUNMODE_COPY_AND_RUN) {
		// Check file error
		err=init_file(buff,appname);
		if (err) {
			setcursorcolor(COLOR_ERRORTEXT);
			printstr("Can't Open ");
			printstr(appname);
			printchar('\n');
			return -1;
		}
		close_file();
	}

	// Initialize parameters
	g_pcg_font=0;
	g_use_graphic=0;
	g_graphic_area=0;
	clearscreen();
	setcursor(0,0,7);
	g_long_name_var_num=0;

	// Initialize music system
	init_music();

	// Initialize clib
	init_clib();

	printstr("BASIC "BASVER"\n");
	wait60thsec(15);

	printstr("Compiling...");

	if (mode==RUNMODE_COPY_AND_RUN) {
		// Set g_object/g_objpos for library functions like lib_read().
		// The g_object/g_objpos values are stoared just before MOS.
		g_object=(int*)g_object_mos;
		g_objpos=g_objpos_mos;
		// Copy the object from MOS
		j=(int)(&g_object[g_objpos])-(int)(&RAM[0]);
		appname=(char*)MACHIKANIA_OBJ_ADDRESS;
		for(i=0;i<j;i++) RAM[i]=appname[i];
	} else {
		// Initialize compiler
		cmpdata_init();
		// Compile the file
		i=compile_and_link_main_file(buff,appname);
		if (i) return i;
	}
	// All done
	printstr("done\n");
	if(mode==RUNMODE_COMPILE_ONLY) return 0; //コンパイルのみの場合
	wait60thsec(15);

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
	// Initialize file system
	lib_file(FUNC_FINIT,0,0,0);

	// Assign memory
	set_free_area((void*)(g_object+g_objpos),(void*)(&RAM[RAMSIZE]));

	// Warm up environment
	pre_run();
	init_timer();

	// Execute program
	// Start program from the beginning of RAM.
	// Work area (used for A-Z values) is next to the object code area.
	start_program((void*)(&(RAM[0])),(void*)(&g_var_mem[0]));
	printstr("\nOK\n");

	// Cool down environment
	post_run();
	lib_file(FUNC_FINIT,0,0,0);
	stop_timer();

	return 0;
}

int create_self_running_hex(char* hexfilename){
	int i,j,fpos;
	FSFILE* dst_file;
	char* buff;
	char* err;
	unsigned int* object;
	unsigned int addr,adjust;
	unsigned int data[4];
	// Set buffer positions
	buff=(char*)&(RAM[RAMSIZE-512]);
	// Open original and destination HEX files.
	if (hex_init_file(buff,HEXFILE)) return -1;
	dst_file=FSfopen(hexfilename,"w");
	if (!dst_file) {
		hex_close_file();
		return -1;
	}
	// Copy the HEX file from original MachiKania, except for MOS.
	addr=0;
	fpos=0;
	while(1) {
		if (0==((fpos++)&0x3ff)) {
			// Indicator works every 1024 lines
			printchar('.');
		}
		err=hex_read_line();
		if (err) break;
		// Determine type and current address.
		// If address is OK, write it to destination.
		if (g_hexline.type==1) {
			// EOF
			break;
		} else if (g_hexline.type==4) {
			// extended linear address
			addr=g_hexline.data[0];
			addr=addr<<8;
			addr|=g_hexline.data[1];
			addr=addr<<16;
			// Highest bit will be 1 for 0x9D0xxxxx instead of 0x1D0xxxxx
			addr|=0x80000000;
			// Write this anyway
			err=hex_write(dst_file);
			if (err) break;
		} else if (g_hexline.type==0) {
			// data
			addr&=0xffff0000;
			addr|=g_hexline.address;
			// Write this line if not in MOS
			if (addr<MACHIKANIA_OBJ_INFO || FILENAME_FLASH_ADDRESS<=addr) {
				err=hex_write(dst_file);
				if (err) break;
			}
		} else {
			// Unknown type
			err=ERR_HEX_ERROR;
			break;
		}
	}
	hex_close_file();
	if (err) {
		FSfclose(dst_file);
		printstr(err);
		return -1;
	}
	// Save MACHIKANIA_OBJ_INFO
	addr=MACHIKANIA_OBJ_INFO;
	err=hex_write_address(dst_file,addr>>16);
	if (!err) {
		data[2]=(int)g_object;
		data[3]=(int)g_objpos;
		err=hex_write_data_16(dst_file,addr&0xffff,&data[0]);
	}
	if (err) {
		FSfclose(dst_file);
		printstr(err);
		return -1;
	}
	// Add MOS. Adjustment is for changing address from RAM area to MOS.
	addr=-1;
	object=(unsigned int*)(&RAM[0]);
	adjust=(unsigned int)MACHIKANIA_OBJ_ADDRESS-(unsigned int)object;
	while(object<(unsigned int*)(&g_object[g_objpos])){
		if (0==((fpos++)&0x3ff)) {
			// Indicator works every 1024 lines
			printchar('.');
		}
		if ((0x7fff0000 & ((unsigned int)object+adjust)) != addr) {
			// Construct a hex line for providing Extended linear addres
			addr=0x7fff0000 & ((unsigned int)object+adjust);
			err=hex_write_address(dst_file,addr>>16);
			if (err) break;
		}
		// Construct a hex line for data
		err=hex_write_data_16(dst_file,((unsigned int)object+adjust)&0xffff,object);
		if (err) break;
		// All OK for these 4 words (16 bytes).
		object+=4;
		err=0;
	}
	if (err) {
		FSfclose(dst_file);
		printstr(err);
		return -1;
	}
	// All done. Write EOF
	err=hex_write_eof(dst_file);
	FSfclose(dst_file);
	if (err) return -1;
	return 0;
}
