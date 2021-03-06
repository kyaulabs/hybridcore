/*
 * cmdschan.c -- part of channels.mod
 *   commands from a user via dcc that cause server interaction
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

#include <ctype.h>

static struct flag_record user = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
static struct flag_record victim = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };


static void cmd_pls_ban(struct userrec *u, int idx, char *par)
{
  char *chname, *who, s[UHOSTLEN], s1[UHOSTLEN], *p, *p_expire;
  long expire_foo;
  unsigned long expire_time = 0;
  int sticky = 0;
  struct chanset_t *chan = NULL;
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Usage: +ban <hostmask> [channel] [%%<XyXdXhXm>] [reason]\n");
  } else {
    who = newsplit(&par);
    /* Sanity check for <channel> <ban> vs. <ban> <channel> */
    if (par[0] && strchr(CHANMETA, who[0])) {
      chname = who;
      who = newsplit(&par);
      dprintf(idx, "Usage: +ban <hostmask> [channel] [%%<XyXdXhXm>] [reason]\n");
      dprintf(idx, "Did you mean: .+ban %s %s %s\n", who, chname, par);
      return;
    } else if (par[0] && strchr(CHANMETA, par[0]))
      chname = newsplit(&par);
    else
      chname = 0;
    if (chname || !(u->flags & USER_OP)) {
      if (!chname)
        chname = dcc[idx].u.chat->con_chan;
      get_user_flagrec(u, &user, chname);
      chan = findchan_by_dname(chname);
      /* *shrug* ??? (guppy:10Feb1999) */
      if (!chan) {
        dprintf(idx, "\00304That channel doesn't exist!\003\n");
        return;
      } else if (!((glob_op(user) && !chan_deop(user)) || (glob_halfop(user) &&
               !chan_dehalfop(user)) || chan_op(user) || chan_halfop(user))) {
        dprintf(idx, "\00304You don't have access to set bans on %s.\003\n", chname);
        return;
      }
    } else
      chan = 0;
    /* Added by Q and Solal -- Requested by Arty2, special thanx :) */
    if (par[0] == '%') {
      p = newsplit(&par);
      p_expire = p + 1;
      while (*(++p) != 0) {
        switch (tolower((unsigned) *p)) {
        case 'y':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * 24 * 365 * expire_foo;
          p_expire = p + 1;
          break;
        case 'd':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * 24 * expire_foo;
          p_expire = p + 1;
          break;
        case 'h':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * expire_foo;
          p_expire = p + 1;
          break;
        case 'm':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * expire_foo;
          p_expire = p + 1;
        }
      }
      if (expire_time > (60 * 60 * 24 * 365 * 5)) {
        dprintf(idx, "Ban expiration time cannot exceed 5 years "
            "(1825 days)\n");
        return;
      }
    }
    if (!par[0])
      par = "requested";
    else if (strlen(par) > MASKREASON_MAX)
      par[MASKREASON_MAX] = 0;
    if (strlen(who) > UHOSTMAX - 4)
      who[UHOSTMAX - 4] = 0;
    /* Fix missing ! or @ BEFORE checking against myself */
    if (!strchr(who, '!')) {
      if (!strchr(who, '@'))
        egg_snprintf(s, sizeof s, "%s!*@*", who);       /* Lame nick ban */
      else
        egg_snprintf(s, sizeof s, "*!%s", who);
    } else if (!strchr(who, '@'))
      egg_snprintf(s, sizeof s, "%s@*", who);   /* brain-dead? */
    else
      strlcpy(s, who, sizeof s);
    if ((me = module_find("server", 0, 0)) && me->funcs) {
      egg_snprintf(s1, sizeof s1, "%s!%s", me->funcs[SERVER_BOTNAME],
                   me->funcs[SERVER_BOTUSERHOST]);
      if (match_addr(s, s1)) {
        dprintf(idx, "\00304I'm not going to ban myself.\003\n");
        putlog(LOG_CMDS, "*", "\00307#attempted +ban#\003 \00306%s\003 %s", s, dcc[idx].nick);
        return;
      }
    }
      /* IRC can't understand bans longer than 70 characters */
      if (strlen(s) > 70) {
        s[69] = '*';
        s[70] = 0;
      }
      if (chan) {
        u_addban(chan, s, dcc[idx].nick, par,
                 expire_time ? now + expire_time : 0, 0);
        if (par[0] == '*') {
          sticky = 1;
          par++;
          putlog(LOG_CMDS, "*", "\00307#(%s) +ban#\003 \00306%s %s (%s) (sticky)\003 %s",
                 dcc[idx].u.chat->con_chan, s, chan->dname, par, dcc[idx].nick);
          dprintf(idx, "New %s sticky ban: %s (%s)\n", chan->dname, s, par);
        } else {
          putlog(LOG_CMDS, "*", "\00307#(%s) +ban#\003 \00306%s %s (%s)\003 %s",
                 dcc[idx].u.chat->con_chan, s, chan->dname, par, dcc[idx].nick);
          dprintf(idx, "New %s ban: %s (%s)\n", chan->dname, s, par);
        }
        /* Avoid unnesessary modes if you got +dynamicbans, and there is
         * no reason to set mode if irc.mod aint loaded. (dw 001120)
         */
        if ((me = module_find("irc", 0, 0)))
          (me->funcs[IRC_CHECK_THIS_BAN]) (chan, s, sticky);
      } else {
        u_addban(NULL, s, dcc[idx].nick, par,
                 expire_time ? now + expire_time : 0, 0);
        if (par[0] == '*') {
          sticky = 1;
          par++;
          putlog(LOG_CMDS, "*", "\00307#(GLOBAL) +ban#\003 \00306%s (%s) (sticky)\003 %s",
                 s, par, dcc[idx].nick);
          dprintf(idx, "New sticky ban: %s (%s)\n", s, par);
        } else {
          putlog(LOG_CMDS, "*", "\00307#(GLOBAL) +ban#\003 \00306%s (%s)\003 %s",
                 s, par, dcc[idx].nick);
          dprintf(idx, "New ban: %s (%s)\n", s, par);
        }
        if ((me = module_find("irc", 0, 0)))
          for (chan = chanset; chan != NULL; chan = chan->next)
            (me->funcs[IRC_CHECK_THIS_BAN]) (chan, s, sticky);
      }
  }
}

static void cmd_pls_exempt(struct userrec *u, int idx, char *par)
{
  char *chname, *who, s[UHOSTLEN], *p, *p_expire;
  long expire_foo;
  unsigned long expire_time = 0;
  struct chanset_t *chan = NULL;

  if (!use_exempts) {
    dprintf(idx, "\00304This command can only be used with use-exempts enabled.\003\n");
    return;
  }
  if (!par[0]) {
    dprintf(idx, "Usage: +exempt <hostmask> [channel] [%%<XydXhXm>] [reason]\n");
  } else {
    who = newsplit(&par);
    if (par[0] && strchr(CHANMETA, par[0]))
      chname = newsplit(&par);
    else
      chname = 0;
    if (chname || !(u->flags & USER_OP)) {
      if (!chname)
        chname = dcc[idx].u.chat->con_chan;
      get_user_flagrec(u, &user, chname);
      chan = findchan_by_dname(chname);
      /* *shrug* ??? (guppy:10Feb99) */
      if (!chan) {
        dprintf(idx, "\00304That channel doesn't exist!\003\n");
        return;
      } else if (!((glob_op(user) && !chan_deop(user)) || (glob_halfop(user) &&
               !chan_dehalfop(user)) || chan_op(user) || chan_halfop(user))) {
        dprintf(idx, "\00304You don't have access to set exempts on %s.\003\n", chname);
        return;
      }
    } else
      chan = 0;
    /* Added by Q and Solal  - Requested by Arty2, special thanx :) */
    if (par[0] == '%') {
      p = newsplit(&par);
      p_expire = p + 1;
      while (*(++p) != 0) {
        switch (tolower((unsigned) *p)) {
        case 'y':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * 24 * 365 * expire_foo;
          p_expire = p + 1;
          break;
        case 'd':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * 24 * expire_foo;
          p_expire = p + 1;
          break;
        case 'h':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * expire_foo;
          p_expire = p + 1;
          break;
        case 'm':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * expire_foo;
          p_expire = p + 1;
        }
      }
      if (expire_time > (60 * 60 * 24 * 365 * 5)) {
        dprintf(idx, "Exempt expiration time cannot exceed 5 years "
            "(1825 days)\n");
        return;
      }
    }
    if (!par[0])
      par = "requested";
    else if (strlen(par) > MASKREASON_MAX)
      par[MASKREASON_MAX] = 0;
    if (strlen(who) > UHOSTMAX - 4)
      who[UHOSTMAX - 4] = 0;
    /* Fix missing ! or @ BEFORE checking against myself */
    if (!strchr(who, '!')) {
      if (!strchr(who, '@'))
        egg_snprintf(s, sizeof s, "%s!*@*", who);       /* Lame nick exempt */
      else
        egg_snprintf(s, sizeof s, "*!%s", who);
    } else if (!strchr(who, '@'))
      egg_snprintf(s, sizeof s, "%s@*", who);   /* brain-dead? */
    else
      strlcpy(s, who, sizeof s);

    /* IRC can't understand exempts longer than 70 characters */
    if (strlen(s) > 70) {
      s[69] = '*';
      s[70] = 0;
    }
    if (chan) {
      u_addexempt(chan, s, dcc[idx].nick, par,
                  expire_time ? now + expire_time : 0, 0);
      if (par[0] == '*') {
        par++;
        putlog(LOG_CMDS, "*", "\00307#(%s) +exempt#\003 \00306%s %s (%s) (sticky)\003 %s",
               dcc[idx].u.chat->con_chan, s, chan->dname, par, dcc[idx].nick);
        dprintf(idx, "New %s sticky exempt: %s (%s)\n", chan->dname, s, par);
      } else {
        putlog(LOG_CMDS, "*", "\00307#(%s) +exempt#\003 \00306%s %s (%s)\003 %s",
               dcc[idx].u.chat->con_chan, s, chan->dname, par, dcc[idx].nick);
        dprintf(idx, "New %s exempt: %s (%s)\n", chan->dname, s, par);
      }
      add_mode(chan, '+', 'e', s);
    } else {
      u_addexempt(NULL, s, dcc[idx].nick, par,
                  expire_time ? now + expire_time : 0, 0);
      if (par[0] == '*') {
        par++;
        putlog(LOG_CMDS, "*", "\00307#(GLOBAL) +exempt#\003 \00306%s (%s) (sticky)\003 %s",
               s, par, dcc[idx].nick);
        dprintf(idx, "New sticky exempt: %s (%s)\n", s, par);
      } else {
        putlog(LOG_CMDS, "*", "\00307#(GLOBAL) +exempt#\003 \00306%s (%s)\003 %s",
               s, par, dcc[idx].nick);
        dprintf(idx, "New exempt: %s (%s)\n", s, par);
      }
      for (chan = chanset; chan != NULL; chan = chan->next)
        add_mode(chan, '+', 'e', s);
    }
  }
}

