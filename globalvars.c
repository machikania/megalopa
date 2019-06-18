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
#include "main.h"

// Store g_object/g_objpos values for MOS at jest before MACHIKANIA_OBJ_ADDRESS.
// When these values are 0, MOS code is not loaded.
const volatile int __attribute__((address(MACHIKANIA_OBJ_ADDRESS-16))) _reserved1_mos=0;
const volatile int __attribute__((address(MACHIKANIA_OBJ_ADDRESS-12))) _reserved2_mos=0;
const volatile int __attribute__((address(MACHIKANIA_OBJ_ADDRESS-8)))  g_object_mos=0;
const volatile int __attribute__((address(MACHIKANIA_OBJ_ADDRESS-4)))  g_objpos_mos=0;

// Contain the valus of $gp and $s6 (GPR of MIPS32)
int g_gp;
int g_s6;

// Line data when compiling
int g_line;
int g_fileline;

// Contain the address to which return in "END" statement.
int g_end_addr;

// Following vars are used in value.c and string.c.
// These define the depth of stack pointer used for
// handling values and strings.
int g_sdepth;
int g_maxsdepth;
char g_allow_shift_obj;

// Following var shows what type of variable was defined
// in compiling the last code.
enum variable g_lastvar;

// Vars used for handling constant integer
int g_intconst;
char g_valueisconst;

// Global vars associated to RAM
char* g_source;
int g_srcpos;
int* g_object;
int g_objpos;
int* g_objmax;
char RAM[RAMSIZE] __attribute__((persistent,address(0xA0000000+PIC32MX_RAMSIZE-PERSISTENT_RAM_SIZE)));
unsigned int g_ex_data[EXCEPTION_DATA_SIZE/4] __attribute__((persistent,address(0xA0000000+PIC32MX_RAMSIZE-EXCEPTION_DATA_SIZE)));

// Global area for vars A-Z and three temporary string pointers
int g_var_mem[ALLOC_BLOCK_NUM];
unsigned short g_var_pointer[ALLOC_BLOCK_NUM];
unsigned short g_var_size[ALLOC_BLOCK_NUM];

// Flag to use temporary area when compiling
char g_temp_area_used;

// Flag to use option nolinenum
char g_option_nolinenum;

// Heap area
int* g_heap_mem;
int g_max_mem;

// Random seed
unsigned int g_rnd_seed;

// Enable/disable Break keys
char g_disable_break;

// Font data used for PCG
unsigned char* g_pcg_font;

// Use or do not use graphic
char g_use_graphic;

// Pointer to graphic RAM
unsigned short* g_graphic_area;

// Parameter-containing block used for library
int* g_libparams;

// Number of long name variables
int g_long_name_var_num;

// Flag for active music/sound/wave function
char g_music_active;

// Class name being compiled
int g_class;
// Flag (and class name) to compile class file
int g_compiling_class;
// Number of classes used
unsigned char g_num_classes;
// OPTION FASTFIELD
char g_option_fastfield;

// Flag if file system has been initialized
char g_fs_valid;

// Result of reading a HEX file line
HEXLINE g_hexline;

// Stack to store $gp
void* g_gp_stack[8];
void* g_gp_stack_pointer=&g_gp_stack[0];

// General purpose integer used for asigning value with pointer
int g_temp;
