/*
 * msgcmds.c -- part of irc.mod
 *   all commands entered via /MSG
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2018 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

static int msg_hello(char *nick, char *h, struct userrec *u, char *p)
{
  char host[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN], handle[HANDLEN + 1];
  char *p1;
  int common = 0;
  int atr = 0;
  struct chanset_t *chan;

  if (!learn_users && !make_userfile)
    return 0;

  if (match_my_nick(nick))
    return 1;

  if (u) {
    atr = u->flags;
    if (!(atr & USER_COMMON)) {
      return 1;
    }
  }
  strlcpy(handle, nick, sizeof(handle));
  if (get_user_by_handle(userlist, handle)) {
    dprintf(DP_HELP, IRC_BADHOST1, nick);
    dprintf(DP_HELP, IRC_BADHOST2, nick, botname);
    return 1;
  }
  egg_snprintf(s, sizeof s, "%s!%s", nick, h);
  if (u_match_mask(global_bans, s)) {
    return 1;
  }
  if (atr & USER_COMMON) {
    maskhost(s, host);
    strcpy(s, host);
    egg_snprintf(host, sizeof host, "%s!%s", nick, s + 2);
    userlist = adduser(userlist, handle, host, "-", USER_DEFAULT);
    putlog(LOG_MISC, "*", "%s %s (%s) -- %s",
           IRC_INTRODUCED, nick, host, IRC_COMMONSITE);
    common = 1;
  } else {
    maskhost(s, host);
    if (make_userfile) {
      userlist = adduser(userlist, handle, host, "-",
                 sanity_check(default_flags | USER_MASTER | USER_OWNER));
      set_user(&USERENTRY_HOSTS, get_user_by_handle(userlist, handle),
               "-telnet!*@*");
    } else
      userlist = adduser(userlist, handle, host, "-",
                         sanity_check(default_flags));
    putlog(LOG_MISC, "*", "\00311=\003 !%s! \00314(%s)\003 \00310<SECURE +ADMIN>\003", nick, host);
  }
  for (chan = chanset; chan; chan = chan->next)
    if (ismember(chan, handle))
      add_chanrec_by_handle(userlist, handle, chan->dname);
  dprintf(DP_HELP, IRC_SALUT2, nick, host);
  if (common) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_SALUT2A);
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_SALUT2B);
  }
  if (make_userfile) {
    putlog(LOG_MISC, "*", "\00309□\003 hybrid(core): \00314install complete\003 \00306<%s>\003", handle);
    make_userfile = 0;
    write_userfile(-1);
  } else {
    dprintf(DP_HELP, IRC_INTRO1, nick, botname);
  }
  if (strlen(nick) > HANDLEN)
    /* Notify the user that his/her handle was truncated. */
    dprintf(DP_HELP, IRC_NICKTOOLONG, nick, handle);
  if (notify_new[0]) {
    egg_snprintf(s, sizeof s, IRC_INITINTRO, nick, host);
    strcpy(s1, notify_new);
    while (s1[0]) {
      p1 = strchr(s1, ',');
      if (p1 != NULL) {
        *p1 = 0;
        p1++;
        rmspace(p1);
      }
      rmspace(s1);
      if (p1 == NULL)
        s1[0] = 0;
      else
        strlcpy(s1, p1, sizeof s1);
    }
  }
  return 1;
}

int secpass(char *password) {
  int ucase, lcase, other, pl, i, check_it;
  pl = strlen(password);
 
  if (pl < HYBRID_PASSWDLEN) {
      return 0;
  }
  other = ucase = lcase = 0;
  for(i=0; i < pl; i++) {
    check_it = (int)password[i];
    if(check_it < 58 && check_it >  47)  other = 1;
    if(check_it > 64 && check_it <  91)  ucase = 1;
    if(check_it > 96 && check_it < 123)  lcase = 1;
  }
  if ((ucase+lcase+other)==3) {
    return 1;
  }
  return 0;
}

static int msg_pass(char *nick, char *host, struct userrec *u, char *par)
{
  char *old, *new;

  if (!u || match_my_nick(nick) || (u->flags & (USER_BOT | USER_COMMON)))
    return 1;

  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
            u_pass_match(u, "-") ? IRC_NOPASS : IRC_PASS);
    putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<?PASS?>\003", u->handle, nick, host);
    return 1;
  }
  old = newsplit(&par);
  if (!u_pass_match(u, "-") && !par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_EXISTPASS);
    return 1;
  }
  if (par[0]) {
    if (!u_pass_match(u, old)) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_FAILPASS);
      return 1;
    }
    new = newsplit(&par);
  } else
    new = old;
  putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE PASS>\003", u->handle, nick, host);
  if (strlen(new) > 15)
    new[15] = 0;
  /* secpass function */
  if (secpass(new) == 0) {
    dprintf(DP_HELP, "NOTICE %s :(%s) is not secure try again\n", nick, new);
    putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00304<INSECURE PASS>\003", u->handle, nick, host);
    return 1;
  }
  set_user(&USERENTRY_PASS, u, new);
  dprintf(DP_HELP, "NOTICE %s :%s \002(\002%s\002)\002\n", nick,
          "password =", new);
  return 1;
}