static void cmd_pls_invite(struct userrec *u, int idx, char *par)
{
  char *chname, *who, s[UHOSTLEN], *p, *p_expire;
  long expire_foo;
  unsigned long expire_time = 0;
  struct chanset_t *chan = NULL;

  if (!use_invites) {
    dprintf(idx, "\00304This command can only be used with use-invites enabled.\003\n");
    return;
  }

  if (!par[0]) {
    dprintf(idx, "Usage: +invite <hostmask> [channel] [%%<XyXdXhXm>] [reason]\n");
  } else {
    who = newsplit(&par);
    if (par[0] && strchr(CHANMETA, par[0]))
      chname = newsplit(&par);
    else
      chname = 0;
    if (chname || !(u->flags & USER_OP)) {
      if (!chname)
        chname = dcc[idx].u.chat->con_chan;
      get_user_flagrec(u, &user, chname);
      chan = findchan_by_dname(chname);
      /* *shrug* ??? (guppy:10Feb99) */
      if (!chan) {
        dprintf(idx, "\00304That channel doesn't exist!\003\n");
        return;
      } else if (!((glob_op(user) && !chan_deop(user)) || (glob_halfop(user) &&
               !chan_dehalfop(user)) || chan_op(user) || chan_halfop(user))) {
        dprintf(idx, "\00304You don't have access to set invites on %s.\003\n", chname);
        return;
      }
    } else
      chan = 0;
    /* Added by Q and Solal  - Requested by Arty2, special thanx :) */
    if (par[0] == '%') {
      p = newsplit(&par);
      p_expire = p + 1;
      while (*(++p) != 0) {
        switch (tolower((unsigned) *p)) {
        case 'y':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * 24 * 365 * expire_foo;
          p_expire = p + 1;
          break;
        case 'd':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * 24 * expire_foo;
          p_expire = p + 1;
          break;
        case 'h':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * 60 * expire_foo;
          p_expire = p + 1;
          break;
        case 'm':
          *p = 0;
          expire_foo = strtol(p_expire, NULL, 10);
          expire_time += 60 * expire_foo;
          p_expire = p + 1;
        }
      }
      if (expire_time > (60 * 60 * 24 * 365 * 5)) {
        dprintf(idx, "Invite expiration time cannot exceed 5 years "
            "(1825 days)\n");
        return;
      }
    }
    if (!par[0])
      par = "requested";
    else if (strlen(par) > MASKREASON_MAX)
      par[MASKREASON_MAX] = 0;
    if (strlen(who) > UHOSTMAX - 4)
      who[UHOSTMAX - 4] = 0;
    /* Fix missing ! or @ BEFORE checking against myself */
    if (!strchr(who, '!')) {
      if (!strchr(who, '@'))
        egg_snprintf(s, sizeof s, "%s!*@*", who);       /* Lame nick invite */
      else
        egg_snprintf(s, sizeof s, "*!%s", who);
    } else if (!strchr(who, '@'))
      egg_snprintf(s, sizeof s, "%s@*", who);   /* brain-dead? */
    else
      strlcpy(s, who, sizeof s);

    /* IRC can't understand invites longer than 70 characters */
    if (strlen(s) > 70) {
      s[69] = '*';
      s[70] = 0;
    }
    if (chan) {
      u_addinvite(chan, s, dcc[idx].nick, par,
                  expire_time ? now + expire_time : 0, 0);
      if (par[0] == '*') {
        par++;
        putlog(LOG_CMDS, "*", "\00307#(%s) +invite#\003 \00306%s %s (%s) (sticky)\003 %s",
               dcc[idx].u.chat->con_chan, s, chan->dname, par, dcc[idx].nick);
        dprintf(idx, "New %s sticky invite: %s (%s)\n", chan->dname, s, par);
      } else {
        putlog(LOG_CMDS, "*", "\00307#(%s) +invite#\003 \00306%s %s (%s)\003 %s",
               dcc[idx].u.chat->con_chan, s, chan->dname, par, dcc[idx].nick);
        dprintf(idx, "New %s invite: %s (%s)\n", chan->dname, s, par);
      }
      add_mode(chan, '+', 'I', s);
    } else {
      u_addinvite(NULL, s, dcc[idx].nick, par,
                  expire_time ? now + expire_time : 0, 0);
      if (par[0] == '*') {
        par++;
        putlog(LOG_CMDS, "*", "\00307#(GLOBAL) +invite#\003 \00306%s (%s) (sticky)\003 %s",
               s, par, dcc[idx].nick);
        dprintf(idx, "New sticky invite: %s (%s)\n", s, par);
      } else {
        putlog(LOG_CMDS, "*", "\00307#(GLOBAL) +invite#\003 \00306%s (%s)\003 %s",
               s, par, dcc[idx].nick);
        dprintf(idx, "New invite: %s (%s)\n", s, par);
      }
      for (chan = chanset; chan != NULL; chan = chan->next)
        add_mode(chan, '+', 'I', s);
    }
  }
}

