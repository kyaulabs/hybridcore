/*
 * chancmds.c -- part of irc.mod
 *   handles commands directly relating to channel interaction
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

static struct chanset_t *get_channel(int idx, char *chname)
{
  struct chanset_t *chan;

  if (chname && chname[0]) {
    chan = findchan_by_dname(chname);
    if (chan)
      return chan;
    else
      dprintf(idx, "\00304No such channel.\003\n");
  } else {
    chname = dcc[idx].u.chat->con_chan;
    chan = findchan_by_dname(chname);
    if (chan)
      return chan;
    else
      dprintf(idx, "\00304Invalid console channel.\003\n");
  }
  return 0;
}

/* Do we have any flags that will allow us ops on a channel?
 */
static int has_op(int idx, struct chanset_t *chan)
{
  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  if (chan_op(user) || (glob_op(user) && !chan_deop(user)))
    return 1;
  dprintf(idx, "\00304You are not a channel op on %s.\003\n", chan->dname);
  return 0;
}

static int has_oporhalfop(int idx, struct chanset_t *chan)
{
  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  if (chan_op(user) || chan_halfop(user) || (glob_op(user) &&
      !chan_deop(user)) || (glob_halfop(user) && !chan_dehalfop(user)))
    return 1;
  dprintf(idx, "\00304You are not a channel op or halfop on %s.\003\n", chan->dname);
  return 0;
}

/* Finds a nick of the handle. Returns m->nick if
 * the nick was found, otherwise NULL (Sup 1Nov2000)
 */
static char *getnick(char *handle, struct chanset_t *chan)
{
  char s[UHOSTLEN];
  struct userrec *u;
  memberlist *m;

  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    if ((u = get_user_by_host(s)) && !egg_strcasecmp(u->handle, handle))
      return m->nick;
  }
  return NULL;
}

