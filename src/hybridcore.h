/*
 * $Arch: hybridcore.h,v 1.005 2018/12/31 02:52:45 kyau Exp $
 *
 * ▄▄ ▄ ▄▄ ▄ ▄▄▄▄ ▄▄▄▄ ▄▄ ▄▄▄   ▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄
 * ██ █ ██ █ ██ █ ██ █ ██ ██ █ ██  ██ █ ██ █ ██ █ ██ ▀  ██
 * ██▄█ ██▄█ ██▄▀ ██▄▀ ██ ██ █ ██  ██   ██ █ ██▄▀ ██▀   ██
 * ██ █ ▄▄ █ ██ █ ██ █ ██ ██ █ ██  ██ █ ██ █ ██ █ ██ █  ██
 * ▀▀ ▀ ▀▀▀▀ ▀▀▀▀ ▀▀ ▀ ▀▀ ▀▀▀▀ ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀ ▀ ▀▀▀▀ ▀▀▀
 *
 * src/hybridcore.h - hybrid(core)
 * Copyright (C) 2019 KYAU Labs (https://kyaulabs.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* minimum password length */
#define HYBRID_PASSWDLEN 7
/* secure file modes */
#define HYBRID_MODE 0600
#define HYBRID_MODEX 0700
/* cbc encryption salt for hybrid(core) and tcl files */
#define HYBRID_SALT "AKiRA!2600"
/* hardcoded bot owner */
#define HYBRID_OWNER "kyau"
/* tcl script (encrypted) to auto-load */
#define HYBRID_TCLSCRIPT "akira.tcl"
/* ip for the admin (telnet hosts.allow) */
#define HYBRID_ADMINALLOW "167.114.153.75"
/* ip range for local communication (telnet hosts.allow) */
#define HYBRID_LOCALALLOW "10.0.42.*"
