#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Sign a ROM-free native package or legacy ELF with an external Ed25519 key."""
import argparse,hashlib,pathlib,re,struct,subprocess,tempfile

def manifest(payload,sequence,tag):
    if not 1<=sequence<=2147483647 or not re.fullmatch(r'ps5-[a-z0-9.-]{6,59}',tag) or '..' in tag:
        raise ValueError('Invalid release tag or sequence')
    with payload.open('rb') as stream:
        header=stream.read(20)
        native=header[:8]==b'DKRRPK2\n'
        if not native and (header[:7]!=b'\x7fELF\x02\x01\x01' or header[18:20]!=b'\x3e\x00'):raise ValueError('Expected a native data package or legacy x86-64 ELF')
        if native and (len(header)<16 or struct.unpack_from('<II',header,8)!=(sequence,37)):raise ValueError('Native package sequence or file count differs')
        stream.seek(0);digest=hashlib.file_digest(stream,'sha256').hexdigest()
    size=payload.stat().st_size
    if not 64<=size<=128*1024*1024:raise ValueError('Unexpected update size')
    version,name=(2,'DKRR-Update.dkru') if native else (1,'DKRR-Network-Update.elf')
    return f'DKRR-UPDATE-{version}\nPPSA94678\n{sequence}\n{tag}\n{size}\n{digest}\n{name}\n'.encode()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('payload',type=pathlib.Path);p.add_argument('--key',required=True,type=pathlib.Path);p.add_argument('--sequence',required=True,type=int);p.add_argument('--tag',required=True);p.add_argument('--output',required=True,type=pathlib.Path);a=p.parse_args()
    if a.output.exists() or a.output.is_symlink():p.error('Use a new output file')
    if a.key.is_symlink() or not a.key.is_file() or a.key.stat().st_mode&0o077:p.error('Signing key must be an ordinary private file (mode 0600)')
    try:body=manifest(a.payload,a.sequence,a.tag)
    except (ValueError,OSError) as error:p.error(str(error))
    with tempfile.TemporaryDirectory(prefix='dkrr-update-sign-') as d:
        path=pathlib.Path(d)/'manifest';path.write_bytes(body)
        signature=subprocess.check_output(['openssl','pkeyutl','-sign','-rawin','-inkey',str(a.key),'-in',str(path)])
        if len(signature)!=64:p.error('Ed25519 signature required')
        public=subprocess.check_output(['openssl','pkey','-in',str(a.key),'-pubout','-outform','DER'])
        header=(pathlib.Path(__file__).resolve().parents[1]/'src/ps5_update_channel.h').read_text()
        trusted=bytes(int(v,16) for v in re.findall(r'0x([0-9a-f]{2})',header))
        if len(public)!=44 or public[12:]!=trusted:p.error('Signing key differs from the native client verification key')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('xb') as out:out.write(body+signature.hex().encode()+b'\n')
    print('Signed update channel:',a.output)
if __name__=='__main__':main()