static void cmd_act(struct userrec *u, int idx, char *par)
{
  char *chname;
  struct chanset_t *chan;
  memberlist *m;

  if (!par[0]) {
    dprintf(idx, "Usage: act [channel] <action>\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  chan = get_channel(idx, chname);
  if (!chan || !has_op(idx, chan))
    return;
  m = ismember(chan, botname);
  if (!m) {
    dprintf(idx, "\00304Cannot say to %s: I'm not on that channel.\003\n", chan->dname);
    return;
  }
  if ((chan->channel.mode & CHANMODER) && !me_op(chan) && !me_halfop(chan) &&
      !me_voice(chan)) {
    dprintf(idx, "\00304Cannot say to %s: It is moderated.\003\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "\00307#(%s) act#\003 \00306%s\003 %s", chan->dname, par), dcc[idx].nick;
  dprintf(DP_HELP, "PRIVMSG %s :\001ACTION %s\001\n", chan->name, par);
  dprintf(idx, "Action to %s: %s\n", chan->dname, par);
}

static void cmd_msg(struct userrec *u, int idx, char *par)
{
  char *nick;

  nick = newsplit(&par);
  if (!par[0])
    dprintf(idx, "Usage: msg <nick> <message>\n");
  else {
    putlog(LOG_CMDS, "*", "\00307#msg#\003 \00306%s %s\003 %s", nick, par, dcc[idx].nick);
    dprintf(DP_HELP, "PRIVMSG %s :%s\n", nick, par);
    dprintf(idx, "Msg to %s: %s\n", nick, par);
  }
}

static void cmd_say(struct userrec *u, int idx, char *par)
{
  char *chname;
  struct chanset_t *chan;
  memberlist *m;

  if (!par[0]) {
    dprintf(idx, "Usage: say [channel] <message>\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  chan = get_channel(idx, chname);
  if (!chan || !has_op(idx, chan))
    return;
  m = ismember(chan, botname);
  if (!m) {
    dprintf(idx, "\00304Cannot say to %s: I'm not on that channel.\003\n", chan->dname);
    return;
  }
  if ((chan->channel.mode & CHANMODER) && !me_op(chan) && !me_halfop(chan) &&
      !me_voice(chan)) {
    dprintf(idx, "\00304Cannot say to %s: It is moderated.\003\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "\00307#(%s) say#\003 \00306%s\003 %s", chan->dname, par, dcc[idx].nick);
  dprintf(DP_HELP, "PRIVMSG %s :%s\n", chan->name, par);
  dprintf(idx, "Said to %s: %s\n", chan->dname, par);
}

static void cmd_kickban(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname, *nick, *s1;
  memberlist *m;
  char s[UHOSTLEN];
  char bantype = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: kickban [channel] [-|@]<nick> [reason]\n");
    return;
  }

  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;
  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }
  if (HALFOP_CANTDOMODE('b')) {
    dprintf(idx, "\00304I can't help you now because I'm not a channel op or halfop "
            "on %s, or halfops cannot set bans.\003\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "\00307#(%s) kickban#\003 \00306%s\003 %s",
         chan->dname, par, dcc[idx].nick);
  nick = newsplit(&par);
  if ((nick[0] == '@') || (nick[0] == '-')) {
    bantype = nick[0];
    nick++;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "\00304I'm not going to kickban myself.\003\n");
    return;
  }
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s\003\n", nick, chan->dname);
    return;
  }
  if (!me_op(chan) && chan_hasop(m)) {
    dprintf(idx, "\00304I can't help you now because halfops cannot kick ops.\003\n");
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "\00304%s is a legal op.\003\n", nick);
    return;
  }
  if ((chan_master(victim) || glob_master(victim)) &&
      !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "\00304%s is a %s master.\003\n", nick, chan->dname);
    return;
  }
  if (glob_bot(victim) && !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "\00304%s is another channel bot!\003\n", nick);
    return;
  }
  if (use_exempts && (u_match_mask(global_exempts, s) ||
      u_match_mask(chan->exempts, s))) {
    dprintf(idx, "\00304%s is permanently exempted!\003\n", nick);
    return;
  }
  if (m->flags & CHANOP)
    add_mode(chan, '-', 'o', m->nick);
  check_exemptlist(chan, s);
  switch (bantype) {
  case '@':
    s1 = strchr(s, '@');
    s1 -= 3;
    s1[0] = '*';
    s1[1] = '!';
    s1[2] = '*';
    break;
  case '-':
    s1 = strchr(s, '!');
    s1[1] = '*';
    s1--;
    s1[0] = '*';
    break;
  default:
    s1 = quickban(chan, m->userhost);
    break;
  }
  if (bantype == '@' || bantype == '-')
    do_mask(chan, chan->channel.ban, s1, 'b');
  if (!par[0])
    par = "requested";
  dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, m->nick, par);
  m->flags |= SENTKICK;
  u_addban(chan, s1, dcc[idx].nick, par, now + (60 * chan->ban_time), 0);
  dprintf(idx, "Okay, done.\n");
}

static void cmd_op(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan || !has_op(idx, chan))
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: op <nick> [channel]\n");
    return;
  }

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('o')) {
    dprintf(idx, "\00304I can't help you now because I'm not a chan op or halfop on \003"
            "\00304%s, or halfops cannot set +o modes.\003\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "\00307#(%s) op#\003 \00306%s\003 %s", chan->dname, nick, dcc[idx].nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s.\003\n", nick, chan->dname);
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if (chan_deop(victim) || (glob_deop(victim) && !glob_op(victim))) {
    dprintf(idx, "\00304%s is currently being auto-deopped.\003\n", m->nick);
    return;
  }
  if (channel_bitch(chan) && !(chan_op(victim) || (glob_op(victim) &&
      !chan_deop(victim)))) {
    dprintf(idx, "\00304%s is not a registered op.\003\n", m->nick);
    return;
  }
  add_mode(chan, '+', 'o', nick);
  dprintf(idx, "Gave op to %s on %s.\n", nick, chan->dname);
}

static void cmd_deop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan || !has_op(idx, chan))
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: deop <nick> [channel]\n");
    return;
  }

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('o')) {
    dprintf(idx, "\00304I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set -o modes.\003\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "\00307#(%s) deop#\003 \00306%s\003 %s", chan->dname, nick, dcc[idx].nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s.\003\n", nick, chan->dname);
    return;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "\00304I'm not going to deop myself.\003\n");
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_master(victim) || glob_master(victim)) &&
      !(chan_owner(user) || glob_owner(user))) {
    dprintf(idx, "\00304%s is a master for %s.\003\n", m->nick, chan->dname);
    return;
  }
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "\00304%s has the op flag for %s.\003\n", m->nick, chan->dname);
    return;
  }
  add_mode(chan, '-', 'o', nick);
  dprintf(idx, "Took op from %s on %s.\n", nick, chan->dname);
}

