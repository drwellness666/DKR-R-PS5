/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <errno.h>
/* Mesa's optional assembly dump can ask an external objdump to print debug
 * text. PS5 has no command processor. Report that real capability accurately;
 * shader compilation and all game/network/import/update operations are native
 * in-process paths and never depend on this diagnostic shell operation. */
int dkrr_ps5_no_shell(const char *command) {
    if(!command)return 0;
    errno=ENOSYS;return -1;
}
