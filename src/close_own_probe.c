/* SPDX-License-Identifier: GPL-3.0-or-later
 * Close a manually started diagnostic only if its running title ID matches.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
uint32_t sceLncUtilGetAppIdOfRunningBigApp(void);
int sceSystemServiceGetAppId(const char *);
int sceLncUtilGetAppTitleId(uint32_t, char *);
int sceSystemServiceKillApp(int, int, int, int);
int sceUserServiceInitialize(void *);
int main(void) {
    (void)sceUserServiceInitialize(NULL);
#if defined(DKR_KNOWN_APP_ID)
    // Exact positive ID returned by this session's own launch, still title-checked.
    const uint32_t app = DKR_KNOWN_APP_ID;
#else
    const int known = sceSystemServiceGetAppId("PPSA94677");
    if (known <= 0) { puts("Own diagnostic is not running; nothing closed."); return 0; }
    const uint32_t app = (uint32_t)known;
#endif
    char title[16] = {0};
    if (!app || sceLncUtilGetAppTitleId(app, title) != 0 ||
        memcmp(title, "PPSA94677", 9) != 0) {
        puts("No matching own PPSA94677 diagnostic is running; nothing closed.");
        return 0;
    }
    int rc = sceSystemServiceKillApp((int)app, -1, 0, 0);
    printf("Closed only verified PPSA94677 probe: rc=0x%08x\n", (unsigned)rc);
    return rc != 0;
}