static void cmd_halfop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: halfop <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);
  if (m && !chan_op(user) && (!glob_op(user) || chan_deop(user))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_halfop(user) &&
        (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "\00304You are not a channel op on %s.\003\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('h')) {
    dprintf(idx, "\00304I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set +h modes.\003\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "\00307#(%s) halfop#\003 \00306%s\003 %s",
         chan->dname, nick, dcc[idx].nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s.\003\n", nick, chan->dname);
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if (chan_dehalfop(victim) || (glob_dehalfop(victim) && !glob_halfop(victim))) {
    dprintf(idx, "\00304%s is currently being auto-dehalfopped.\003\n", m->nick);
    return;
  }
  if (channel_bitch(chan) && !(chan_op(victim) || chan_op(victim) ||
      (glob_op(victim) && !chan_deop(victim)) || (glob_halfop(victim) &&
      !chan_dehalfop(victim)))) {
    dprintf(idx, "\00304%s is not a registered halfop.\003\n", m->nick);
    return;
  }
  add_mode(chan, '+', 'h', nick);
  dprintf(idx, "Gave halfop to %s on %s.\n", nick, chan->dname);
}

static void cmd_dehalfop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: dehalfop <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);
  if (m && !chan_op(user) && (!glob_op(user) || chan_deop(user))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_halfop(user) &&
        (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "\00304You are not a channel op on %s.\003\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('h')) {
    dprintf(idx, "\00304I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set -h modes.\003\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "\00307#(%s) dehalfop#\003 \00306%s\003 %s",
         chan->dname, nick, dcc[idx].nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s.\003\n", nick, chan->dname);
    return;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "\00304I'm not going to dehalfop myself.\003\n");
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_master(victim) || glob_master(victim)) &&
      !(chan_owner(user) || glob_owner(user))) {
    dprintf(idx, "\00304%s is a master for %s.\003\n", m->nick, chan->dname);
    return;
  }
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "\00304%s has the op flag for %s.\003\n", m->nick, chan->dname);
    return;
  }
  if ((chan_halfop(victim) || (glob_halfop(victim) &&
      !chan_dehalfop(victim))) && !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "\00304%s has the halfop flag for %s.\003\n", m->nick, chan->dname);
    return;
  }
  add_mode(chan, '-', 'h', nick);
  dprintf(idx, "Took halfop from %s on %s.\n", nick, chan->dname);
}

