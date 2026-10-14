/*
 * PS5 Platform - working-directory traversal, adapted for DKR-R.
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Derived from PS5_PayloadSDK b5efad528e0ac1b8289f39d72a0e890ff68ff0a6,
 * platform/src/directory.c, SHA256:
 * b092b6a628b9a9e9852da6b0995d848e871a7d7f91602b4da7b1a7e9277deed9.
 * Integration: standard names bound to the pinned platform's directory layer;
 * PATH_MAX buffers; preserve stat errors. No chdir or process privileges.
 */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *
getcwd(char *buffer, size_t size)
{
   char path[PATH_MAX]; /* the answer, built from its end */
   size_t start = sizeof(path) - 1;
   path[start] = '\0';
   char up[PATH_MAX] = ".";
   struct stat here, parent;
   if (buffer && size == 0) {
      errno = EINVAL;
      return NULL;
   }
   if (stat(".", &here) != 0)
      return NULL;
   for (;;) {
      char above[PATH_MAX];
      if ((size_t)snprintf(above, sizeof(above), "%s/..", up) >= sizeof(above)) {
         errno = ENAMETOOLONG;
         return NULL;
      }
      if (stat(above, &parent) != 0)
         return NULL;
      if (parent.st_dev == here.st_dev && parent.st_ino == here.st_ino)
         break; /* the root is its own parent */
      DIR *const directory = opendir(above);
      if (!directory)
         return NULL;
      bool found = false;
      for (struct dirent *entry; !found && (entry = readdir(directory));) {
         if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
         char candidate[PATH_MAX];
         struct stat status;
         if ((size_t)snprintf(candidate, sizeof(candidate), "%s/%s", above, entry->d_name) <
                sizeof(candidate) &&
             lstat(candidate, &status) == 0 && status.st_dev == here.st_dev &&
             status.st_ino == here.st_ino) {
            const size_t length = strlen(entry->d_name);
            if (length + 1 > start) {
               closedir(directory);
               errno = ENAMETOOLONG;
               return NULL;
            }
            start -= length;
            memcpy(path + start, entry->d_name, length);
            path[--start] = '/';
            found = true;
         }
      }
      closedir(directory);
      if (!found) {
         errno = ENOENT;
         return NULL;
      }
      here = parent;
      memcpy(up, above, strlen(above) + 1);
   }
   if (path[start] == '\0')
      path[--start] = '/';
   const size_t length = sizeof(path) - 1 - start;
   if (!buffer) {
      buffer = malloc(size > length ? size : length + 1);
      if (!buffer)
         return NULL;
   } else if (size <= length) {
      errno = ERANGE;
      return NULL;
   }
   memcpy(buffer, path + start, length + 1);
   return buffer;
}

