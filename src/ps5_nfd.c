/* SPDX-License-Identifier: GPL-3.0-or-later
 * NFD ABI over the in-game PS5 picker. Called only by the dialog worker.
 */
#include "nfd.h"
#include <stdlib.h>
#include <string.h>
extern int dkrr_ps5_choose_file(int, char **, const char *, const char *);
extern const char *dkrr_ps5_file_picker_error(void);
nfdresult_t NFD_Init(void) { return NFD_OKAY; }
void NFD_Quit(void) {}
const char *NFD_GetError(void) { return dkrr_ps5_file_picker_error(); }
void NFD_ClearError(void) {}
void NFD_FreePathU8(nfdu8char_t *path) { free(path); }
static nfdresult_t choose(int mode, nfdu8char_t **output,
                          const nfdu8filteritem_t *filters,
                          nfdfiltersize_t count, const char *name) {
  char extensions[257] = {0};
  size_t used = 0;
  if (output)
    *output = NULL;
  if (count > 16 || (count && !filters))
    return NFD_ERROR;
  for (nfdfiltersize_t i = 0; i < count; ++i) {
    if (!filters[i].spec)
      return NFD_ERROR;
    const size_t length = strnlen(filters[i].spec, sizeof(extensions));
    if (length == 0 || length + used + (used ? 1 : 0) >= sizeof(extensions))
      return NFD_ERROR;
    if (used)
      extensions[used++] = ',';
    memcpy(extensions + used, filters[i].spec, length);
    used += length;
    extensions[used] = 0;
  }
  return (nfdresult_t)dkrr_ps5_choose_file(mode, output, extensions, name);
}
nfdresult_t NFD_OpenDialogU8(nfdu8char_t **out,
                             const nfdu8filteritem_t *filters,
                             nfdfiltersize_t count,
                             const nfdu8char_t *initial) {
  (void)initial;
  return choose(0, out, filters, count, NULL);
}
nfdresult_t NFD_SaveDialogU8(nfdu8char_t **out,
                             const nfdu8filteritem_t *filters,
                             nfdfiltersize_t count, const nfdu8char_t *initial,
                             const nfdu8char_t *name) {
  (void)initial;
  return choose(1, out, filters, count, name);
}
nfdresult_t NFD_PickFolderU8(nfdu8char_t **out, const nfdu8char_t *initial) {
  (void)initial;
  return choose(2, out, NULL, 0, NULL);
}