static void cmd_mns_ban(struct userrec *u, int idx, char *par)
{
  int console = 0, i = 0, j;
  struct chanset_t *chan = NULL;
  char s[UHOSTLEN], *ban, *chname, *mask;
  masklist *b;

  if (!par[0]) {
    dprintf(idx, "Usage: -ban <hostmask|ban #> [channel]\n");
    return;
  }
  ban = newsplit(&par);
  /* Sanity check for <channel> <ban> vs. <ban> <channel> */
  if (par[0] && strchr(CHANMETA, ban[0])) {
      chname = ban;
      ban = newsplit(&par);
      dprintf(idx, "Usage: -ban <hostmask|ban #> [channel]\n");
      dprintf(idx, "Did you mean: .-ban %s %s\n", ban, chname);
      return;
  } else if (par[0] && strchr(CHANMETA, par[0]))
    chname = newsplit(&par);
  else {
    chname = dcc[idx].u.chat->con_chan;
    console = 1;
  }
  if (chname || !(u->flags & USER_OP)) {
    if (!chname)
      chname = dcc[idx].u.chat->con_chan;
    get_user_flagrec(u, &user, chname);

    if ((!chan_op(user) && (!glob_op(user) || chan_deop(user))) &&
        (!chan_halfop(user) && (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "\00304You don't have access to remove bans on %s.\003\n", chname);
      return;
    }
  }
  strlcpy(s, ban, sizeof s);
  if (console) {
    i = u_delban(NULL, s, (u->flags & USER_OP));
    if (i > 0) {
      if (lastdeletedmask)
        mask = lastdeletedmask;
      else
        mask = s;
      putlog(LOG_CMDS, "*", "\00307#-ban#\003 \00306%s\003 %s", mask, dcc[idx].nick);
      dprintf(idx, "%s: %s\n", IRC_REMOVEDBAN, mask);
      for (chan = chanset; chan != NULL; chan = chan->next)
        add_mode(chan, '-', 'b', mask);
      return;
    }
  }
  /* Channel-specific ban? */
  if (chname)
    chan = findchan_by_dname(chname);
  if (!chan) {
    dprintf(idx, "\00304Invalid channel.\003\n");
    return;
  }
  if (str_isdigit(ban)) {
    i = atoi(ban);
    /* subtract the number of global bans to get the number of the channel ban */
    egg_snprintf(s, sizeof s, "%d", i);
    j = u_delban(0, s, 0);
    if (j < 0) {
      egg_snprintf(s, sizeof s, "%d", -j);
      j = u_delban(chan, s, 1);
      if (j > 0) {
        if (lastdeletedmask)
          mask = lastdeletedmask;
        else
          mask = s;
        putlog(LOG_CMDS, "*", "\00307#(%s) -ban#\003 \00306%s\003 %s", chan->dname,
               mask, dcc[idx].nick);
        dprintf(idx, "Removed %s channel ban: %s\n", chan->dname, mask);
        add_mode(chan, '-', 'b', mask);
        return;
      }
    }
    i = 0;
    for (b = chan->channel.ban; b && b->mask && b->mask[0]; b = b->next) {
      if ((!u_equals_mask(global_bans, b->mask)) &&
          (!u_equals_mask(chan->bans, b->mask))) {
        i++;
        if (i == -j) {
          add_mode(chan, '-', 'b', b->mask);
          dprintf(idx, "%s '%s' on %s.\n", IRC_REMOVEDBAN,
                  b->mask, chan->dname);
          putlog(LOG_CMDS, "*", "\00307#(%s) -ban#\003 \00306%s [on channel]\003 %s",
                 dcc[idx].u.chat->con_chan, ban, dcc[idx].nick);
          return;
        }
      }
    }
  } else {
    j = u_delban(chan, ban, 1);
    if (j > 0) {
      putlog(LOG_CMDS, "*", "\00307#(%s) -ban#\003 \00306%s\003 %s",
             dcc[idx].u.chat->con_chan, ban, dcc[idx].nick);
      dprintf(idx, "Removed %s channel ban: %s\n", chname, ban);
      add_mode(chan, '-', 'b', ban);
      return;
    }
    for (b = chan->channel.ban; b && b->mask && b->mask[0]; b = b->next) {
      if (!rfc_casecmp(b->mask, ban)) {
        add_mode(chan, '-', 'b', b->mask);
        dprintf(idx, "%s '%s' on %s.\n", IRC_REMOVEDBAN, b->mask, chan->dname);
        putlog(LOG_CMDS, "*", "\00307#(%s) -ban#\003 \00306%s [on channel]\003 %s",
               dcc[idx].u.chat->con_chan, ban, dcc[idx].nick);
        return;
      }
    }
  }
  dprintf(idx, "\00304No such ban.\003\n");
}

static void cmd_mns_exempt(struct userrec *u, int idx, char *par)
{
  int console = 0, i = 0, j;
  struct chanset_t *chan = NULL;
  char s[UHOSTLEN], *exempt, *chname, *mask;
  masklist *e;

  if (!use_exempts) {
    dprintf(idx, "\00304This command can only be used with use-exempts enabled.\003\n");
    return;
  }
  if (!par[0]) {
    dprintf(idx, "Usage: -exempt <hostmask|exempt #> [channel]\n");
    return;
  }
  exempt = newsplit(&par);
  if (par[0] && strchr(CHANMETA, par[0]))
    chname = newsplit(&par);
  else {
    chname = dcc[idx].u.chat->con_chan;
    console = 1;
  }
  if (chname || !(u->flags & USER_OP)) {
    if (!chname)
      chname = dcc[idx].u.chat->con_chan;
    get_user_flagrec(u, &user, chname);
    if ((!chan_op(user) && (!glob_op(user) || chan_deop(user))) &&
        (!chan_halfop(user) && (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "\00304You don't have access to remove exempts on %s.\003\n", chname);
      return;
    }
  }
  strlcpy(s, exempt, sizeof s);
  if (console) {
    i = u_delexempt(NULL, s, (u->flags & USER_OP));
    if (i > 0) {
      if (lastdeletedmask)
        mask = lastdeletedmask;
      else
        mask = s;
      putlog(LOG_CMDS, "*", "\00307#-exempt#\003 \00306%s\003 %s", mask, dcc[idx].nick);
      dprintf(idx, "%s: %s\n", IRC_REMOVEDEXEMPT, mask);
      for (chan = chanset; chan != NULL; chan = chan->next)
        add_mode(chan, '-', 'e', mask);
      return;
    }
  }
  /* Channel-specific exempt? */
  if (chname)
    chan = findchan_by_dname(chname);
  if (!chan) {
    dprintf(idx, "\00304Invalid channel.\003\n");
    return;
  }
  if (str_isdigit(exempt)) {
    i = atoi(exempt);
    /* subtract the number of global exempts to get the number of the channel exempt */
    egg_snprintf(s, sizeof s, "%d", i);
    j = u_delexempt(0, s, 0);
    if (j < 0) {
      egg_snprintf(s, sizeof s, "%d", -j);
      j = u_delexempt(chan, s, 1);
      if (j > 0) {
        if (lastdeletedmask)
          mask = lastdeletedmask;
        else
          mask = s;
        putlog(LOG_CMDS, "*", "\00307#(%s) -exempt#\003 \00306%s\003 %s",
               chan->dname, mask, dcc[idx].nick);
        dprintf(idx, "Removed %s channel exempt: %s\n", chan->dname, mask);
        add_mode(chan, '-', 'e', mask);
        return;
      }
    }
    i = 0;
    for (e = chan->channel.exempt; e && e->mask && e->mask[0]; e = e->next) {
      if (!u_equals_mask(global_exempts, e->mask) &&
          !u_equals_mask(chan->exempts, e->mask)) {
        i++;
        if (i == -j) {
          add_mode(chan, '-', 'e', e->mask);
          dprintf(idx, "%s '%s' on %s.\n", IRC_REMOVEDEXEMPT,
                  e->mask, chan->dname);
          putlog(LOG_CMDS, "*", "\00307#(%s) -exempt#\003 \00306%s [on channel]\003 %s",
                 dcc[idx].u.chat->con_chan, exempt, dcc[idx].nick);
          return;
        }
      }
    }
  } else {
    j = u_delexempt(chan, exempt, 1);
    if (j > 0) {
      putlog(LOG_CMDS, "*", "\00307#(%s) -exempt#\003 \00306%s\003 %s",
             dcc[idx].u.chat->con_chan, exempt, dcc[idx].nick);
      dprintf(idx, "Removed %s channel exempt: %s\n", chname, exempt);
      add_mode(chan, '-', 'e', exempt);
      return;
    }
    for (e = chan->channel.exempt; e && e->mask && e->mask[0]; e = e->next) {
      if (!rfc_casecmp(e->mask, exempt)) {
        add_mode(chan, '-', 'e', e->mask);
        dprintf(idx, "%s '%s' on %s.\n",
                IRC_REMOVEDEXEMPT, e->mask, chan->dname);
        putlog(LOG_CMDS, "*", "\00307#(%s) -exempt#\003 \00306%s [on channel]\003 %s",
               dcc[idx].u.chat->con_chan, exempt, dcc[idx].nick);
        return;
      }
    }
  }
  dprintf(idx, "\00304No such exemption.\003\n");
}

static void cmd_mns_invite(struct userrec *u, int idx, char *par)
{
  int console = 0, i = 0, j;
  struct chanset_t *chan = NULL;
  char s[UHOSTLEN], *invite, *chname, *mask;
  masklist *inv;

  if (!use_invites) {
    dprintf(idx, "\00304This command can only be used with use-invites enabled.\003\n");
    return;
  }
  if (!par[0]) {
    dprintf(idx, "Usage: -invite <hostmask|invite #> [channel]\n");
    return;
  }
  invite = newsplit(&par);
  if (par[0] && strchr(CHANMETA, par[0]))
    chname = newsplit(&par);
  else {
    chname = dcc[idx].u.chat->con_chan;
    console = 1;
  }
  if (chname || !(u->flags & USER_OP)) {
    if (!chname)
      chname = dcc[idx].u.chat->con_chan;
    get_user_flagrec(u, &user, chname);
    if ((!chan_op(user) && (!glob_op(user) || chan_deop(user))) &&
        (!chan_halfop(user) && (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "\00304You don't have access to remove invites on %s.\003\n", chname);
      return;
    }
  }
  strlcpy(s, invite, sizeof s);
  if (console) {
    i = u_delinvite(NULL, s, (u->flags & USER_OP));
    if (i > 0) {
      if (lastdeletedmask)
        mask = lastdeletedmask;
      else
        mask = s;
      putlog(LOG_CMDS, "*", "\00307#-invite#\003 \00306%s\003 %s", mask, dcc[idx].nick);
      dprintf(idx, "%s: %s\n", IRC_REMOVEDINVITE, mask);
      for (chan = chanset; chan != NULL; chan = chan->next)
        add_mode(chan, '-', 'I', mask);
      return;
    }
  }
  /* Channel-specific invite? */
  if (chname)
    chan = findchan_by_dname(chname);
  if (!chan) {
    dprintf(idx, "\00304Invalid channel.\003\n");
    return;
  }
  if (str_isdigit(invite)) {
    i = atoi(invite);
    /* subtract the number of global invites to get the number of the channel invite */
    egg_snprintf(s, sizeof s, "%d", i);
    j = u_delinvite(0, s, 0);
    if (j < 0) {
      egg_snprintf(s, sizeof s, "%d", -j);
      j = u_delinvite(chan, s, 1);
      if (j > 0) {
        if (lastdeletedmask)
          mask = lastdeletedmask;
        else
          mask = s;
        putlog(LOG_CMDS, "*", "\00307#(%s) -invite#\003 \00306%s\003 %s",
               chan->dname, mask, dcc[idx].nick);
        dprintf(idx, "Removed %s channel invite: %s\n", chan->dname, mask);
        add_mode(chan, '-', 'I', mask);
        return;
      }
    }
    i = 0;
    for (inv = chan->channel.invite; inv && inv->mask && inv->mask[0];
         inv = inv->next) {
      if (!u_equals_mask(global_invites, inv->mask) &&
          !u_equals_mask(chan->invites, inv->mask)) {
        i++;
        if (i == -j) {
          add_mode(chan, '-', 'I', inv->mask);
          dprintf(idx, "%s '%s' on %s.\n", IRC_REMOVEDINVITE,
                  inv->mask, chan->dname);
          putlog(LOG_CMDS, "*", "\00307#(%s) -invite#\003 \00306%s [on channel]\003 %s",
                 dcc[idx].u.chat->con_chan, invite, dcc[idx].nick);
          return;
        }
      }
    }
  } else {
    j = u_delinvite(chan, invite, 1);
    if (j > 0) {
      putlog(LOG_CMDS, "*", "\00307#(%s) -invite#\003 \00306%s\003 %s",
             dcc[idx].u.chat->con_chan, invite, dcc[idx].nick);
      dprintf(idx, "Removed %s channel invite: %s\n", chname, invite);
      add_mode(chan, '-', 'I', invite);
      return;
    }
    for (inv = chan->channel.invite; inv && inv->mask && inv->mask[0];
         inv = inv->next) {
      if (!rfc_casecmp(inv->mask, invite)) {
        add_mode(chan, '-', 'I', inv->mask);
        dprintf(idx, "%s '%s' on %s.\n",
                IRC_REMOVEDINVITE, inv->mask, chan->dname);
        putlog(LOG_CMDS, "*", "\00307#(%s) -invite#\003 \00306%s [on channel]\003 %s",
               dcc[idx].u.chat->con_chan, invite, dcc[idx].nick);
        return;
      }
    }
  }
  dprintf(idx, "\00304No such invite.\003\n");
}

static void cmd_bans(struct userrec *u, int idx, char *par)
{
  if (!egg_strcasecmp(par, "all")) {
    putlog(LOG_CMDS, "*", "\00307#bans#\003 \00306all\003 %s", dcc[idx].nick);
    tell_bans(idx, 1, "");
  } else {
    putlog(LOG_CMDS, "*", "\00307#bans#\003 \00306%s\003 %s", par, dcc[idx].nick);
    tell_bans(idx, 0, par);
  }
}

static void cmd_exempts(struct userrec *u, int idx, char *par)
{
  if (!use_exempts) {
    dprintf(idx, "\00304This command can only be used with use-exempts enabled.\003\n");
    return;
  }
  if (!egg_strcasecmp(par, "all")) {
    putlog(LOG_CMDS, "*", "\00307#exempts#\003 \00306all\003 %s", dcc[idx].nick);
    tell_exempts(idx, 1, "");
  } else {
    putlog(LOG_CMDS, "*", "\00307#exempts#\003 \00306%s\003 %s", par, dcc[idx].nick);
    tell_exempts(idx, 0, par);
  }
}

static void cmd_invites(struct userrec *u, int idx, char *par)
{
  if (!use_invites) {
    dprintf(idx, "\00304This command can only be used with use-invites enabled.\003\n");
    return;
  }
  if (!egg_strcasecmp(par, "all")) {
    putlog(LOG_CMDS, "*", "\00307#invites#\003 \00306all\003 %s", dcc[idx].nick);
    tell_invites(idx, 1, "");
  } else {
    putlog(LOG_CMDS, "*", "\00307#invites#\003 \00306%s\003 %s", par, dcc[idx].nick);
    tell_invites(idx, 0, par);
  }
}

static void cmd_info(struct userrec *u, int idx, char *par)
{
  char s[512], *chname, *s1;
  int locked = 0;

  if (!use_info) {
    dprintf(idx, "\00304Info storage is turned off.\003\n");
    return;
  }
  s1 = get_user(&USERENTRY_INFO, u);
  if (s1 && s1[0] == '@')
    locked = 1;
  if (par[0] && strchr(CHANMETA, par[0])) {
    chname = newsplit(&par);
    if (!findchan_by_dname(chname)) {
      dprintf(idx, "\00304No such channel.\003\n");
      return;
    }
    get_handle_chaninfo(dcc[idx].nick, chname, s);
    if (s[0] == '@')
      locked = 1;
    s1 = s;
  } else
    chname = 0;
  if (!par[0]) {
    if (s1 && s1[0] == '@')
      s1++;
    if (s1 && s1[0]) {
      if (chname) {
        dprintf(idx, "Info on %s: %s\n", chname, s1);
        dprintf(idx, "Use '.info %s none' to remove it.\n", chname);
      } else {
        dprintf(idx, "Default info: %s\n", s1);
        dprintf(idx, "Use '.info none' to remove it.\n");
      }
    } else
      dprintf(idx, "No info has been set for you.\n");
    putlog(LOG_CMDS, "*", "\00307#info#\003 \00306%s\003 %s", chname ? chname : "", dcc[idx].nick);
    return;
  }
  if (locked && !(u && (u->flags & USER_MASTER))) {
    dprintf(idx, "\00304Your info line is locked.  Sorry.\003\n");
    return;
  }
  if (!egg_strcasecmp(par, "none")) {
    if (chname) {
      par[0] = 0;
      set_handle_chaninfo(userlist, dcc[idx].nick, chname, NULL);
      dprintf(idx, "Removed your info line on %s.\n", chname);
      putlog(LOG_CMDS, "*", "\00307#info#\003 \00306%s none\003 %s", chname, dcc[idx].nick);
    } else {
      set_user(&USERENTRY_INFO, u, NULL);
      dprintf(idx, "Removed your default info line.\n");
      putlog(LOG_CMDS, "*", "\00307#info#\003 \00306none\003 %s", dcc[idx].nick);
    }
    return;
  }
/*  if (par[0] == '@')    This is stupid, and prevents a users info from being locked */
/*    par++;              without .tcl, or a tcl script, aka, 'half-assed' -poptix 4Jun01 */
  if (chname) {
    set_handle_chaninfo(userlist, dcc[idx].nick, chname, par);
    dprintf(idx, "Your info on %s is now: %s\n", chname, par);
    putlog(LOG_CMDS, "*", "\00307#info#\003 \00306%s\003 %s", chname, dcc[idx].nick);
  } else {
    set_user(&USERENTRY_INFO, u, par);
    dprintf(idx, "Your default info is now: %s\n", par);
    putlog(LOG_CMDS, "*", "\00307#info#\003 %s", dcc[idx].nick);
  }
}

static void cmd_chinfo(struct userrec *u, int idx, char *par)
{
  char *handle, *chname;
  struct userrec *u1;

  if (!use_info) {
    dprintf(idx, "\00304Info storage is turned off.\003\n");
    return;
  }
  handle = newsplit(&par);
  if (!handle[0]) {
    dprintf(idx, "Usage: chinfo <handle> [channel] <new-info>\n");
    return;
  }
  u1 = get_user_by_handle(userlist, handle);
  if (!u1) {
    dprintf(idx, "\00304No such user.\003\n");
    return;
  }
  if (par[0] && strchr(CHANMETA, par[0])) {
    chname = newsplit(&par);
    if (!findchan_by_dname(chname)) {
      dprintf(idx, "\00304No such channel.\003\n");
      return;
    }
  } else
    chname = 0;
  if ((u1->flags & USER_BOT) && !(u->flags & USER_MASTER)) {
    dprintf(idx, "\00304You have to be master to change bots info.\003\n");
    return;
  }
  if ((u1->flags & USER_OWNER) && !(u->flags & USER_OWNER)) {
    dprintf(idx, "\00304You can't change info for the bot owner.\003\n");
    return;
  }
  if (chname) {
    get_user_flagrec(u, &user, chname);
    get_user_flagrec(u1, &victim, chname);
    if ((chan_owner(victim) || glob_owner(victim)) &&
        !(glob_owner(user) || chan_owner(user))) {
      dprintf(idx, "\00304You can't change info for the channel owner.\003\n");
      return;
    }
  }
  putlog(LOG_CMDS, "*", "\00307#chinfo#\003 \00306%s %s %s\003 %s", handle,
         chname ? chname : par, chname ? par : "", dcc[idx].nick);
  if (!egg_strcasecmp(par, "none"))
    par[0] = 0;
  if (chname) {
    set_handle_chaninfo(userlist, handle, chname, par);
    if (par[0] == '@')
      dprintf(idx, "New info (LOCKED) for %s on %s: %s\n", handle, chname,
              &par[1]);
    else if (par[0])
      dprintf(idx, "New info for %s on %s: %s\n", handle, chname, par);
    else
      dprintf(idx, "Wiped info for %s on %s\n", handle, chname);
  } else {
    set_user(&USERENTRY_INFO, u1, par[0] ? par : NULL);
    if (par[0] == '@')
      dprintf(idx, "New default info (LOCKED) for %s: %s\n", handle, &par[1]);
    else if (par[0])
      dprintf(idx, "New default info for %s: %s\n", handle, par);
    else
      dprintf(idx, "Wiped default info for %s\n", handle);
  }
}

static void cmd_stick_yn(int idx, char *par, int yn)
{
  int i = 0, j;
  struct chanset_t *chan, *achan;
  char *stick_type, s[UHOSTLEN], chname[CHANNELLEN + 1];
  module_entry *me;

  stick_type = newsplit(&par);
  strlcpy(s, newsplit(&par), sizeof s);
  strlcpy(chname, newsplit(&par), sizeof chname);

  if (egg_strcasecmp(stick_type, "exempt") &&
      egg_strcasecmp(stick_type, "invite") &&
      egg_strcasecmp(stick_type, "ban")) {
    strlcpy(chname, s, sizeof chname);
    strlcpy(s, stick_type, sizeof s);
  }
  if (!s[0]) {
    dprintf(idx, "Usage: %sstick [ban/exempt/invite] <hostmask or number> "
            "[channel]\n", yn ? "" : "un");
    return;
  }
  /* Now deal with exemptions */
  if (!egg_strcasecmp(stick_type, "exempt")) {
    if (!use_exempts) {
      dprintf(idx, "\00304This command can only be used with use-exempts "
              "enabled.\003\n");
      return;
    }
    if (!chname[0]) {
      i = u_setsticky_exempt(NULL, s,
                             (dcc[idx].user->flags & USER_OP) ? yn : -1);
      if (i > 0) {
        putlog(LOG_CMDS, "*", "\00307#%sstick exempt#\003 \00306%s\003 %s",
               yn ? "" : "un", s, dcc[idx].nick);
        dprintf(idx, "%stuck exempt: %s\n", yn ? "S" : "Uns", s);
        return;
      }
      strlcpy(chname, dcc[idx].u.chat->con_chan, sizeof chname);
    }
    /* Channel-specific exempt? */
    if (!(chan = findchan_by_dname(chname))) {
      dprintf(idx, "\00304No such channel.\003\n");
      return;
    }
    if (str_isdigit(s)) {
      /* subtract the number of global exempts to get the number of the channel exempt */
      j = u_setsticky_exempt(NULL, s, -1);
      if (j < 0)
        egg_snprintf(s, sizeof s, "%d", -j);
    }
    j = u_setsticky_exempt(chan, s, yn);
    if (j > 0) {
      putlog(LOG_CMDS, "*", "\00307#%sstick exempt#\003 \00306%s %s\003 %s",
             yn ? "" : "un", s, chname, dcc[idx].nick);
      dprintf(idx, "%stuck %s exempt: %s\n", yn ? "S" : "Uns", chname, s);
      return;
    }
    dprintf(idx, "\00304No such exempt.\003\n");
    return;
  }
  /* Now the invites */
  else if (!egg_strcasecmp(stick_type, "invite")) {
    if (!use_invites) {
      dprintf(idx, "\00304This command can only be used with use-invites enabled.\003\n");
      return;
    }
    if (!chname[0]) {
      i = u_setsticky_invite(NULL, s,
                             (dcc[idx].user->flags & USER_OP) ? yn : -1);
      if (i > 0) {
        putlog(LOG_CMDS, "*", "\00307#%sstick invite#\003 \00306%s\003 %s",
               yn ? "" : "un", s, dcc[idx].nick);
        dprintf(idx, "%stuck invite: %s\n", yn ? "S" : "Uns", s);
        return;
      }
      strlcpy(chname, dcc[idx].u.chat->con_chan, sizeof chname);
    }
    /* Channel-specific invite? */
    if (!(chan = findchan_by_dname(chname))) {
      dprintf(idx, "\00304No such channel.\003\n");
      return;
    }
    if (str_isdigit(s)) {
      /* subtract the number of global invites to get the number of the channel invite */
      j = u_setsticky_invite(NULL, s, -1);
      if (j < 0)
        egg_snprintf(s, sizeof s, "%d", -j);
    }
    j = u_setsticky_invite(chan, s, yn);
    if (j > 0) {
      putlog(LOG_CMDS, "*", "\00307#%sstick invite#\003 \00306%s %s\003 %s",
             yn ? "" : "un", s, chname, dcc[idx].nick);
      dprintf(idx, "%stuck %s invite: %s\n", yn ? "S" : "Uns", chname, s);
      return;
    }
    dprintf(idx, "\00304No such invite.\003\n");
    return;
  }
  if (!chname[0]) {
    i = u_setsticky_ban(NULL, s,
                        (dcc[idx].user->flags & USER_OP) ? yn : -1);
    if (i > 0) {
      putlog(LOG_CMDS, "*", "\00307#%sstick ban#\003 \00306%s\003 %s",
             yn ? "" : "un", s, dcc[idx].nick);
      dprintf(idx, "%stuck ban: %s\n", yn ? "S" : "Uns", s);
      if ((me = module_find("irc", 0, 0)))
        for (achan = chanset; achan != NULL; achan = achan->next)
          (me->funcs[IRC_CHECK_THIS_BAN]) (achan, s, yn);
      return;
    }
    strlcpy(chname, dcc[idx].u.chat->con_chan, sizeof chname);
  }
  /* Channel-specific ban? */
  if (!(chan = findchan_by_dname(chname))) {
    dprintf(idx, "\00304No such channel.\003\n");
    return;
  }
  if (str_isdigit(s)) {
    /* subtract the number of global bans to get the number of the channel ban */
    j = u_setsticky_ban(NULL, s, -1);
    if (j < 0)
      egg_snprintf(s, sizeof s, "%d", -j);
  }
  j = u_setsticky_ban(chan, s, yn);
  if (j > 0) {
    putlog(LOG_CMDS, "*", "\00307#%sstick ban#\003 \00306%s %s\003 %s",
           yn ? "" : "un", s, chname, dcc[idx].nick);
    dprintf(idx, "%stuck %s ban: %s\n", yn ? "S" : "Uns", chname, s);
    if ((me = module_find("irc", 0, 0)))
      (me->funcs[IRC_CHECK_THIS_BAN]) (chan, s, yn);
    return;
  }
  dprintf(idx, "\00304No such ban.\003\n");
}


static void cmd_stick(struct userrec *u, int idx, char *par)
{
  cmd_stick_yn(idx, par, 1);
}

static void cmd_unstick(struct userrec *u, int idx, char *par)
{
  cmd_stick_yn(idx, par, 0);
}

static void cmd_pls_chrec(struct userrec *u, int idx, char *par)
{
  char *nick, *chn;
  struct chanset_t *chan;
  struct userrec *u1;
  struct chanuserrec *chanrec;

  if (!par[0]) {
    dprintf(idx, "Usage: +chrec <user> [channel]\n");
    return;
  }
  nick = newsplit(&par);
  u1 = get_user_by_handle(userlist, nick);
  if (!u1) {
    dprintf(idx, "\00304No such user.\003\n");
    return;
  }
  if (!par[0])
    chan = findchan_by_dname(dcc[idx].u.chat->con_chan);
  else {
    chn = newsplit(&par);
    chan = findchan_by_dname(chn);
  }
  if (!chan) {
    dprintf(idx, "\00304No such channel.\003\n");
    return;
  }
  get_user_flagrec(u, &user, chan->dname);
  get_user_flagrec(u1, &victim, chan->dname);
  if ((!glob_master(user) && !chan_master(user)) ||     /* drummer */
      (chan_owner(victim) && !chan_owner(user) && !glob_owner(user)) ||
      (glob_owner(victim) && !glob_owner(user))) {
    dprintf(idx, "\00304You have no permission to do that.\003\n");
    return;
  }
  chanrec = get_chanrec(u1, chan->dname);
  if (chanrec) {
    dprintf(idx, "User %s already has a channel record for %s.\n",
            nick, chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "\00307#+chrec#\003 \00306%s %s\003 %s", nick, chan->dname, dcc[idx].nick);
  add_chanrec(u1, chan->dname);
  dprintf(idx, "Added %s channel record for %s.\n", chan->dname, nick);
}

static void cmd_mns_chrec(struct userrec *u, int idx, char *par)
{
  char *nick, *chn = NULL;
  struct userrec *u1;
  struct chanuserrec *chanrec;

  if (!par[0]) {
    dprintf(idx, "Usage: -chrec <user> [channel]\n");
    return;
  }
  nick = newsplit(&par);
  u1 = get_user_by_handle(userlist, nick);
  if (!u1) {
    dprintf(idx, "\00304No such user.\003\n");
    return;
  }
  if (!par[0]) {
    struct chanset_t *chan;

    chan = findchan_by_dname(dcc[idx].u.chat->con_chan);
    if (chan)
      chn = chan->dname;
    else {
      dprintf(idx, "\00304Invalid console channel.\003\n");
      return;
    }
  } else
    chn = newsplit(&par);
  get_user_flagrec(u, &user, chn);
  get_user_flagrec(u1, &victim, chn);
  if ((!glob_master(user) && !chan_master(user)) ||     /* drummer */
      (chan_owner(victim) && !chan_owner(user) && !glob_owner(user)) ||
      (glob_owner(victim) && !glob_owner(user))) {
    dprintf(idx, "\00304You have no permission to do that.\003\n");
    return;
  }
  chanrec = get_chanrec(u1, chn);
  if (!chanrec) {
    dprintf(idx, "User %s doesn't have a channel record for %s.\n", nick, chn);
    return;
  }
  putlog(LOG_CMDS, "*", "\00307#-chrec#\003 \00306%s %s\003 %s", nick, chn, dcc[idx].nick);
  del_chanrec(u1, chn);
  dprintf(idx, "Removed %s channel record from %s.\n", chn, nick);
}

static void cmd_pls_chan(struct userrec *u, int idx, char *par)
{
  int i, argc;
  EGG_CONST char **argv;
  char *chname;
  struct chanset_t *chan;

  if (!par[0]) {
    dprintf(idx, "Usage: +chan [%s]<channel> [options]\n", CHANMETA);
    return;
  }

  chname = newsplit(&par);
  if (findchan_by_dname(chname)) {
    dprintf(idx, "\00304That channel already exists!\003\n");
    return;
  } else if ((chan = findchan(chname))) {
    dprintf(idx, "\00304That channel already exists as %s!\003\n", chan->dname);
    return;
  } else if (strchr(CHANMETA, chname[0]) == NULL) {
    dprintf(idx, "\00304Invalid channel prefix.\003\n");
    return;
  } else if (strchr(chname, ',') != NULL) {
    dprintf(idx, "\00304Invalid channel name.\003\n");
    return;
  }

  if (Tcl_SplitList(NULL, par, &argc, &argv ) == TCL_ERROR) {
    dprintf(idx, "\00304Invalid channel options.\003\n");
    return;
  }
  for (i = 0; i < argc; i++) {
    if ((!strncmp(argv[i], "need-", 5) || !strcmp(argv[i] + 1, "static"))
        && (!(u->flags & USER_OWNER) || (!isowner(dcc[idx].nick)
        && must_be_owner))) {
      dprintf(idx, "\00304Due to security concerns, only permanent owners can "
                   "set the need-* and +/-static modes.\003\n");
      Tcl_Free((char *) argv);
      return;
    }
    if (argv[i][0] == '-' || argv[i][0] == '+')
      continue;
    i++;
  }
  Tcl_Free((char *) argv);

  if (tcl_channel_add(0, chname, par) == TCL_ERROR)
    dprintf(idx, "\00304Invalid channel or channel options.\003\n");
  else
    putlog(LOG_CMDS, "*", "\00307#+chan#\003 \00306%s\003 %s", chname, dcc[idx].nick);
}

static void cmd_mns_chan(struct userrec *u, int idx, char *par)
{
  char *chname;
  struct chanset_t *chan;
  int i;

  if (!par[0]) {
    dprintf(idx, "Usage: -chan [%s]<channel>\n", CHANMETA);
    return;
  }
  chname = newsplit(&par);
  chan = findchan_by_dname(chname);
  if (!chan) {
    if ((chan = findchan(chname)))
      dprintf(idx, "\00304That channel exists with a short name of %s, use that.\003\n",
              chan->dname);
    else
      dprintf(idx, "\00304That channel doesn't exist!\003\n");
    return;
  }
  if (channel_static(chan)) {
    dprintf(idx, "\00304Cannot remove %s, it is a static channel!\003\n", chname);
    return;
  }

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type->flags & DCT_CHAT) &&
        !rfc_casecmp(dcc[i].u.chat->con_chan, chan->dname)) {
      dprintf(i, "%s is no longer a valid channel, changing your console "
              "to '*'\n", chname);
      strcpy(dcc[i].u.chat->con_chan, "*");
    }
  remove_channel(chan);
  dprintf(idx, "Channel %s removed from the bot.\n", chname);
  dprintf(idx, "This includes any channel specific bans, invites, exemptions "
          "and user records that you set.\n");
  putlog(LOG_CMDS, "*", "\00307#-chan#\003 \00306%s\003 %s", chname, dcc[idx].nick);
}

static void cmd_chaninfo(struct userrec *u, int idx, char *par)
{
  char *chname, work[512];
  struct chanset_t *chan;
  int ii, tmp;
  struct udef_struct *ul;

  if (!par[0]) {
    chname = dcc[idx].u.chat->con_chan;
    if (chname[0] == '*') {
      dprintf(idx, "\00304Your console channel is invalid.\003\n");
      return;
    }
  } else {
    chname = newsplit(&par);
    get_user_flagrec(u, &user, chname);
    if (!glob_master(user) && !chan_master(user)) {
      dprintf(idx, "\00304You don't have access to %s.\003\n", chname);
      return;
    }
  }
  if (!(chan = findchan_by_dname(chname)))
    dprintf(idx, "\00304No such channel defined.\003\n");
  else {
    putlog(LOG_CMDS, "*", "\00307#chaninfo#\003 \00306%s\003 %s", chname, dcc[idx].nick);
    dprintf(idx, "\00309???\003 hybrid(core): \00314%s\003 \00306<%s>\003\n",
            chan->dname, channel_static(chan) ? "static" : "dynamic");
    get_mode_protect(chan, work);
    dprintf(idx, "\00309???\003 chanmode: \00314%s\003\n", work[0] ? work : "None");
    if (chan->idle_kick)
      dprintf(idx, "\00309???\003 idle-kick: \00314%d\003\n", chan->idle_kick);
    else
      dprintf(idx, "\00309???\003 idle-kick: \00314none\003\n");
    if (chan->stopnethack_mode && chan->revenge_mode) {
      dprintf(idx, "\00309???\003 stopnethack-mode: \00314%d\003 \00301,01.\003 revenge-mode: \00314%d\003\n", chan->stopnethack_mode, chan->revenge_mode);
    } else if (chan->stopnethack_mode) {
      dprintf(idx, "\00309???\003 stopnethack-mode: \00314%d\003 \00301,01.\003 revenge-mode: \00314none\003\n", chan->stopnethack_mode);
    } else if (chan->revenge_mode) {
      dprintf(idx, "\00309???\003 stopnethack-mode: \00314none\003 \00301,01.\003 revenge-mode: \00314%d\003\n", chan->revenge_mode);
    } else {
      dprintf(idx, "\00309???\003 stopnethack: \00314none\003 \00301,01.\003 revenge-mode: \00314none\003\n");
    }
    dprintf(idx, "\00309???\003 aop-delay: \00314%d:%d\003\n", chan->aop_min, chan->aop_max);
    if (chan->ban_time)
      dprintf(idx, "\00309???\003 ban-type: \00314%d\003 \00301,01........\003 ban-time: \00314%d\003\n", chan->ban_type, chan->ban_time);
    else
      dprintf(idx, "\00309???\003 ban-type: \00314%d\003 \00301,01........\003 ban-time: \003140\003\n", chan->ban_type);
    if (chan->exempt_time && chan->invite_time)
      dprintf(idx, "\00309???\003 exempt-time: \00314%d\003 \00301,01.....\003 invite-time: \00314%d\003\n", chan->exempt_time, chan->invite_time);
    else if (chan->exempt_time)
      dprintf(idx, "\00309???\003 exempt-time: \00314%d\003 \00301,01.....\003 invite-time: \003140\003\n", chan->exempt_time);
    else if (chan->invite_time)
      dprintf(idx, "\00309???\003 exempt-time: \003140\003 \00301,01.....\003 invite-time: \00314%d\003\n", chan->invite_time);
    else
      dprintf(idx, "\00309???\003 exempt-time: \003140\003 \00301,01.....\003 invite-time: \003140\003\n");
    /* Only bot owners can see/change these (they're TCL commands) */
    if (u->flags & USER_OWNER) {
      if (chan->need_op[0])
        dprintf(idx, "To regain op's (need-op):\n%s\n", chan->need_op);
      if (chan->need_invite[0])
        dprintf(idx, "To get invite (need-invite):\n%s\n", chan->need_invite);
      if (chan->need_key[0])
        dprintf(idx, "To get key (need-key):\n%s\n", chan->need_key);
      if (chan->need_unban[0])
        dprintf(idx, "If I'm banned (need-unban):\n%s\n", chan->need_unban);
      if (chan->need_limit[0])
        dprintf(idx, "When channel full (need-limit):\n%s\n",
                chan->need_limit);
    }
    dprintf(idx, "\00309???\003 hybrid(core): \00314channel flags\003\n");
    dprintf(idx,
            "\00301,01.\003 %sinactive\003 \00301,01.......\003 %sstatuslog\003 \00301,01......\003 %ssecret\003 \00301,01.........\003 "
            "%sshared\003\n", (chan->status & CHAN_INACTIVE) ? "\00315+" : "\00305-",
            (chan->status & CHAN_LOGSTATUS) ? "\00315+" : "\00305-",
            (chan->status & CHAN_SECRET) ? "\00315+" : "\00305-",
            (chan->status & CHAN_SHARED) ? "\00315+" : "\00305-");
    dprintf(idx,
            "\00301,01.\003 %sgreet\003 \00301,01..........\003 %sseen\003 \00301,01...........\003 %scycle\003 \00301,01..........\003 "
            "%sdontkickops\003\n", (chan->status & CHAN_GREET) ? "\00315+" : "\00305-",
            (chan->status & CHAN_SEEN) ? "\00315+" : "\00305-",
            (chan->status & CHAN_CYCLE) ? "\00315+" : "\00305-",
            (chan->status & CHAN_DONTKICKOPS) ? "\00315+" : "\00305-");
    dprintf(idx,
            "\00301,01.\003 %sprotectops\003 \00301,01.....\003 %sprotectfriends\003 \00301,01.\003 %srevenge\003 \00301,01........\003 "
            "%srevengebot\003\n", (chan->status & CHAN_PROTECTOPS) ? "\00315+" : "\00305-",
            (chan->status & CHAN_PROTECTFRIENDS) ? "\00315+" : "\00305-",
            (chan->status & CHAN_REVENGE) ? "\00315+" : "\00305-",
            (chan->status & CHAN_REVENGEBOT) ? "\00315+" : "\00305-");
    dprintf(idx,
            "\00301,01.\003 %sbitch\003 \00301,01..........\003 %sautoop\003 \00301,01.........\003 %sautovoice\003 \00301,01......\003 "
            "%snodesynch\003\n", (chan->status & CHAN_BITCH) ? "\00315+" : "\00305-",
            (chan->status & CHAN_OPONJOIN) ? "\00315+" : "\00305-",
            (chan->status & CHAN_AUTOVOICE) ? "\00315+" : "\00305-",
            (chan->status & CHAN_NODESYNCH) ? "\00315+" : "\00305-");
    dprintf(idx,
            "\00301,01.\003 %senforcebans\003 \00301,01....\003 %sdynamicbans\003 \00301,01....\003 %suserbans\003 \00301,01.......\003 "
            "%sautohalfop\003\n", (chan->status & CHAN_ENFORCEBANS) ? "\00315+" : "\00305-",
            (chan->status & CHAN_DYNAMICBANS) ? "\00315+" : "\00305-",
            (chan->status & CHAN_NOUSERBANS) ? "\00305-" : "\00315+",
            (chan->status & CHAN_AUTOHALFOP) ? "\00315+" : "\00305-");
    dprintf(idx, "\00301,01.\003 %sprotecthalfops\003 \00301,01.\003 %sstatic\003\n",
            (chan->status & CHAN_PROTECTHALFOPS) ? "\00315+" : "\00305-",
            (chan->status & CHAN_STATIC) ? "\00315+" : "\00305-");
    dprintf(idx,
            "\00301,01.\003 %sdynamicexempts\003 \00301,01.\003 %suserexempts\003 \00301,01....\003 %sdynamicinvites\003 \00301,01.\003 "
            "%suserinvites\003\n",
            (chan->ircnet_status & CHAN_DYNAMICEXEMPTS) ? "\00315+" : "\00305-",
            (chan->ircnet_status & CHAN_NOUSEREXEMPTS) ? "\00305-" : "\00315+",
            (chan->ircnet_status & CHAN_DYNAMICINVITES) ? "\00315+" : "\00305-",
            (chan->ircnet_status & CHAN_NOUSERINVITES) ? "\00305-" : "\00315+");

    ii = 1;
    tmp = 0;
    for (ul = udef; ul; ul = ul->next)
      if (ul->defined && ul->type == UDEF_FLAG) {
        int work_len;

        if (!tmp) {
          dprintf(idx, "\00309???\003 ak!ra: \00314channel flags\003\n");
          tmp = 1;
        }
        if (ii == 1)
          egg_snprintf(work, sizeof work, " \00301,01.\003");
        work_len = strlen(work);
        if (ii == 1)
          egg_snprintf(work + work_len, sizeof(work) - work_len, "%s%s\003",
                     getudef(ul->values, chan->dname) ? "\00315+" : "\00305-", ul->name);
        else
          egg_snprintf(work + work_len, sizeof(work) - work_len, " \00301,01.\003 %s%s\003",
                     getudef(ul->values, chan->dname) ? "\00315+" : "\00305-", ul->name);
        ii++;
        if (ii > 4) {
          dprintf(idx, "%s\n", work);
          ii = 1;
        }
      }
    if (ii > 1)
      dprintf(idx, "%s\n", work);

    work[0] = 0;
    ii = 1;
    tmp = 0;
    for (ul = udef; ul; ul = ul->next)
      if (ul->defined && ul->type == UDEF_INT) {
        int work_len = strlen(work);

        if (!tmp) {
          dprintf(idx, "\00309???\003 ak!ra: \00314channel settings\003\n");
          tmp = 1;
        }
        egg_snprintf(work + work_len, sizeof(work) - work_len, "%s: %d   ",
                     ul->name, getudef(ul->values, chan->dname));
        ii++;
        if (ii > 4) {
          dprintf(idx, "%s\n", work);
          work[0] = 0;
          ii = 1;
        }
      }
    if (ii > 1)
      dprintf(idx, "%s\n", work);

    if (u->flags & USER_OWNER) {
      tmp = 0;

      for (ul = udef; ul; ul = ul->next) {
        if (ul->defined && ul->type == UDEF_STR) {
          char *p = (char *) getudef(ul->values, chan->dname);

          if (!p)
            p = "{}";

          if (!tmp) {
            dprintf(idx, "\00309???\003 ak!ra: \00314channel strings\003\n");
            tmp = 1;
          }
          dprintf(idx, "%s: %s\n", ul->name, p);
        }
      }
    }


    dprintf(idx, "\00309???\003 flood settings: \00314chan\003 \00306<%d in %d sec(s)>\003 \00314ctcp\003 \00306<%d in %d sec(s)>\003 \00314join\003 \00306<%d in %d sec(s)>\003\n",
            chan->flood_pub_thr, chan->flood_pub_time,
            chan->flood_ctcp_thr, chan->flood_ctcp_time,
            chan->flood_join_thr, chan->flood_join_time);
    dprintf(idx, "\00301,01..................\003\00314kick\003 \00306<%d in %d sec(s)>\003 \00314deop\003 \00306<%d in %d sec(s)>\003 \00314nick\003 \00306<%d in %d sec(s)>\003\n",
            chan->flood_kick_thr, chan->flood_kick_time,
            chan->flood_deop_thr, chan->flood_deop_time,
            chan->flood_nick_thr, chan->flood_nick_time);
/*
    dprintf(idx, "number:          %3d  %3d  %3d  %3d  %3d  %3d\n",
            chan->flood_pub_thr, chan->flood_ctcp_thr,
            chan->flood_join_thr, chan->flood_kick_thr,
            chan->flood_deop_thr, chan->flood_nick_thr);
    dprintf(idx, "time  :          %3d  %3d  %3d  %3d  %3d  %3d\n",
            chan->flood_pub_time, chan->flood_ctcp_time,
            chan->flood_join_time, chan->flood_kick_time,
            chan->flood_deop_time, chan->flood_nick_time);
*/
  }
}

static void cmd_chanset(struct userrec *u, int idx, char *par)
{
  char *chname = NULL, answers[512], *parcpy;
  char *list[2], *bak, *buf;
  struct chanset_t *chan = NULL;
  int all = 0;

  if (!par[0])
    dprintf(idx, "Usage: chanset [%schannel] <settings>\n", CHANMETA);
  else {
    if (strlen(par) > 2 && par[0] == '*' && par[1] == ' ') {
      all = 1;
      get_user_flagrec(u, &user, chanset ? chanset->dname : "");
      if (!glob_master(user)) {
        dprintf(idx, "\00304You need to be a global master to use .chanset *.\003\n");
        return;
      }
      newsplit(&par);
    } else {
      if (strchr(CHANMETA, par[0])) {
        chname = newsplit(&par);
        get_user_flagrec(u, &user, chname);
        if (!glob_master(user) && !chan_master(user)) {
          dprintf(idx, "\00304You don't have access to %s.\003\n", chname);
          return;
        } else if (!(chan = findchan_by_dname(chname)) && (chname[0] != '+')) {
          dprintf(idx, "\00304That channel doesn't exist!\003\n");
          return;
        }
        if (!chan) {
          if (par[0])
            *--par = ' ';
          par = chname;
        }
      }
      if (!par[0] || par[0] == '*') {
        dprintf(idx, "Usage: chanset [%schannel] <settings>\n", CHANMETA);
        return;
      }
      if (!chan &&
          !(chan = findchan_by_dname(chname = dcc[idx].u.chat->con_chan))) {
        dprintf(idx, "\00304Invalid console channel.\003\n");
        return;
      }
    }
    if (all)
      chan = chanset;
    bak = par;
    buf = nmalloc(strlen(par) + 1);
    while (chan) {
      chname = chan->dname;
      strcpy(buf, bak);
      par = buf;
      list[0] = newsplit(&par);
      answers[0] = 0;
      while (list[0][0]) {
        if (list[0][0] == '+' || list[0][0] == '-' ||
            (!strcmp(list[0], "dont-idle-kick"))) {
            if (!strcmp(list[0] + 1, "static") && must_be_owner &&
                !(isowner(dcc[idx].nick))) {
              dprintf(idx, "\00304Only permanent owners can modify the static flag.\003\n");
              nfree(buf);
              return;
            }
          if (tcl_channel_modify(0, chan, 1, list) == TCL_OK) {
            strcat(answers, list[0]);
            strcat(answers, " ");
          } else if (!all || !chan->next)
            dprintf(idx, "\00304Error trying to set %s for %s, invalid mode.\003\n",
                    list[0], all ? "all channels" : chname);
          list[0] = newsplit(&par);
          continue;
        }
        /* The rest have an unknown amount of args, so assume the rest of the
         * line is args. Woops nearly made a nasty little hole here :) we'll
         * just ignore any non global +n's trying to set the need-commands.
         */
        if (strncmp(list[0], "need-", 5) || (u->flags & USER_OWNER)) {
          Tcl_Interp *irp = NULL;
          if (!strncmp(list[0], "need-", 5) && !(isowner(dcc[idx].nick)) &&
              must_be_owner) {
            dprintf(idx, "\00304Due to security concerns, only permanent owners can set these modes.\003\n");
            nfree(buf);
            return;
          }
          list[1] = par;
          /* Par gets modified in tcl_channel_modify under some
           * circumstances, so save it now.
           */
          parcpy = nmalloc(strlen(par) + 1);
          strcpy(parcpy, par);
          irp = Tcl_CreateInterp();
          if (tcl_channel_modify(irp, chan, 2, list) == TCL_OK) {
            int len = strlen(answers);
            egg_snprintf(answers + len, (sizeof answers) - len, "%s { %s }", list[0], parcpy); /* Concatenation */
          } else if (!all || !chan->next)
            dprintf(idx, "\00304Error trying to set %s for %s, %s\003\n",
                    list[0], all ? "all channels" : chname, Tcl_GetStringResult(irp));
          Tcl_ResetResult(irp);
          Tcl_DeleteInterp(irp);
          nfree(parcpy);
        }
        break;
      }
      if (!all && answers[0]) {
        dprintf(idx, "Successfully set modes { %s } on %s.\n", answers,
                chname);
        putlog(LOG_CMDS, "*", "\00307#chanset#\003 \00306%s %s\003 %s", chname,
               answers, dcc[idx].nick);
      }
      if (!all)
        chan = NULL;
      else
        chan = chan->next;
    }
    if (all && answers[0]) {
      dprintf(idx, "Successfully set modes { %s } on all channels.\n",
              answers);
      putlog(LOG_CMDS, "*", "\00307#chanset#\003 \00306* %s\003 %s", answers, dcc[idx].nick);
    }
    nfree(buf);
  }
}

static void cmd_chansave(struct userrec *u, int idx, char *par)
{
  if (!chanfile[0])
    dprintf(idx, "\00304No channel saving file defined.\003\n");
  else {
    putlog(LOG_CMDS, "*", "\00307#chansave#\003 %s", dcc[idx].nick);
    putlog(LOG_CMDS, "*", "\00309???\003 hybrid(core): \00314save\003 \00306<chanfile>\003");
    write_channels();
    check_tcl_event("savechannels");
  }
}

static void cmd_chanload(struct userrec *u, int idx, char *par)
{
  if (!chanfile[0])
    dprintf(idx, "\00304No channel saving file defined.\003\n");
  else {
    putlog(LOG_CMDS, "*", "\00307#chanload#\003 %s", dcc[idx].nick);
    putlog(LOG_CMDS, "*", "\00309???\003 hybrid(core): \00314reload\003 \00306<chanfile>\003");
    read_channels(1, 1);
    check_tcl_event("loadchannels");
  }
}

/* DCC CHAT COMMANDS
 *
 * Function call should be:
 *    int cmd_whatever(idx,"parameters");
 *
 * NOTE: As with msg commands, the function is responsible for any logging.
 */
static cmd_t C_dcc_irc[] = {
  {"+ban",     "ol|ol", (IntFunc) cmd_pls_ban,    NULL},
  {"+exempt",  "ol|ol", (IntFunc) cmd_pls_exempt, NULL},
  {"+invite",  "ol|ol", (IntFunc) cmd_pls_invite, NULL},
  {"+chan",    "n",     (IntFunc) cmd_pls_chan,   NULL},
  {"+chrec",   "m|m",   (IntFunc) cmd_pls_chrec,  NULL},
  {"-ban",     "ol|ol", (IntFunc) cmd_mns_ban,    NULL},
  {"-chan",    "n",     (IntFunc) cmd_mns_chan,   NULL},
  {"-chrec",   "m|m",   (IntFunc) cmd_mns_chrec,  NULL},
  {"bans",     "ol|ol", (IntFunc) cmd_bans,       NULL},
  {"-exempt",  "ol|ol", (IntFunc) cmd_mns_exempt, NULL},
  {"-invite",  "ol|ol", (IntFunc) cmd_mns_invite, NULL},
  {"exempts",  "ol|ol", (IntFunc) cmd_exempts,    NULL},
  {"invites",  "ol|ol", (IntFunc) cmd_invites,    NULL},
  {"chaninfo", "m|m",   (IntFunc) cmd_chaninfo,   NULL},
  {"chanload", "n|n",   (IntFunc) cmd_chanload,   NULL},
  {"chanset",  "n|n",   (IntFunc) cmd_chanset,    NULL},
  {"chansave", "n|n",   (IntFunc) cmd_chansave,   NULL},
  {"chinfo",   "m|m",   (IntFunc) cmd_chinfo,     NULL},
  {"info",     "",      (IntFunc) cmd_info,       NULL},
  {"stick",    "ol|ol", (IntFunc) cmd_stick,      NULL},
  {"unstick",  "ol|ol", (IntFunc) cmd_unstick,    NULL},
  {NULL,       NULL,    NULL,                      NULL}
};
