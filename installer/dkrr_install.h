/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DKRR_INSTALL_H
#define DKRR_INSTALL_H
#include <stddef.h>
#include <stdint.h>
#define DKRR_ROM_SIZE (12U*1024U*1024U)
#define DKRR_MAX_FILES 128
#define DKRR_PATH 512
struct dkrr_file { char path[160]; unsigned long long size; char sha1[41]; unsigned mode; };
struct dkrr_manifest { char build[80]; unsigned sequence; size_t count; struct dkrr_file files[DKRR_MAX_FILES]; };
struct dkrr_rom { char path[DKRR_PATH]; unsigned revision; };
int dkrr_sha1_fd(int fd,unsigned char digest[20]);
void dkrr_sha1_bytes(const void *,size_t,unsigned char digest[20]);
void dkrr_hex(const unsigned char digest[20],char out[41]);
int dkrr_regular(const char *path,int missing);
int dkrr_mkdir(const char *path,unsigned mode);
int dkrr_write_new(const char *path,const void *bytes,size_t size,unsigned mode);
int dkrr_hash_file(const char *path,char out[41],unsigned long long *size);
int dkrr_sync_parent(const char *path);
int dkrr_manifest_read(const char *path,struct dkrr_manifest *out);
int dkrr_manifest_write(const char *path,const struct dkrr_manifest *value);
int dkrr_manifest_check(const char *app,const struct dkrr_manifest *value,int reject_unknown);
int dkrr_manifest_add(struct dkrr_manifest *,const char *root,const char *relative,unsigned mode);
int dkrr_rom_identify(const char *path,unsigned char **bytes,unsigned *revision);
int dkrr_rom_find(const char *usb,const char *owned,struct dkrr_rom out[2]);
int dkrr_rom_install(const char *owned,const struct dkrr_rom inputs[2]);
int dkrr_register_source(const char *root,const char *app);
int dkrr_audio_prepare(const unsigned char *rom,size_t size,unsigned revision,const char *destination);
#endif
