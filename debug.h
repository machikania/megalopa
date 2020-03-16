/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

/*
	This file is shared by Megalopa and Zoea
*/

#ifdef __DEBUG

// Pseudo reading config setting for debug mode
extern unsigned int g_DEVCFG1;
#define DEVCFG1 g_DEVCFG1

// Do not use PS/2 keyboard
#define ps2init() not_ps2init_but_init_Timer1()
int not_ps2init_but_init_Timer1();

// Do not use asm("wait") but use asmbly for "drawcount++"
#undef WAIT
#define WAIT "la $k0,%0"::"i"(&drawcount));\
	asm volatile("lhu $k1,0($k0)");\
	asm volatile("addiu $k1,$k1,1");\
	asm volatile("sh $k1,0($k0)"

#endif // __DEBUG

// key waiting macro

#define debug_wait() lineinput((char*)&g_temp,3)
#define debug_wait_char(x) do {\
		printchar(x); \
		lineinput((char*)&g_temp,3);\
	} while(0)	
