![ANSI Logo](https://gitlab.com/kyaulabs/hybridcore/raw/master/hybridcore.ans.png "ANSI Logo")  
<a href="irc://irc.kyaulabs.com:+9999/ak!ra">irc://irc.kyaulabs.com:+9999/ak!ra</a>

[![](https://img.shields.io/badge/coded_in-vim-green.svg?logo=vim&logoColor=brightgreen&colorB=brightgreen&longCache=true&style=flat)](https://vim.org) &nbsp; [![](https://img.shields.io/badge/license-GNU_GPLv2-blue.svg?style=flat)](https://gitlab.com/kyaulabs/hybridcore/blob/master/COPYING) &nbsp; [![](https://img.shields.io/badge/tcl-8.5+-C85000.svg?style=flat)](https://www.tcl.tk/)  

Running a BOTNET
------
Before getting started you will need to designate two bots, the hub and the info 
bot. The hub is the bot all other bots will connect to and the info bot is the 
bot with all of the public channel commands.

*NOTE: If you are going to run ZNC and a bot on the same host, I recommend 
running your hub on the same host as your ZNC if possible.*

#### ZNC
Setup and managing of a hybrid(core) botnet requires `DCC SCHAT`. ZNC with 
schat is the recommended setup, after installing copy your LetsEncrypt 
certificate to `znc.pem` and then generate `schat.pem`.

```shell
sudo cat /etc/letsencrypt/live/<example.com>/{privkey,fullchain}.pem | sudo -u znc tee /var/lib/znc/.znc/znc.pem
openssl req -x509 -new -newkey rsa:4096 -sha256 -days 1096 -nodes -out schat.pem -keyout schat.pem
```
Locations are as follows:

```shell
/var/lib/znc/.znc/znc.pem
/var/lib/znc/.znc/users/<user>/networks/<network>/moddata/schat/schat.pem
```

#### Firewall
In order for `DCC SCHAT` to work you will need to modify your firewall to allow 
for data port access from your botaddr's. For `nftables` you can add the 
following modifications, first creating an IP group for all of the bots on the 
botnet. While your at it if you plan to run on a bot on this same server as well 
add allows for hybrid(core) as well.

```nftables
table inet filter {
  set botnet {
    elements = {
      1.2.3.4,		# bot1.domain.com
      2.3.4.5,		# bot2.vhost.com
      3.4.5.6		# bot3.vhost.com
    }
  }
...
  # allow znc and sdcc data ports
  ip saddr @botnet tcp dport { 10000-65535 } accept
  ip saddr @botnet udp dport { 10000-65535 } accept
  tcp dport 6697 accept
  
  # allow hybrid(core)
  ip saddr @botnet tcp dport { 2600-2601,5000-5050 } accept
...
}
```

#### Install
When setting up your botnet setup the hub first, this will simplify many things.

First, start by cloning the hybrid(core) and akira repositories.

```shell
git clone https://gitlab.com/kyaulabs/hybridcore.git
git clone https://gitlab.com/kyaulabs/akira.git
```

Edit `hybridcore/src/hybridcore.h` in order to set the file encryption salt, 
botnet owner and hardcoded hosts.allow for telnet/dcc. If you end up changing 
the `HYBRID_SALT` you will also have to change it inside of `akira/encrypt.tcl`. 
Set `HYBRID_ADMINALLOW` to the IP address of the vhost your ZNC uses.

Compile hybrid(core) and add the necessary scripts from akira.

```shell
cd hybridcore
autoconf; ./configure --enable-tls; make config; make; make install DEST=~/hub; make sslcert DEST=~/hub
cd ../akira
cp akira.tcl ../hub/akira-decrypt.tcl
cp serv-<network>.tcl ../hub/serv.tcl
cp encrypt.tcl ../hub/
```

Enter the bots directory and edit the `hybrid.cf` configuration file. Provided 
this is the hub (as per recommendation) set `admin` to your nick, set `hub` and 
`nick` to the name of the hub bot, `info` to the name of the infobot and 
`altnick` to `<nick>?`. Change the timezone and offset if applicable and then 
add a vhost / listen address if the machine has more than one IP. Finally at 
the bottom of the file load all of the plaintext scripts.

```conf
source akira-decrypt.tcl
source serv.tcl
source encrypt.tcl
```

Load up the bot for the first time using userfile creation and local terminal 
(in case schat fails). After loading it will prompt for the hard-coded botnet 
password (set in `akira.tcl`). The default password is `2600` if you did not 
change it. Once at the partyline, wait until you see it has connected to the 
IRC server successfully.

```shell
./hybridcore -nt -m
2600
```

From your ZNC client message the hub to finalize setup, then set a password.

```irc
/msg hub +moo
/msg hub +pass <password>
```

*NOTE: If you would prefer not to set the password via PRIVMSG you can use the 
hello command (+moo) and then set your password via the console using `.chpass`.*

Once your password is set, access the partyline again and generate a new key 
for the hard-coded botnet response (use the same key for both).

```irc
.tcl putlog [encrypt <key> <key>]
```

It will output the encrypted response to the partyline. Copy this string and 
then open up `akira-decrypt.tcl`

```irc
.encrypt akira-decrypt.tcl
.encrypt serv.tcl
```

Go ahead and save the userfile (so it saves your password change) and then 
shutdown the bot.

```irc
.save
.die
```

By default, `encrypt.tcl` will encrypt the file and replace `.tcl` with 
`-secure.tcl`.  This is the default filename that hybrid(core) uses for 
autoload, however the main botnet script is set in `hybridcore.h` as shown 
above, which defaults to `akira.tcl`. Rename the encrypted script and then 
remove the three `source` lines that were added to `hybrid.cf` above.

```shell
mv akira-decrypt-secure.tcl akira.tcl
rm serv-<network>.tcl akira-decrypt.tcl encrypt.tcl
```

Re-launch with the bot, now with encrypted scripts.

```shell
./hybridcore
```

In order for all bots to pass secauth, they must have precisely the same 
encrypted `akira.tcl`. This means you can not encrypt a new copy for each bot, 
you must copy the first one you encrypt to all bots. This is one of many ways 
the script knows if a bot has been tampered with. Files that end in 
`-secure.tcl` that are also encrypted and auto-loaded are excluded from this 
check.
