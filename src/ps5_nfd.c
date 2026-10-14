/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native preview uses a configured local ROM, not desktop OS file dialogs.
 */
#include "nfd.h"
#include <stdlib.h>
nfdresult_t NFD_Init(void) { return NFD_OKAY; }
void NFD_Quit(void) {}
const char *NFD_GetError(void) { return "A desktop file dialog is unavailable in the PS5 preview."; }
void NFD_ClearError(void) {}
void NFD_FreePathU8(nfdu8char_t *path) { free(path); }
nfdresult_t NFD_OpenDialogU8(nfdu8char_t **out, const nfdu8filteritem_t *filters,
    nfdfiltersize_t count, const nfdu8char_t *initial) {
    (void)filters;(void)count;(void)initial;if(out)*out=NULL;return NFD_CANCEL;
}
nfdresult_t NFD_SaveDialogU8(nfdu8char_t **out, const nfdu8filteritem_t *filters,
    nfdfiltersize_t count, const nfdu8char_t *initial, const nfdu8char_t *name) {
    (void)name;return NFD_OpenDialogU8(out,filters,count,initial);
}
nfdresult_t NFD_PickFolderU8(nfdu8char_t **out, const nfdu8char_t *initial) {
    return NFD_OpenDialogU8(out,NULL,0,initial);
}
