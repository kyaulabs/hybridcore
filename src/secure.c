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
char tmp[255],tmp2[255],tmp3[255];
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
/* HOSTFILE: remtelnet() {{{ */
int remtelnet(char *par)
{
	char host[512];
	if (!par[0])
		return 0;
	split(host, par);
	if (host != NULL)
		if (!host[0]) {
			strcpy(host, par);
			par[0] = 0;
		}

	sprintf(tmp3, "%s\n", host);
	tmp4 = 0;
    /* create a hostfile if it doesn't exist */
	struct stat buffer;
	if (stat (hostfile, &buffer) != 0) {
	  file = fopen(hostfile, "a");
	  fprintf(file, "\n");
	  fclose(file);
	  //nfree(file);
	  putlog(LOG_MISC, "*", "Created new hostfile..");
	}
	file = fopen(hostfile, "rt");
	sprintf(tmp, "%s~new", hostfile);
	file2 = fopen(tmp, "wt");
	//while (!feof(file)) {
		//fgets(tmp2, sizeof tmp2, file);
	while (fgets(tmp2, sizeof tmp2, file)!= NULL) {
		//if (feof(file)) break;
		if (strcasecmp(tmp2, tmp3) == 0) tmp4 = 1;
		if (tmp4 == 0) { fputs(tmp2, file2); }
		if (tmp4 == 2) { fputs(tmp2, file2); }
		if (tmp4 == 1) { tmp4 = 2; }
	}
	fclose(file);
	fclose(file2);
	if (tmp4 == 0) return 0;
	if (tmp4 == 2) movefile(tmp, hostfile);
	return 1;
}
/* }}} */
/* HOSTFILE: addtelnet() {{{ */
int addtelnet(char *par)
{ 
   //char tmp[255],tmp2[255],tmp3[255];
   //int tmp4;
   //FILE *file,*file2; 
	char host[512];
	if (!par[0])
		return 0;
	split(host, par);
	if (host != NULL)
		if (!host[0]) {
			strcpy(host, par);
			par[0] = 0;
		}

	sprintf(tmp3, "%s\n", host);
	tmp4 = 0;
    /* create a hostfile if it doesn't exist */
	struct stat buffer;
	if (stat (hostfile, &buffer) != 0) {
	  file = fopen(hostfile, "a");
	  fprintf(file, "\n");
	  fclose(file);
	  //nfree(file);
	  putlog(LOG_MISC, "*", "Created new hostfile..");
	}
	file = fopen(hostfile, "rt");
	sprintf(tmp, "%s~new", hostfile);
	file2 = fopen(tmp, "wt");

	//while (!feof(file)) {
		//fgets(tmp2, sizeof tmp2, file);
	while (fgets(tmp2, sizeof tmp2, file)!= NULL) {
		//if (feof(file)) { break; }
		if (strcasecmp(tmp2, tmp3) == 0) tmp4 = 1;
		fputs(tmp2, file2);
	}
	fclose(file);

	if ( tmp4 == 0 ) {
		fputs(tmp3, file2);
		fclose(file2);
		movefile(tmp, hostfile);
		return 1;
	}
	return 0;
}
/* }}} */

/*
 * vim: ft=c sw=4 ts=4 noet:
 */