static void cmd_voice(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: voice <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);

  /* By factoring out a !, this code becomes a lot clearer.
   * If you are... not a (channel op, or a channel half op, or a global op
   * without channel deop, or a global halfop without channel dehalfop)...
   * - stdarg */
  if (m && !(chan_op(user) || chan_halfop(user) || (glob_op(user) &&
      !chan_deop(user)) || (glob_halfop(user) && !chan_dehalfop(user)))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_voice(user) &&
        (!glob_voice(user) || chan_quiet(user)))) {
      dprintf(idx, "\00304You are not a channel op or halfop on %s.\003\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('v')) {
    dprintf(idx, "\00304I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set +v modes.\003\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "\00307#(%s) voice#\003 \00306%s\003 %s", chan->dname, nick, dcc[idx].nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s.\003\n", nick, chan->dname);
    return;
  }
  add_mode(chan, '+', 'v', nick);
  dprintf(idx, "Gave voice to %s on %s\n", nick, chan->dname);
}

static void cmd_devoice(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: devoice <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);
  if (m && !(chan_op(user) || chan_halfop(user) || (glob_op(user) &&
      !chan_deop(user)) || (glob_halfop(user) && !chan_dehalfop(user)))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_voice(user) &&
        (!glob_voice(user) || chan_quiet(user)))) {
      dprintf(idx, "\00304You are not a channel op or halfop on %s.\003\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('v')) {
    dprintf(idx, "\00304I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set -v modes.\003\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "\00307#(%s) devoice#\003 \00306%s\003 %s",
         chan->dname, nick, dcc[idx].nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s.\003\n", nick, chan->dname);
    return;
  }
  add_mode(chan, '-', 'v', nick);
  dprintf(idx, "Devoiced %s on %s\n", nick, chan->dname);
}

static void cmd_kick(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname, *nick;
  memberlist *m;
  char s[UHOSTLEN];

  if (!par[0]) {
    dprintf(idx, "Usage: kick [channel] <nick> [reason]\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;
  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }
  if (!me_op(chan) && !me_halfop(chan)) {
    dprintf(idx, "\00304I can't help you now because I'm not a channel op or halfop "
            "on %s.\003\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "\00307#(%s) kick#\003 \00306%s\003 %s", chan->dname, par, dcc[idx].nick);
  nick = newsplit(&par);
  if (!par[0])
    par = "request";
  if (match_my_nick(nick)) {
    dprintf(idx, "\00304I'm not going to kick myself.\003\n");
    return;
  }
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "\00304%s is not on %s\003\n", nick, chan->dname);
    return;
  }
  if (!me_op(chan) && chan_hasop(m)) {
    dprintf(idx, "\00304I can't help you now because halfops cannot kick ops.\003\n",
            chan->dname);
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "\00304%s is a legal op.\003\n", nick);
    return;
  }
  if ((chan_master(victim) || glob_master(victim)) &&
      !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "\00304%s is a %s master.\003\n", nick, chan->dname);
    return;
  }
  if (glob_bot(victim) && !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "\00304%s is another channel bot!\003\n", nick);
    return;
  }
  dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, m->nick, par);
  m->flags |= SENTKICK;
  dprintf(idx, "Okay, done.\n");
}

static void cmd_invite(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  memberlist *m;
  char *nick;

  if (!par[0])
    par = dcc[idx].nick;
  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "\00307#(%s) invite#\003 \00306%s\003 %s", chan->dname,
         nick, dcc[idx].nick);
  if (!me_op(chan) && !me_halfop(chan)) {
    if (chan->channel.mode & CHANINV) {
      dprintf(idx, "\00304I can't help you now because I'm not a channel op or "
              "halfop on %s.\003\n", chan->dname);
      return;
    }
    if (!channel_active(chan)) {
      dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
      return;
    }
  }
  m = ismember(chan, nick);
  if (m && !chan_issplit(m)) {
    dprintf(idx, "\00304%s is already on %s!\003\n", nick, chan->dname);
    return;
  }
  dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
  dprintf(idx, "Inviting %s to %s.\n", nick, chan->dname);
}

