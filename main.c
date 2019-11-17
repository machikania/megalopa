/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

// main.c
// MachiKania BASIC System Ver Megalopa
// KM-BASIC �����J�����s�� for PIC32MX370F512H by K.Tanaka

// ���p�V�X�e��
// ps2keyboard370f.X.a : PS/2�L�[�{�[�h���̓V�X�e�����C�u����
// lib_videoout_megalopa.X.a : �J���[�r�f�I�M���o�̓V�X�e�����C�u����
// sdfsio370fLib.a �F SD�J�[�h�A�N�Z�X�p���C�u����


/*
	PIC32MX �y���t�F�����g�p��
	
	���荞��
		NTSC,   Timer2, vector  8, priority 5
		NTSC,   OC5,    vector 22, priority 5
		NTSC,   OC2,    vector 10, priority 5
		NTSC,   OC1,    vector  6, priority 5
		PS/2,   CNF,    vector 33, priority 6
		PS/2,   Timer5, vector 20, priority 4
		MUSIC,  CS0,    vector  1, priority 2
		SERIAL, UART,   vector 31, priority 3
		TIMER,  Timer1, vector  4, priority 3
		INT,    CS1,    vector  2, priority 1
	
	�^�C�}�[
		Timer1 ���g�p
		Timer2 NTSC
		Timer3 MUSIC/PWM
		Timer4 MUSIC
		Timer5 PS/2
	
	DMA
		DMA0 ���g�p
		DMA1 MUSIC
		DMA2 MUSIC
		DMA3 PS/2
	
	Output compair
		OC1 NTSC
		OC2 NTSC
		OC3 MUSIC/PWM
		OC4 MUSIC/PWM
		OC5 NTSC
	
	UART
		UART1 �V���A���ʐM
		UART2 ���g�p
	
	SPI
		SPI1 SPI�ʐM�i�\��j
		SPI2 �}���`���f�B�A�J�[�h
	
	I2C
		I2C1 I2C�ʐM�i�\��j
		I2C2 ���g�p
	
	�|�[�g�g�p
		B0  I/O, AN0
		B1  I/O, AN1
		B2  I/O, AN2
		B3  I/O, AN3
		B4  I/O, AN4
		B5  I/O, AN5
		B6  I/O, AN6
		B7  I/O, AN7
		B8  I/O, AN8
		B9  I/O, AN9
		B10 I/O, AN10
		B11 I/O, AN11
		B12 I/O, AN12
		B13 I/O, AN13
		B14 I/O, AN14
		B15 I/O, AN15
		C12 OSC1 (Crystal)
		C13 U1TX (UART)
		C14 U1RX (UART)
		C15 OSC2 (Crystal)
		D0  SW_DOWN
		D1  SW_LEFT
		D2  SW_UP
		D3  SW_RIGHT
		D4  SW_START
		D5  SW_FIRE
		D6  
		D7  
		D8  
		D9  SPI1_CS (SPI)
		D10 PWM1
		D11 PWM2
		E0  NTSC
		E1  NTSC
		E2  NTSC
		E3  NTSC
		E4  NTSC
		E5  I/O, AN22
		E6  I/O, AN23
		E7  I/O, AN27
		F0  PS/2 DAT
		F1  PS/2 CLK
		F2  SDI1 (SPI)
		F3  SPI2_CS (MMC)
		F4  AUDIO_R
		F5  AUDIO_L
		F6  SCK1 (SPI)
		G2  SCL1 (I2C)
		G3  SDA1 (I2C)
		G6  SCK2 (MMC)
		G7  SDI2 (MMC)
		G8  SDO2 (MMC)
		G9  SDO1 (SPI)
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"
#include "editor.h"
#include "interface/keyinput.h"
#include "main.h"

//�O�t���N���X�^�� with PLL (20/3�{)
//�N���X�^����3.579545�~4��14.31818MHz
#pragma config FSRSSEL = PRIORITY_7
#pragma config PMDL1WAY = OFF
#pragma config IOL1WAY = OFF
//#pragma config FUSBIDIO = OFF
//#pragma config FVBUSONIO = OFF
#pragma config FPLLIDIV = DIV_3
#pragma config FPLLMUL = MUL_20
//#pragma config UPLLIDIV = DIV_1
//#pragma config UPLLEN = OFF
#pragma config FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL
#pragma config FSOSCEN = OFF
#pragma config IESO = OFF
#pragma config POSCMOD = XT
#pragma config OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1
#pragma config FCKSM = CSDCMD
#pragma config FWDTEN = OFF
#pragma config DEBUG = OFF
#pragma config PWP = OFF
#pragma config BWP = OFF
#pragma config CP = OFF

#define mBMXSetRAMKernProgOffset(offset)	(BMXDKPBA = (offset))
#define mBMXSetRAMUserDataOffset(offset)	(BMXDUDBA = (offset))
#define mBMXSetRAMUserProgOffset(offset)	(BMXDUPBA = (offset))

// INI�t�@�C���w��L�[���[�h�i8�����ȓ��j
const char InitKeywords[][9]={
	"106KEY","101KEY","NUMLOCK","CAPSLOCK","SCRLLOCK","WIDTH36","WIDTH48","WIDTH80"
};
unsigned char initialvmode;

void freadline(char *s,FSFILE *fp){
// �t�@�C������1�s�ǂݍ��݁A�z��s�ɕԂ�
// �ő�8�����܂ŁB9�����ȏ�̏ꍇ����
// #�܂���0x20�ȉ��̃R�[�h���������ꍇ�A�ȍ~�͖���
// s:9�o�C�g�ȏ�̔z��
// fp:�t�@�C���|�C���^
	int n;
	char c,*p;
	n=0;
	p=s;
	*p=0;
	while(n<=8){
		if(FSfread(p,1,1,fp)==0 || *p=='\n'){
			*p=0;
			return;
		}
		if(*p=='#'){
			*p=0;
			break;
		}
		if(*p<=' '){
			if(n>0){
				*p=0;
				break;
			}
			continue;
		}
		p++;
		n++;
	}
	if(n>8) *s=0; //9�����ȏ�̕�����̏ꍇ�͖���
	//�ȍ~�̕����͖���
	while(FSfread(&c,1,1,fp) && c!='\n') ;
}
int searchinittext(char *s){
// InitKeywords�z��̒����當����s��T���A�ʒu�����ꍇ���Ԗڂ���Ԃ�
// ������Ȃ������ꍇ-1��Ԃ�
	int i;
	char *p1;
	const char *p2;
	for(i=0;i<sizeof(InitKeywords)/sizeof(InitKeywords[0]);i++){
		p1=s;
		p2=InitKeywords[i];
		while(*p1==*p2){
			if(*p1==0) return i;
			p1++;
			p2++;
		}
	}
	return -1;
}
void readinifile(void){
	FSFILE *fp;
	char inittext[9];
	if (!g_fs_valid) return;

	fp=FSfopen(INIFILE,"r");
	if(fp==NULL) return;
	printstr("Initialization File Found\n");
	lockkey=0; //INI�t�@�C�������݂���ꍇ�ALock�֘A�L�[��INI�t�@�C���ɏ]��
	while(1){
		if(FSfeof(fp)) break;
		freadline(inittext,fp);
		switch(searchinittext(inittext)){
			case 0:
				keytype=0;//���{��L�[�{�[�h
				break;
			case 1:
				keytype=1;//�p��L�[�{�[�h
				break;
			case 2:
				lockkey|=2;//Num Lock
				break;
			case 3:
				lockkey|=4;//CAPS Lock
				break;
			case 4:
				lockkey|=1;//Scroll Lock
				break;
			case 5:
				initialvmode=VMODE_STDTEXT;
				break;
			case 6:
				initialvmode=VMODE_WIDETEXT;
				break;
			case 7:
				initialvmode=VMODE_MONOTEXT;
				break;
		}
	}
	FSfclose(fp);
}

void printhex8(unsigned char d){
	printchar("0123456789ABCDEF"[d>>4]);
	printchar("0123456789ABCDEF"[d&0x0f]);	
}

void printhex16(unsigned short d){
	printhex8(d>>8);
	printhex8(d&0x00ff);
}

void printhex32(unsigned int d){
	printhex16(d>>16);
	printhex16(d&0x0000ffff);
}

int main(void){
	char *appname,*s;
	int use_editor;

	/* �|�[�g�̏����ݒ� */
	CNPUB = 0xFFFF; // PORTB�S�ăv���A�b�v(I/O)
	TRISB = 0xFFFF; // PORTB�S�ē���
	CNPUC = 0x4000; // PORTC14�v���A�b�v(U1RX)
	TRISC = 0x4000; // PORTC14�ȊO�͏o��
	TRISD = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// �{�^���ڑ��|�[�g���͐ݒ�
	CNPUE = 0x00E0; // PORTE5-7�v���A�b�v(I/O)
	TRISE = 0x00E0; // PORTE0-4�o��5-7����
	CNPUF = 0x0004; // PORTF2�v���A�b�v(SDI1)
	TRISF = 0x0004; // PORTF2�ȊO�͏o��
	TRISG = 0x0080; // PORTG7�ȊO�͏o��

	ANSELB = 0x0000; // �S�ăf�W�^��
	ANSELD = 0x0000; // �S�ăf�W�^��
	ANSELE = 0x0000; // �S�ăf�W�^��
	ANSELG = 0x0000; // �S�ăf�W�^��
	CNPUDSET=KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// �v���A�b�v�ݒ�
	CNPUFSET=0x0004; // PORTF2 (SDI1) �v���A�b�v
	ODCF = 0x0003;	//RF0,RF1�̓I�[�v���h���C��
	ODCG = 0x000c;//RG2,RG3�̓I�[�v���h���C��
	LATGSET = 0x000c;

	// ���Ӌ@�\�s�����蓖��
	SDI2R = 1; //RPG7��SDI2�����蓖��
	RPG8R = 6; //RPG8��SDO2�����蓖��

	// Make RAM executable. See also "char RAM[RAMSIZE]" in globalvars.c
	mBMXSetRAMKernProgOffset(PIC32MX_RAMSIZE-RAMSIZE);
	mBMXSetRAMUserDataOffset(PIC32MX_RAMSIZE);
	mBMXSetRAMUserProgOffset(PIC32MX_RAMSIZE);

	init_composite(); // �r�f�I�������N���A�A���荞�ݏ������A�J���[�r�f�I�o�͊J�n
	setcursor(0,0,COLOR_NORMALTEXT);

	// Show blue screen if exception before soft reset.
	blue_screen();

	// ���s��HEX�t�@�C������HEXFILE�ƈ�v���邩�ǂ���
	use_editor=0;
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=HEXFILE;
	while(*s++==*appname++) {
		if(*s==0) {
			//�e�L�X�g�G�f�B�^�[�Ăяo��
			use_editor=1;
			break;
		}
	}

	printstr("MachiKania BASIC System\n");
	printstr(" Ver "SYSVER1" "SYSVER2" by KENKEN\n");
	printstr("BASIC Compiler "BASVER"\n");
	printstr(" by Katsumi\n\n");
	//SD�J�[�h�t�@�C���V�X�e��������
	setcursorcolor(COLOR_NORMALTEXT);
	printstr("Init File System...");
	// Initialize the File System
	g_fs_valid=FSInit(); //�t�@�C���V�X�e��������
	if(g_fs_valid==FALSE){
		if (use_editor || !g_objpos_mos) {
			// Editor���[�h�̏ꍇ�A�y�сABAS�t�@�C���ǂݍ��݃��[�h�̏ꍇ
			//�G���[�̏ꍇ��~
			setcursorcolor(COLOR_ERRORTEXT);
			printstr("\nFile System Error\n");
			printstr("Insert Correct Card\n");
			printstr("And Reset\n");
			while(1) asm("wait");
		} else {
			// MOS���[�h�̏ꍇ�́A���Ɠ��A�g���C
			// �G���[�\���̌�A������
			g_fs_valid=FSInit();
			if(g_fs_valid==FALSE) g_fs_valid=FSInit();
		}
	}
	if (g_fs_valid) printstr("OK\n");
	else printstr("Failed\n");

	// ����������
	OC4RS=LATFbits.LATF5 ? 0xff:0x00;
	OC3RS=LATFbits.LATF4 ? 0xff:0x00;
	init_music();

	initialvmode=VMODE_STDTEXT; // �W���e�L�X�g���[�h�i36�����j
	lockkey=2; // NumLock�L�[�I��
	keytype=0; // ���{��L�[�{�[�h
	readinifile(); //INI�t�@�C���ǂݍ���
	printstr("Init PS/2...");
	wait60thsec(30); //0.5�b�҂�
	if(ps2init()){ //PS/2������
		//�L�[�{�[�h��������Ȃ��ꍇ
		printstr("Keyboard Not Found\n");
	}
	else printstr("OK\n");

	wait60thsec(60); //1�b�҂�

	set_videomode(initialvmode,0); //�r�f�I���[�h�ؑ�

	// ���s��HEX�t�@�C������HEXFILE�ƈ�v�����ꍇ�̓G�f�B�^�N��
	if(use_editor) texteditor(); //�e�L�X�g�G�f�B�^�[�Ăяo��

	g_disable_break=1; // Break�L�[������
	// buttonmode(); //�{�^���L����
	if (g_objpos_mos) {
		// MOS����R�[�h���R�s�[���Ď��s
		runbasic(0,2);
	} else {
		// ���s��HEX�t�@�C�����́u.HEX�v���u.BAS�v�ɒu��������BAS�t�@�C�������s
		appname=(char*)FILENAME_FLASH_ADDRESS;
		s=tempfile;
		while(*appname!='.') *s++=*appname++;
		appname=".BAS";
		while(*appname!=0) *s++=*appname++;
		*s=0;
		runbasic(tempfile,0);
	}
	while(1) asm(WAIT);
}
