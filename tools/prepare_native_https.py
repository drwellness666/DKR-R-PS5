#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the pinned static HTTPS client against the game's existing TLS ABI."""
import argparse,hashlib,json,pathlib,subprocess
p=argparse.ArgumentParser();p.add_argument('game',type=pathlib.Path);p.add_argument('sdk',type=pathlib.Path);p.add_argument('curl_source',type=pathlib.Path);p.add_argument('ca_bundle',type=pathlib.Path);p.add_argument('--cmake',default='cmake');a=p.parse_args()
game=a.game.resolve();sdk=a.sdk.resolve();source=a.curl_source.resolve();ca=a.ca_bundle.resolve();stage=game/'native-https';stage.mkdir(exist_ok=True)
raw=ca.read_bytes()
if hashlib.sha256(raw).hexdigest()!='796f1389bfea13c0ed78f663b765baaff54a508e5483786bd6ece73c5e3e3cbe':raise SystemExit('Unexpected public CA bundle')
if (source/'include/curl/curlver.h').read_text().find('#define LIBCURL_VERSION "8.23.0"')<0:raise SystemExit('Unexpected curl source version')
(stage/'ps5_tls_roots.generated.h').write_text('// Mozilla CA bundle from curl.se; see HTTPS_SOURCES.json.\nstatic constexpr char dkrr_tls_roots[]=R"DKRR_CA('+raw.decode() +')DKRR_CA";\n')
extern=game/'source/DKR-R/extern';library=game/'game-ps5/mbedtls/library';config=game/'source/DKR-R/runtime-recomp/src/game/netplay/mbedtls_dkrr_user_config.h'
args=[a.cmake,'-S',str(source),'-B',str(stage/'build'),'-G','Unix Makefiles','-DCMAKE_TOOLCHAIN_FILE='+str(sdk/'toolchain/prospero.cmake'),'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY','-DBUILD_CURL_EXE=OFF','-DBUILD_TESTING=OFF','-DBUILD_SHARED_LIBS=OFF','-DBUILD_STATIC_LIBS=ON','-DCURL_USE_MBEDTLS=ON','-DCURL_USE_OPENSSL=OFF','-DCURL_USE_LIBPSL=OFF','-DCURL_DISABLE_INSTALL=ON','-DCURL_DISABLE_LDAP=ON','-DCURL_DISABLE_LDAPS=ON','-DCURL_ZLIB=OFF','-DCURL_BROTLI=OFF','-DCURL_ZSTD=OFF','-DUSE_NGHTTP2=OFF','-DUSE_LIBIDN2=OFF','-DCURL_USE_LIBSSH2=OFF','-DENABLE_THREADED_RESOLVER=ON','-DCURL_USE_PKGCONFIG=OFF','-DHTTP_ONLY=ON','-DHAVE_PIPE2=0','-DHAVE_FCNTL_O_NONBLOCK=0','-DHAVE_IOCTL_FIONBIO=1','-DCURL_CA_BUNDLE=none','-DCURL_CA_PATH=none','-DMBEDTLS_INCLUDE_DIR='+str(extern/'mbedtls/include'),'-DMBEDTLS_LIBRARY='+str(library/'libmbedtls.a'),'-DMBEDX509_LIBRARY='+str(library/'libmbedx509.a'),'-DMBEDCRYPTO_LIBRARY='+str(library/'libmbedcrypto.a'),'-DCMAKE_C_FLAGS=-DMBEDTLS_USER_CONFIG_FILE=\\"'+str(config)+'\\"']
# The public SDK has no if_nametoindex import. Curl retains numeric IPv6
# scope IDs and ordinary IPv4/IPv6 connections through its existing fallback.
args.append('-DHAVE_IF_NAMETOINDEX=0')
# The easy API uses its own bounded poll loop. Native titles do not need
# cross-thread curl_multi_wakeup; avoid treating denied wakeup pipes as OOM.
args.extend(['-DCURL_DISABLE_SOCKETPAIR=ON','-DHAVE_FNMATCH=0'])
args.extend(['-DCURL_DISABLE_FILE=ON','-DHAVE_SENDMMSG=0'])
bridge=pathlib.Path(__file__).resolve().parents[1]/'src/ps5_net_alias.h'
args.extend(['-DCMAKE_PROJECT_INCLUDE='+str(bridge.parent.parent/'tools/NativeSocketTarget.cmake'),'-DDKRR_NATIVE_NET_HEADER='+str(bridge)])
subprocess.run(args,check=True)
subprocess.run([a.cmake,'--build',str(stage/'build'),'--target','libcurl_static','-j3'],check=True)
(stage/'HTTPS-RECEIPT.json').write_text(json.dumps(dict(curl_version='8.23.0',ca_sha256=hashlib.sha256(raw).hexdigest(),tls_config_sha256=hashlib.sha256(config.read_bytes()).hexdigest(),archive_sha256=hashlib.sha256((stage/'build/lib/libcurl.a').read_bytes()).hexdigest(),tls='existing-game-mbedtls',verification='hostname and public CA chain required; HTTPS only; redirects owned by caller'),indent=2)+'\n')
print('PASS static native HTTPS library; console transport remains to be tested')