static void cmd_channel(struct userrec *u, int idx, char *par)
{
  char handle[HANDLEN + 1], s[UHOSTLEN], s1[UHOSTLEN], atrflag, chanflag;
  struct chanset_t *chan;
  memberlist *m;
  int maxnicklen, maxhandlen;
  char format[81];

  chan = get_channel(idx, par);
  if (!chan || !has_oporhalfop(idx, chan))
    return;
  putlog(LOG_CMDS, "*", "\00307#(%s) channel#\003 %s", chan->dname, dcc[idx].nick);
  strlcpy(s, getchanmode(chan), sizeof s);
  if (channel_pending(chan))
    egg_snprintf(s1, sizeof s1, "%s %s", IRC_PROCESSINGCHAN, chan->dname);
  else if (channel_active(chan))
    egg_snprintf(s1, sizeof s1, "%s %s", IRC_CHANNEL, chan->dname);
  else
    egg_snprintf(s1, sizeof s1, "%s %s", IRC_DESIRINGCHAN, chan->dname);
  dprintf(idx, "%s, %d member%s, mode %s:\n", s1, chan->channel.members,
          chan->channel.members == 1 ? "" : "s", s);
  if (chan->channel.topic)
    dprintf(idx, "%s: %s\n", IRC_CHANNELTOPIC, chan->channel.topic);
  if (channel_active(chan)) {
    /* find max nicklen and handlen */
    maxnicklen = maxhandlen = 0;
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (strlen(m->nick) > maxnicklen)
        maxnicklen = strlen(m->nick);
      if ((m->user) && (strlen(m->user->handle) > maxhandlen))
        maxhandlen = strlen(m->user->handle);
    }
    if (maxnicklen < 9)
      maxnicklen = 9;
    if (maxhandlen < 9)
      maxhandlen = 9;

    dprintf(idx, "(n = owner, m = master, o = op, d = deop, b = bot)\n");
    egg_snprintf(format, sizeof format, " %%-%us %%-%us %%-6s %%-5s %%s\n",
                 maxnicklen, maxhandlen);
    dprintf(idx, format, "NICKNAME", "HANDLE", " JOIN", "IDLE", "USER@HOST");
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (m->joined > 0) {
        if ((now - (m->joined)) > 86400)
          egg_strftime(s, 6, "%d%b", localtime(&(m->joined)));
        else
          egg_strftime(s, 6, "%H:%M", localtime(&(m->joined)));
      } else
        strlcpy(s, " --- ", sizeof s);
      if (m->user == NULL) {
        egg_snprintf(s1, sizeof s1, "%s!%s", m->nick, m->userhost);
        m->user = get_user_by_host(s1);
      }
      if (m->user == NULL)
        strlcpy(handle, "*", sizeof handle);
      else
        strlcpy(handle, m->user->handle, sizeof handle);
      get_user_flagrec(m->user, &user, chan->dname);
      /* Determine status char to use */
      if (glob_bot(user) && (glob_op(user) || chan_op(user)))
        atrflag = 'B';
      else if (glob_bot(user))
        atrflag = 'b';
      else if (glob_owner(user))
        atrflag = 'N';
      else if (chan_owner(user))
        atrflag = 'n';
      else if (glob_master(user))
        atrflag = 'M';
      else if (chan_master(user))
        atrflag = 'm';
      else if (glob_deop(user))
        atrflag = 'D';
      else if (chan_deop(user))
        atrflag = 'd';
      else if (glob_dehalfop(user))
        atrflag = 'R';
      else if (chan_dehalfop(user))
        atrflag = 'r';
      else if (glob_autoop(user))
        atrflag = 'A';
      else if (chan_autohalfop(user))
        atrflag = 'y';
      else if (glob_autohalfop(user))
        atrflag = 'Y';
      else if (chan_autoop(user))
        atrflag = 'a';
      else if (glob_op(user))
        atrflag = 'O';
      else if (chan_op(user))
        atrflag = 'o';
      else if (glob_halfop(user))
        atrflag = 'L';
      else if (chan_halfop(user))
        atrflag = 'l';
      else if (glob_quiet(user))
        atrflag = 'Q';
      else if (chan_quiet(user))
        atrflag = 'q';
      else if (glob_gvoice(user))
        atrflag = 'G';
      else if (chan_gvoice(user))
        atrflag = 'g';
      else if (glob_voice(user))
        atrflag = 'V';
      else if (chan_voice(user))
        atrflag = 'v';
      else if (glob_friend(user))
        atrflag = 'F';
      else if (chan_friend(user))
        atrflag = 'f';
      else if (glob_kick(user))
        atrflag = 'K';
      else if (chan_kick(user))
        atrflag = 'k';
      else if (glob_wasoptest(user))
        atrflag = 'W';
      else if (chan_wasoptest(user))
        atrflag = 'w';
      else if (glob_exempt(user))
        atrflag = 'E';
      else if (chan_exempt(user))
        atrflag = 'e';
      else
        atrflag = ' ';
      if (chan_hasop(m))
        chanflag = '@';
      else if (chan_hashalfop(m))
        chanflag = '%';
      else if (chan_hasvoice(m))
        chanflag = '+';
      else
        chanflag = ' ';
      if (chan_issplit(m)) {
        egg_snprintf(format, sizeof format,
                     "%%c%%-%us %%-%us %%s %%c     <- netsplit, %%lus\n",
                     maxnicklen, maxhandlen);
        dprintf(idx, format, chanflag, m->nick, handle, s, atrflag,
                now - (m->split));
      } else if (!rfc_casecmp(m->nick, botname)) {
        egg_snprintf(format, sizeof format,
                     "%%c%%-%us %%-%us %%s %%c     <- it's me!\n",
                     maxnicklen, maxhandlen);
        dprintf(idx, format, chanflag, m->nick, handle, s, atrflag);
      } else {
        /* Determine idle time */
        if (now - (m->last) > 86400)
          egg_snprintf(s1, sizeof s1, "%2lud", ((now - (m->last)) / 86400));
        else if (now - (m->last) > 3600)
          egg_snprintf(s1, sizeof s1, "%2luh", ((now - (m->last)) / 3600));
        else if (now - (m->last) > 180)
          egg_snprintf(s1, sizeof s1, "%2lum", ((now - (m->last)) / 60));
        else
          strlcpy(s1, "   ", sizeof s1);
        egg_snprintf(format, sizeof format,
                     "%%c%%-%us %%-%us %%s %%c %%s  %%s\n", maxnicklen,
                     maxhandlen);
        dprintf(idx, format, chanflag, m->nick, handle, s, atrflag, s1,
                m->userhost);
      }
      if (chan_fakeop(m))
        dprintf(idx, "    (%s)\n", IRC_FAKECHANOP);
      if (chan_sentop(m))
        dprintf(idx, "    (%s)\n", IRC_PENDINGOP);
      if (chan_sentdeop(m))
        dprintf(idx, "    (%s)\n", IRC_PENDINGDEOP);
      if (chan_sentkick(m))
        dprintf(idx, "    (%s)\n", IRC_PENDINGKICK);
    }
  }
  dprintf(idx, "%s\n", IRC_ENDCHANINFO);
}

