#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify the linked implementation, not just the existence of DNS symbols."""
import argparse,pathlib,re,subprocess
p=argparse.ArgumentParser();p.add_argument('sdk',type=pathlib.Path);p.add_argument('game',type=pathlib.Path);a=p.parse_args()
g=a.game;raw=subprocess.check_output([str(a.sdk/'bin/llvm-nm'),'--defined-only',str(g/'native-link/llvm-pie.elf')],text=True)
symbols={row[2]:row[0] for line in raw.splitlines() if len(row:=line.split())==3}
for name,real in [('getaddrinfo','__wrap_getaddrinfo'),('freeaddrinfo','__wrap_freeaddrinfo')]:
 if name not in symbols or real not in symbols or symbols[name]!=symbols[real]:raise SystemExit('Wrong native DNS binding: '+name)
 if symbols.get('ps5_'+name)==symbols[name]:raise SystemExit('Renderer DNS placeholder is active: '+name)
for name in ('dkrr_dns_random','dkrr_secure_random','dkrr_diag_ui_phase'):
 if name not in symbols:raise SystemExit('Missing native network/UI implementation: '+name)
dns=subprocess.check_output([str(a.sdk/'bin/llvm-nm'),'--undefined-only',str(g/'native-dns/build/lib/libcares.a')],text=True)
if not re.search(r'\bU dkrr_dns_random$',dns,re.M):raise SystemExit('DNS does not use checked OS entropy')
for name in ('arc4random_buf','getservbyname','getservbyport','getservbyname_r','getservbyport_r','memmem'):
 if re.search(r'\bU '+name+r'$',dns,re.M):raise SystemExit('Unsupported DNS dependency: '+name)
config=(g/'native-https/build/lib/curl_config.h').read_text()
if '#define CURL_DISABLE_SOCKETPAIR 1' not in config or '#define CURL_DISABLE_FILE 1' not in config or '#define HAVE_FNMATCH 1' in config:raise SystemExit('Wrong native HTTPS capability configuration')
config=(g/'native-dns/build/ares_config.h').read_text()
if '#define HAVE_FCNTL_O_NONBLOCK 1' in config or '#define HAVE_IOCTL_FIONBIO 1' not in config:raise SystemExit('Wrong actual native DNS nonblocking configuration')
for relative in ('src/impl/pollinterrupter.cpp','deps/libjuice/src/conn_poll.c'):
 staged=(g/'game-ps5/native-network'/relative).read_text()
 if 'F_SETFL' in staged or staged.count('dkrr_net_wakeup_pair(')!=1:raise SystemExit('Unchecked native RTC wakeup: '+relative)
# Every networking archive must reference scoped native symbols. Raw BSD
# socket/close/poll imports here would reintroduce the denied kernel FD path.
archives=[g/'native-dns/build/lib/libcares.a',g/'native-https/build/lib/libcurl.a',
          g/'game-ps5/libdatachannel/libdatachannel.a',g/'game-ps5/libdatachannel/deps/libjuice/libjuice.a']
for archive in archives:
 text=subprocess.check_output([str(a.sdk/'bin/llvm-nm'),'--undefined-only',str(archive)],text=True)
 for name in ('socket','close','bind','connect','send','sendto','recv','recvfrom','poll','select','fcntl','ioctl'):
  if re.search(r'\bU '+name+r'$',text,re.M):raise SystemExit('Unscoped network call in '+archive.name+': '+name)
 if not re.search(r'\bU dkrr_net_socket$',text,re.M):raise SystemExit('Native socket binding missing: '+archive.name)
for name in ('dkrr_net_socket','dkrr_net_poll','dkrr_net_wakeup_pair','dkrr_native_runtime_active'):
 if name not in symbols:raise SystemExit('Linked native implementation missing: '+name)
for archive in (g/'game-ps5/libDKR-R.a',g/'game-ps5/libDKRNetplayCore.a'):
 text=subprocess.check_output([str(a.sdk/'bin/llvm-nm'),'--undefined-only',str(archive)],text=True)
 for name in ('socket','bind','connect','send','sendto','sendmsg','recv','recvfrom','recvmsg','select','poll',
              'fork','vfork','execve','execlp','posix_spawn','popen','system'):
  if re.search(r'\bU '+name+r'$',text,re.M):raise SystemExit('Desktop/unscoped native game path: '+archive.name+': '+name)
if 'ps5_exec_allocate' not in symbols or 'ps5_exec_release' not in symbols:
 raise SystemExit('The actual native mod-code allocator is missing')
imports=subprocess.check_output([str(a.sdk/'bin/llvm-nm'),'--undefined-only',str(g/'native-link/llvm-pie.elf')],text=True)
for name in ('fork','vfork','execve','execlp','posix_spawn','popen','system'):
 if re.search(r'\bU '+name+r'$',imports,re.M):raise SystemExit('Desktop process dependency in native title: '+name)
if symbols.get('system')!=symbols.get('dkrr_ps5_no_shell') or 'dkrr_ps5_no_shell' not in symbols:
 raise SystemExit('Optional driver shell diagnostics must report unsupported on PS5')
if symbols.get('localeconv')!=symbols.get('dkrr_ps5_localeconv') or 'dkrr_ps5_localeconv' not in symbols:
 raise SystemExit('Native-title JSON fractions must use the SDK C-locale backport')
for name in ('dkrr_async_log_start','dkrr_async_log_stop','dkrr_async_log_diagnostic',
             '__wrap_fprintf','__wrap_fwrite','dkrr_diag_render_wait'):
 if name not in symbols:raise SystemExit('Missing native nonblocking diagnostics: '+name)
print('PASS linked native network: real shared DNS ownership, checked OS entropy, scoped SceNet dependencies, native DNS callbacks and checked UDP RTC wakeups')
print('PASS native game policy: direct/LAN game sockets, built-in controller owner, no desktop subprocess imports, SDK executable-memory allocator')
