/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This script is to make lines of reserved words for long var names.
*/

function name2int(name){
	var g_source=name+'$';
	var g_srcpos=0;
	var prevpos=0;
	var i=0;
	var b1=g_source.charCodeAt(0);
	do {
		// First character must be A-Z
		// From second, A-Z and 0-9 can be used.
		i*=36;
		if ('0'.charCodeAt(0)<=b1 && b1<='9'.charCodeAt(0)) {
			i+=b1-'0'.charCodeAt(0);
		} else if (g_srcpos==prevpos) {
			// First character must be A-Z.
			// Subtract 9, resulting 1-26 but not 10-35.
			// This subtraction is required to maintain
			// final number being <0x80000000.
			i+=b1-'A'.charCodeAt(0)+1;
		} else {
			i+=b1-'A'.charCodeAt(0)+10;
		}
		g_srcpos++;
		b1=g_source.charCodeAt(g_srcpos);
	} while ('0'.charCodeAt(0)<= b1 && b1<='9'.charCodeAt(0) || 'A'.charCodeAt(0)<=b1 && b1<='Z'.charCodeAt(0));
	return i+65536;
}

var namearray=[
	'ABS',
	'ACOS',
	'ARGS',
	'ASC',
	'ASIN',
	'ATAN',
	'ATAN2',
	'BREAK',
	'CALL',
	'CDATA',
	'CEIL',
	'CHR',
	'CIRCLE',
	'CLEAR',
	'CLS',
	'COLOR',
	'COS',
	'COSH',
	'CREAD',
	'CURSOR',
	'DATA',
	'DEC',
	'DELETE',
	'DIM',
	'DO',
	'ELSE',
	'ELSEIF',
	'END',
	'ENDIF',
	'EXEC',
	'EXP',
	'FABS',
	'FCLOSE',
	'FEOF',
	'FGET',
	'FGETC',
	'FIELD',
	'FILE',
	'FINPUT',
	'FLEN',
	'FLOAT',
	'FLOOR',
	'FMOD',
	'FOPEN',
	'FOR',
	'FPRINT',
	'FPUT',
	'FPUTC',
	'FSEEK',
	'GCLS',
	'GCOLOR',
	'GETDIR',
	'GOSUB',
	'GOTO',
	'GPRINT',
	'HEX',
	'IF',
	'INKEY',
	'INPUT',
	'INT',
	'KEYS',
	'LABEL',
	'LEN',
	'LET',
	'LINE',
	'LOG',
	'LOG10',
	'LOOP',
	'MODF',
	'MUSIC',
	'NEXT',
	'NEW',
	'NOT',
	'PCG',
	'PEEK',
	'PEEK16',
	'PEEK32',
	'PI',
	'POINT',
	'POKE',
	'POKE16',
	'POKE32',
	'POW',
	'PRINT',
	'PSET',
	'PUBLIC',
	'PUTBMP',
	'READ',
	'REM',
	'RETURN',
	'RND',
	'SCROLL',
	'SETDIR',
	'SGN',
	'SIN',
	'SINH',
	'SOUND',
	'SQRT',
	'SYSTEM',
	'TAN',
	'TANH',
	'TVRAM',
	'UNTIL',
	'USEPCG',
	'USEVAR',
	'VAL',
	'VAR',
	'WAIT',
	'WEND',
	'WHILE',
	'WIDTH',
	// For megalopa
	'// For Megalopa',
	'OUT',
	'OUT8H',
	'OUT8L',
	'OUT16',
	'IN',
	'IN8H',
	'IN8L',
	'IN16',
	'ANALOG',
	'PWM',
	'SERIAL',
	'SPI',
	'I2C',
];
var result='';
for(key in namearray){
	var nameint='0000'+name2int(namearray[key]).toString(16);
	nameint=nameint.substr(nameint.length-8);
	if (namearray[key].substr(0,2)=='//') {
		// This is a comment
		result+=namearray[key]+"\r\n";
	} else {
		result+="\t0x"+nameint+", /*"+namearray[key]+"*/\r\n";
	}
}

var fso = new ActiveXObject("Scripting.FileSystemObject");
var file = fso.CreateTextFile("result.txt");
file.Write(result);
file.Close();

WScript.Echo('Done!');