static void cmd_topic(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;

  if (par[0] && (strchr(CHANMETA, par[0]) != NULL)) {
    char *chname = newsplit(&par);

    chan = get_channel(idx, chname);
  } else
    chan = get_channel(idx, "");

  if (!chan || !has_oporhalfop(idx, chan))
    return;

  if (!channel_active(chan)) {
    dprintf(idx, "\00304I'm not on %s right now!\003\n", chan->dname);
    return;
  }
  if (!par[0]) {
    if (chan->channel.topic)
      dprintf(idx, "The topic for %s is: %s\n", chan->dname,
              chan->channel.topic);
    else
      dprintf(idx, "No topic is set for %s\n", chan->dname);
  } else if (channel_optopic(chan) && !me_op(chan) && !me_halfop(chan))
    dprintf(idx, "\00304I'm not a channel op or halfop on %s and the channel is "
            "+t.\003\n", chan->dname);
  else {
    dprintf(DP_SERVER, "TOPIC %s :%s\n", chan->name, par);
    dprintf(idx, "Changing topic...\n");
    putlog(LOG_CMDS, "*", "\00307#(%s) topic#\003 \00306%s\003 %s",
           chan->dname, par, dcc[idx].nick);
  }
}

static void cmd_resetbans(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname = newsplit(&par);

  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "\00307#(%s) resetbans#\003 %s", chan->dname, dcc[idx].nick);
  dprintf(idx, "Resetting bans on %s...\n", chan->dname);
  resetbans(chan);
}

