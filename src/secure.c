#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "main.h"
#include "eggdrop.h"
#include "users.h"
#include "chan.h"
#include "proto.h"
#include "darkmage.h"

extern char hostfile[];
char tmp[512],tmp2[512],tmp3[512];
int tmp4;
FILE *file,*file2;

/* MISC: split() {{{ */
void split(char *first, char *rest)
{
	char divider = ' ';
	char *p;
	p = strchr(rest, divider);
	if (p == NULL) {
		if ((first != rest) && (first != NULL))
			first[0] = 0;
		return;
	}
	*p = 0;
	if (first != NULL)
		strcpy(first, rest);
	if (first != rest)
		strcpy(rest, p + 1);
}
/* }}} */
/* SECURE: secpass() {{{ */
int secpass(char *password) {
	int ucase, lcase, other, pl, i, check_it;
	pl = strlen(password);
 
	if (pl < 7)
		return 0;
	other = ucase = lcase = 0;
	for(i=0; i < pl; i++) {
		check_it = (int)password[i];
		if(check_it < 58 && check_it >	47)  other = 1;
		if(check_it > 64 && check_it <	91)  ucase = 1;
		if(check_it > 96 && check_it < 123)  lcase = 1;
	}
	if ((ucase+lcase+other)==3) {
		return 1;
	}
	return 0;
}
/* }}} */
/* SECURE: encrypt_file() {{{ */
int encrypt_file(char *cfgfile) {
	struct stat buffer;
	if (stat (cfgfile, &buffer) == 0) {
		char stuff[1024];
		FILE *ecfg, *dcfg;
		ecfg = fopen(cfgfile, "r");
		dcfg = fopen(".tmp1", "w");
		while (fgets(stuff, sizeof stuff, ecfg)!= NULL) {
			fprintf(dcfg, "%s\n", encrypt_string(GOD, stuff));
		}
		fclose(ecfg);
		fclose(dcfg);
		return 1;
	} else {
		putlog(LOG_MISC, "!", "crypt error: could not open '%s'", cfgfile);
		return 0;
	}
	return 0;
}
/* }}} */
/* SECURE: decrypt_file() {{{ */
int decrypt_file(char *cfgfile2) {
	struct stat buffer;
	if (stat (cfgfile2, &buffer) == 0) {
		char stuff2[1024];
		FILE *ecfg2, *dcfg2;
		ecfg2 = fopen(cfgfile2, "r");
		dcfg2 = fopen(".tmp2", "w");
		while (fgets(stuff2, sizeof stuff2, ecfg2)!= NULL) {
			fprintf(dcfg2, "%s", decrypt_string(GOD, stuff2));
		}
		fclose(ecfg2);
		fclose(dcfg2);
		return 1;
	} else {
		putlog(LOG_MISC, "!", "crypt error: could not open '%s'", cfgfile2);
		return 0;
	}
	return 0;
}
/* }}} */

/*
 * vim: ft=c sw=4 ts=4 noet:
 */