static int msg_addhost(char *nick, char *host, struct userrec *u, char *par)
{
  char s[UHOSTLEN], s1[UHOSTLEN], *pass, who[NICKLEN];
  struct userrec *u2;

  if (match_my_nick(nick) || (u && (u->flags & USER_BOT)))
    return 1;

  if (u && (u->flags & USER_COMMON)) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_FAILCOMMON);
    return 1;
  }
  pass = newsplit(&par);
  if (!par[0])
    strlcpy(who, nick, sizeof who);
  else
    strlcpy(who, par, sizeof who);
  u2 = get_user_by_handle(userlist, who);
  if (!u2) {
    if (u && !quiet_reject)
      dprintf(DP_HELP, IRC_MISIDENT, nick, nick, u->handle);
  } else if (rfc_casecmp(who, origbotname) && !(u2->flags & USER_BOT)) {
    /* This could be used as detection... */
    if (u_pass_match(u2, "-")) {
      putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00310<SECURE IDENT %s>\003", nick, host, who);
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOPASS);
    } else if (!u_pass_match(u2, pass)) {
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_DENYACCESS);
    } else if (u == u2) {
      /*
       * NOTE: Checking quiet_reject *after* u_pass_match()
       * verifies the password makes NO sense!
       * (Broken since 1.3.0+bel17)  Bad Beldin! No Cookie!
       *   -Toth  [July 30, 2003]
       */
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_RECOGNIZED);
      return 1;
    } else if (u) {
      dprintf(DP_HELP, IRC_MISIDENT, nick, who, u->handle);
      return 1;
    } else {
      putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00310<SECURE IDENT %s>\003", nick, host, who);
      egg_snprintf(s, sizeof s, "%s!%s", nick, host);
      maskhost(s, s1);
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, IRC_ADDHOSTMASK, s1);
      addhost_by_handle(who, s1);
      check_this_user(who, 0, NULL);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00304<FAILED IDENT %s>\003", nick, host, who);
  return 1;
}

static int msg_op(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
        chan = findchan_by_dname(par);
        if (chan && channel_active(chan)) {
          get_user_flagrec(u, &fr, par);
          if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))
            add_mode(chan, '+', 'o', nick);
          putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE OP %s>\003", u->handle, nick, host,
                 par);
          return 1;
        }
      } else {
        for (chan = chanset; chan; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))
            add_mode(chan, '+', 'o', nick);
        }
        putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE OP>\003", u->handle, nick, host);
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00304<FAILED OP>\003", nick, host);
  return 1;
}

static int msg_halfop(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
        chan = findchan_by_dname(par);
        if (chan && channel_active(chan)) {
          get_user_flagrec(u, &fr, par);
          if (chan_op(fr) || chan_halfop(fr) || (glob_op(fr) &&
              !chan_deop(fr)) || (glob_halfop(fr) && !chan_dehalfop(fr)))
            add_mode(chan, '+', 'h', nick);
          putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE HALFOP %s>\003",
                 u->handle, nick, host, par);
          return 1;
        }
      } else {
        for (chan = chanset; chan; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (chan_op(fr) || chan_halfop(fr) || (glob_op(fr) &&
              !chan_deop(fr)) || (glob_halfop(fr) && !chan_dehalfop(fr)))
            add_mode(chan, '+', 'h', nick);
        }
        putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE HALFOP>\003", u->handle, nick, host);
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00304<FAILED HALFOP>\003", nick, host);
  return 1;
}

/* Don't have to specify a channel now and can use this command
 * regardless of +autovoice or being a chanop. (guppy 7Jan1999)
 */
static int msg_voice(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
        chan = findchan_by_dname(par);
        if (chan && channel_active(chan)) {
          get_user_flagrec(u, &fr, par);
          if (chan_voice(fr) || glob_voice(fr) || chan_op(fr) || glob_op(fr)) {
            add_mode(chan, '+', 'v', nick);
            putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE VOICE %s>\003", u->handle, nick, host,
                   par);
          } else
            putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00304<FAILED VOICE %s>\003",
                   nick, host, par);
          return 1;
        }
      } else {
        for (chan = chanset; chan; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (chan_voice(fr) || glob_voice(fr) ||
              chan_op(fr) || glob_op(fr) || chan_halfop(fr) || glob_halfop(fr))
            add_mode(chan, '+', 'v', nick);
        }
        putlog(LOG_CMDS, "*", "\00311=\003 !%s! \00314(%s!%s)\003 \00310<SECURE VOICE>\003", u->handle, nick, host);
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "\00311=\003 !*! \00314(%s!%s)\003 \00304<FAILED VOICE>\003", nick, host);
  return 1;
}

/* MSG COMMANDS
 *
 * Function call should be:
 *    int msg_cmd("handle","nick","user@host","params");
 *
 * The function is responsible for any logging. Return 1 if successful,
 * 0 if not.
 */
static cmd_t C_msg[] = {
  {"+host",   "",    (IntFunc) msg_addhost, NULL},
  {"+moo",    "",    (IntFunc) msg_hello,   NULL},
  {"+op",     "",    (IntFunc) msg_op,      NULL},
  {"+halfop", "",    (IntFunc) msg_halfop,  NULL},
  {"+pass",   "",    (IntFunc) msg_pass,    NULL},
  {"+passwd", "",    (IntFunc) msg_pass,    NULL},
  {"+voice",  "",    (IntFunc) msg_voice,   NULL},
  {NULL,      NULL,  NULL,                   NULL}
};