static void cmd_resetexempts(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname = newsplit(&par);

  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "\00307#(%s) resetexempts#\003 %s", chan->dname, dcc[idx].nick);
  dprintf(idx, "Resetting exempts on %s...\n", chan->dname);
  resetexempts(chan);
}

static void cmd_resetinvites(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname = newsplit(&par);

  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "\00307#(%s) resetinvites#\003 %s", chan->dname, dcc[idx].nick);
  dprintf(idx, "Resetting resetinvites on %s...\n", chan->dname);
  resetinvites(chan);
}

static void cmd_reset(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;

  if (par[0]) {
    chan = findchan_by_dname(par);
    if (!chan)
      dprintf(idx, "%s\n", IRC_NOMONITOR);
    else {
      get_user_flagrec(u, &user, par);
      if (!glob_master(user) && !chan_master(user))
        dprintf(idx, "\00304You are not a master on %s.\003\n", chan->dname);
      else if (!channel_active(chan))
        dprintf(idx, "\00304I'm not on %s at the moment!\003\n", chan->dname);
      else {
        putlog(LOG_CMDS, "*", "\00307#reset#\003 \00306%s\003 %s", par, dcc[idx].nick);
        dprintf(idx, "Resetting channel info for %s...\n", chan->dname);
        reset_chan_info(chan, CHAN_RESETALL);
      }
    }
  } else if (!(u->flags & USER_MASTER))
    dprintf(idx, "\00304You are not a bot master.\003\n");
  else {
    putlog(LOG_CMDS, "*", "\00307#reset#\003 \00306all\003 %s", dcc[idx].nick);
    dprintf(idx, "Resetting channel info for all channels...\n");
    for (chan = chanset; chan; chan = chan->next) {
      if (channel_active(chan))
        reset_chan_info(chan, CHAN_RESETALL);
    }
  }
}

static cmd_t irc_dcc[] = {
  {"reset",        "m|m",   (IntFunc) cmd_reset,        NULL},
  {"resetbans",    "o|o",   (IntFunc) cmd_resetbans,    NULL},
  {"resetexempts", "o|o",   (IntFunc) cmd_resetexempts, NULL},
  {"resetinvites", "o|o",   (IntFunc) cmd_resetinvites, NULL},
  {"act",          "o|o",   (IntFunc) cmd_act,          NULL},
  {"channel",      "o|o",   (IntFunc) cmd_channel,      NULL},
  {"op",           "o|o",   (IntFunc) cmd_op,           NULL},
  {"deop",         "o|o",   (IntFunc) cmd_deop,         NULL},
  {"halfop",       "ol|ol", (IntFunc) cmd_halfop,       NULL},
  {"dehalfop",     "ol|ol", (IntFunc) cmd_dehalfop,     NULL},
  {"voice",        "ov|ov", (IntFunc) cmd_voice,        NULL},
  {"devoice",      "ov|ov", (IntFunc) cmd_devoice,      NULL},
  {"invite",       "o|o",   (IntFunc) cmd_invite,       NULL},
  {"kick",         "lo|lo", (IntFunc) cmd_kick,         NULL},
  {"kickban",      "lo|lo", (IntFunc) cmd_kickban,      NULL},
  {"msg",          "o",     (IntFunc) cmd_msg,          NULL},
  {"say",          "o|o",   (IntFunc) cmd_say,          NULL},
  {"topic",        "lo|lo", (IntFunc) cmd_topic,        NULL},
  {NULL,           NULL,    NULL,                        NULL}
};
