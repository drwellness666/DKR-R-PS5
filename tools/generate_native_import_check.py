#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate an address-only runtime audit of the native title's function imports."""
from pathlib import Path
import re
import subprocess
import sys

if len(sys.argv)!=3:
    raise SystemExit('Usage: generate_native_import_check.py LLVM_PIE OUTPUT_C')
symbols=subprocess.run(['readelf','--wide','--dyn-syms',sys.argv[1]],check=True,text=True,capture_output=True).stdout
bindings={}
for line in symbols.splitlines():
    fields=line.split()
    if len(fields)==8 and fields[3]=='FUNC' and fields[6]=='UND':
        name=fields[7]
        if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',name):
            raise SystemExit('Unsupported import name: '+name)
        bindings[name]=fields[4]
names=sorted(bindings)
# Optional journal setup checks these addresses before use. A shared SDK stub
# can promote the ELF binding to GLOBAL, so retain their policy explicitly.
optional_names={'sigaction','sigemptyset','scePadSetMotionSensorState','scePadSetTriggerEffect'}
text='// SPDX-License-Identifier: GPL-3.0-or-later\n#include <stddef.h>\n'
text+='typedef void (*Function)(void);\n'
for index,name in enumerate(names):
    optional=' __attribute__((weak))' if bindings[name]=='WEAK' or name in optional_names else ''
    text+=f'extern void import_{index}(void) __asm__("{name}"){optional};\n'
text+='static const struct {const char *name; Function address; unsigned optional;} imports[]={\n'
text+=''.join(f'{{"{name}",import_{index},{int(bindings[name]=="WEAK" or name in optional_names)}}},\n' for index,name in enumerate(names))+'};\n'
text+='''unsigned dkrr_report_missing_imports(void (*emit)(const char *)) {
    unsigned missing=0;
    for(size_t i=0;i<sizeof(imports)/sizeof(imports[0]);++i) {
        Function volatile address=imports[i].address;
        if(!address && !imports[i].optional) {++missing;emit(imports[i].name);}
    }
    return missing;
}
'''
Path(sys.argv[2]).write_text(text)
print('Generated native address audit for',len(names),'function imports; no functions called')
