![ANSI Logo](https://gitlab.com/kyaulabs/hybridcore/raw/master/hybridcore.ans.png "ANSI Logo")  
<a href="irc://irc.efnet.org:+9999/kyaulabs">irc://irc.efnet.org:+9999/kyaulabs</a>

[![](https://img.shields.io/badge/coded_in-vim-green.svg?logo=vim&logoColor=brightgreen&colorB=brightgreen&longCache=true&style=flat)](https://vim.org) &nbsp; [![](https://img.shields.io/badge/license-GNU_GPLv2-blue.svg?style=flat)](https://gitlab.com/kyaulabs/hybridcore/blob/master/COPYING) &nbsp; [![](https://img.shields.io/badge/tcl-8.5+-C85000.svg?style=flat)](https://www.tcl.tk/)  

### About
Originally started back in the late 1990s, hybrid(core) was a patch for eggdrop that introduced numerous changes/improvements for private botpacks and irc warfare. While typically it was also paired with either `daCrew.tcl`, `phorce.tcl` or `tnt.tcl` it had quite a bit of use outside of those botpacks as well. The version that was used here as a basis is believed to be the final release, which was for eggdrop-1.1.6-p8.1.

### Features
Given that not everything that was originally added/changed in the eggdrop-1.1.6 version of hybrid(core) is still relevant. Listed here are the core features that were taken and ported to the newer version.
* Automatic encryption/decryption of the chanfile and userfile
* Hosts.allow for telnet/dcc (also now with automatic encryption/decryption)
* Key used for encryption is compiled into the executable (no cleartext files)
* MSG functions have been removed, all that remain have been prefixed with '+' and possibly also renamed
* DH1080 Tcl module was added for blowcrypt (FiSH) support

### Install
In order to install hybrid(core), run the following:

```shell
# autoconf
# ./configure --enable-tls
# make config
# make
# make install
# make sslcert
```

### Attribution
Everything that made it possible for me to port this to the newer version of eggdrop is listed here.
* [DH1080.so TCL Library](https://github.com/orkim/dh1080_tcl)
* [FiSH-irssi](https://github.com/falsovsky/FiSH-irssi)
* [str_replace](https://gist.github.com/amcsi/6068ef6ae59951ed4a9f) function used to convert IRC codes to ANSI codes
* [StackOverflow C Question](https://stackoverflow.com/questions/45528533/fgets-function-printing-garbage-at-the-end) helped me fix my garbage output when encrypting/decrypting
