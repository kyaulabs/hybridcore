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
#include "hybridcore.h"

extern char hostfile[];
extern int protect_readonly;
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
    if(check_it < 58 && check_it >  47) other = 1;
    if(check_it > 64 && check_it <  91) ucase = 1;
    if(check_it > 96 && check_it < 123) lcase = 1;
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
    char *encstr = {0};
    char stuff[8192] = {0};
    FILE *ecfg, *dcfg;
    ecfg = fopen(cfgfile, "r");
    dcfg = fopen(".tmp1", "w");
    while (fgets(stuff, sizeof stuff, ecfg) != NULL) {
      size_t len = strlen (stuff);
      if (len && stuff [len - 1] == '\n')
        stuff[--len] = 0;
      encstr = encrypt_string(GOD, stuff);
      if (encstr == NULL)
        continue;
      int slen = strlen(encstr);
      encstr[slen] = 0;
      fprintf(dcfg, "%s\n", encstr);
    }
    fclose(ecfg);
    fclose(dcfg);
    chmod(".tmp1", HYBRID_MODE);
    return 1;
  } else {
    return 0;
  }
  return 0;
}
/* }}} */
/* SECURE: decrypt_file() {{{ */
int decrypt_file(char *cfgfile) {
  struct stat buffer;
  if (stat (cfgfile, &buffer) == 0) {
    char stuff[8192] = {0};
    char *decstr = {0};
    FILE *ecfg, *dcfg;
    ecfg = fopen(cfgfile, "r");
    dcfg = fopen(".tmp2", "w");
    while (fgets(stuff, sizeof stuff, ecfg) != NULL) {
      size_t len = strlen (stuff);
      if (len && stuff [len - 1] == '\n')
        stuff[--len] = 0;
      decstr = decrypt_string(GOD, stuff);
      if (decstr == NULL)
        continue;
      int slen = strlen(decstr);
      decstr[slen] = 0;
      fprintf(dcfg, "%s\n", decstr);
    }
    fclose(ecfg);
    fclose(dcfg);
    chmod(".tmp2", HYBRID_MODE);
    return 1;
  } else {
    return 0;
  }
  return 0;
}
/* }}} */
/* SECURE: secure_tcl_load() {{{ */
void secure_tcl_load() {
  /* automatic tcl encryption */
  struct stat buffer;
  if (stat ("decrypted.tcl", &buffer) == 0) {
    putlog(LOG_CMDS, "*", "\00309□\003 encrypting: \00314decrypted.tcl\003 => \00314%s\003", HYBRID_TCLSCRIPT);
    if (encrypt_file("decrypted.tcl"))
     movefile(".tmp1", HYBRID_TCLSCRIPT);
  }
  /* automatic tcl decryption and loading */
  if (stat (HYBRID_TCLSCRIPT, &buffer) == 0) {
    putlog(LOG_CMDS, "*", "\00309□\003 decrypting: \00314tcl script\003 \00306(%s)\003", HYBRID_TCLSCRIPT);
    if (decrypt_file(HYBRID_TCLSCRIPT)) {
      if (stat (".tmp2", &buffer) == 0) {
        if (!readtclprog(".tmp2"))
          putlog(LOG_MISC, "*", "\00304‼ ERROR:\003 can't load '%s'!", HYBRID_TCLSCRIPT);
        unlink(".tmp2");
      }
    }
  }
}
/* }}} */

/*
 * vim: ft=c sw=2 ts=2 et:
 */
