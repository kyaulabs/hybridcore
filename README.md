![ANSI Logo](https://gitlab.com/kyaulabs/hybridcore/raw/master/hybridcore.ans.png "ANSI Logo")  
<a href="irc://irc.kyaulabs.com:+9999/ak!ra">irc://irc.kyaulabs.com:+9999/ak!ra</a>

[![](https://img.shields.io/badge/coded_in-vim-green.svg?logo=vim&logoColor=brightgreen&colorB=brightgreen&longCache=true&style=flat)](https://vim.org) &nbsp; [![](https://img.shields.io/badge/license-GNU_GPLv2-blue.svg?style=flat)](https://gitlab.com/kyaulabs/hybridcore/blob/master/COPYING) &nbsp; [![](https://img.shields.io/badge/tcl-8.5+-C85000.svg?style=flat)](https://www.tcl.tk/)  

About
------
Originally started back in the late 1990s, hybrid(core) was a patch for eggdrop 
that introduced numerous changes/improvements for private botpacks and irc 
warfare. While typically it was also paired with either `daCrew.tcl`, 
`phorce.tcl` or `tnt.tcl` it had quite a bit of use outside of those botpacks 
as well. The version that was used here as a basis is believed to be the final 
release, which was for eggdrop-1.1.6-p8.1.

Features
------
Given that not everything that was originally added/changed in the eggdrop-1.1.6 
version of hybrid(core) is still relevant. Listed here are the core features 
that were taken and ported to the newer version.
* Automatic encryption/decryption of the chanfile and userfile
* Hosts.allow for telnet/dcc (also now with automatic encryption/decryption)
* Key used for encryption is compiled into the executable (no cleartext files)
* MSG functions have been removed, all that remain have been prefixed with '+' 
and possibly also renamed
* DH1080 Tcl module was added for blowcrypt (FiSH) support
* Built to be run with [`akira.tcl`](https://gitlab.com/kyaulabs/akira)

Setup
------
If you plan to setup your own net it should first be stated that if you do 
not plan on running [`akira.tcl`](https://gitlab.com/kyaulabs/akira) then you 
are on your own.

#### Config
Open up `src/hybridcore.h` in a text editor. The variables you might want to 
change are all listed below:

```c
/* cbc encryption salt for hybrid(core) and tcl files */
#define HYBRID_SALT "AKiRA!2600"
/* hardcoded bot owner */
#define HYBRID_OWNER "kyau"
/* tcl script (encrypted) to auto-load */
#define HYBRID_TCLSCRIPT "akira.tcl"
/* ip for the admin (telnet hosts.allow) */
#define HYBRID_ADMINALLOW "23.94.70.21"
/* ip range for local communication (telnet hosts.allow) */
#define HYBRID_LOCALALLOW "10.42.1.*"
```

#### Install
In order to install hybrid(core), run the following:

```shell
# autoconf
# ./configure --enable-tls
# make config
# make
# make install
# make sslcert
```

Attribution
------
Everything that made it possible for me to port this to the newer version of 
eggdrop is listed here.
* [DH1080.so TCL Library](https://github.com/orkim/dh1080_tcl)
* [FiSH-irssi](https://github.com/falsovsky/FiSH-irssi)
* [str_replace](https://gist.github.com/amcsi/6068ef6ae59951ed4a9f) function 
used to convert IRC codes to ANSI codes
* [StackOverflow C Question](https://stackoverflow.com/questions/45528533/fgets-function-printing-garbage-at-the-end) 
helped me fix my garbage output when encrypting/decrypting
