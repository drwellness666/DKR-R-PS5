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
names=[]
for line in symbols.splitlines():
    fields=line.split()
    if len(fields)==8 and fields[3]=='FUNC' and fields[6]=='UND':
        name=fields[7]
        if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',name):
            raise SystemExit('Unsupported import name: '+name)
        names.append(name)
names=sorted(set(names))
text='// SPDX-License-Identifier: GPL-3.0-or-later\n#include <stddef.h>\n'
text+='typedef void (*Function)(void);\n'
for index,name in enumerate(names):
    text+=f'extern void import_{index}(void) __asm__("{name}");\n'
text+='static const struct {const char *name; Function address;} imports[]={\n'
text+=''.join(f'{{"{name}",import_{index}}},\n' for index,name in enumerate(names))+'};\n'
text+='''unsigned dkrr_report_missing_imports(void (*emit)(const char *)) {
    unsigned missing=0;
    for(size_t i=0;i<sizeof(imports)/sizeof(imports[0]);++i) {
        Function volatile address=imports[i].address;
        if(!address) {++missing;emit(imports[i].name);}
    }
    return missing;
}
'''
Path(sys.argv[2]).write_text(text)
print('Generated native address audit for',len(names),'function imports; no functions called')
