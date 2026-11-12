// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Start-up self-test of the console environment. It only measures: every check
// creates and removes its own files below <profile>/selftest, records what the
// system call returned ("ok" = the call succeeded, not "the game is fine") and
// writes <profile>/logs/selftest.json. That turns assumptions about the
// sandbox into observed facts (docs/PS5_FACTS.md) and lets a bug of this kind
// be found by reading one file over FTP.
//
// A check that itself crashes leaves <profile>/logs/selftest.running behind;
// the next start skips the self-test once instead of crashing in a loop. Every
// check writes a journal step first, so a crash names the check.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <sys/resource.h>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "ps5_diagnostics.h"

namespace dkrr::selftest {
struct Result { std::string name; bool ok; std::string detail; };

namespace detail {
inline std::string call(long rc, int error) {
    return "rc=" + std::to_string(rc) + (rc < 0 ? " errno=" + std::to_string(error) : std::string());
}
inline std::string json_text(const std::string &text) {
    std::string out;
    for (const unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 0x20 || c > 0x7e) out += '?';
        else out += char(c);
    }
    return out;
}
inline std::string stat_text(const struct stat &st) {
    char text[160];
    std::snprintf(text, sizeof(text), "uid=%u euid=%u nlink=%u mode=%o size=%lld", unsigned(st.st_uid),
                  unsigned(geteuid()), unsigned(st.st_nlink), unsigned(st.st_mode & 07777), (long long)st.st_size);
    return text;
}
}

// Runs all checks; returns the results (also written to logs/selftest.json).
inline std::vector<Result> run(const std::string &profile, const char *build, const char *app_root = "/app0",
                               const char *data_root = "/data/dkrr") {
    std::vector<Result> results;
    const std::string logs = profile + "/logs", marker = logs + "/selftest.running", folder = profile + "/selftest";
    (void)mkdir(logs.c_str(), 0777);
    if (access(marker.c_str(), F_OK) == 0) {
        // The previous self-test never finished: do not repeat it blindly.
        dkrr_diag_step("selftest", "skipped: previous run did not finish (a check crashed; see the journal steps)");
        if (FILE *file = std::fopen((logs + "/selftest-skipped.txt").c_str(), "w")) {
            std::fputs("The previous self-test did not finish. Its last journal step names the check that crashed.\n", file);
            std::fclose(file);
        }
        (void)unlink(marker.c_str());
        return results;
    }
    if (FILE *file = std::fopen(marker.c_str(), "w")) std::fclose(file);
    auto add = [&](const char *name, bool ok, std::string detail) {
        results.push_back({name, ok, std::move(detail)});
    };
    auto step = [&](const char *name) { dkrr_diag_step("selftest", name); };

    step("identity");
    add("identity", true, std::string("build=") + build + " uid=" + std::to_string(getuid()) + " euid=" +
        std::to_string(geteuid()) + " gid=" + std::to_string(getgid()) + " pid=" + std::to_string(getpid()));

    step("scratch folder");
    errno = 0;
    int rc = mkdir(folder.c_str(), 0700);
    if (rc < 0 && errno == EEXIST) rc = 0;
    add("scratch folder", rc == 0, detail::call(rc, errno));
    if (rc == 0) {
        const std::string file = folder + "/a.bin", moved = folder + "/b.bin";
        step("create write read");
        errno = 0;
        const int fd = open(file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
        add("create file", fd >= 0, detail::call(fd, errno));
        if (fd >= 0) {
            errno = 0;
            const long written = write(fd, "dkrr", 4);
            add("write", written == 4, detail::call(written, errno));
            step("fsync");
            errno = 0;
            rc = fsync(fd);
            add("fsync", rc == 0, detail::call(rc, errno));
            step("stat file");
            struct stat st {};
            errno = 0;
            rc = fstat(fd, &st);
            add("fstat file", rc == 0, rc == 0 ? detail::stat_text(st) : detail::call(rc, errno));
            errno = 0;
            rc = stat(file.c_str(), &st);
            add("stat file", rc == 0, rc == 0 ? detail::stat_text(st) : detail::call(rc, errno));
            errno = 0;
            rc = lstat(file.c_str(), &st);
            add("lstat file", rc == 0, rc == 0 ? detail::stat_text(st) : detail::call(rc, errno));
            step("flock");
            errno = 0;
            rc = flock(fd, LOCK_EX | LOCK_NB);
            add("flock exclusive", rc == 0, detail::call(rc, errno));
            if (rc == 0) {
                // Does a second holder actually conflict? Locks that always succeed give no exclusion.
                const int other = open(file.c_str(), O_RDWR);
                errno = 0;
                const int second = other >= 0 ? flock(other, LOCK_EX | LOCK_NB) : -2;
                add("flock excludes second holder", second < 0 && errno == EWOULDBLOCK, detail::call(second, errno));
                if (other >= 0) close(other);
            }
            step("chmod");
            errno = 0;
            rc = chmod(file.c_str(), 0600);
            add("chmod", rc == 0, detail::call(rc, errno));
            errno = 0;
            rc = fchmod(fd, 0600);
            add("fchmod", rc == 0, detail::call(rc, errno));
            errno = 0;
            rc = fchmodat(AT_FDCWD, file.c_str(), 0600, 0);
            add("fchmodat", rc == 0, detail::call(rc, errno));
            close(fd);
            step("rename");
            errno = 0;
            rc = rename(file.c_str(), moved.c_str());
            add("rename", rc == 0, detail::call(rc, errno));
            (void)unlink(moved.c_str());
            (void)unlink(file.c_str());
        }
        step("stat folder");
        struct stat st {};
        errno = 0;
        rc = stat(folder.c_str(), &st);
        add("stat folder", rc == 0, rc == 0 ? detail::stat_text(st) : detail::call(rc, errno));
    }

    step("proc");
    char link[256] = {};
    errno = 0;
    const long linked = readlink("/proc/self/exe", link, sizeof(link) - 1);
    add("readlink /proc/self/exe", linked >= 0, linked >= 0 ? std::string("target=") + link : detail::call(linked, errno));
    struct stat proc {};
    errno = 0;
    rc = stat("/proc", &proc);
    add("stat /proc", rc == 0, detail::call(rc, errno));

    step("app root");
    struct stat app {};
    errno = 0;
    rc = stat(app_root, &app);
    add("stat app root", rc == 0, rc == 0 ? detail::stat_text(app) : detail::call(rc, errno));
    errno = 0;
    rc = access((std::string(app_root) + "/eboot.bin").c_str(), R_OK);
    add("read app eboot.bin", rc == 0, detail::call(rc, errno));

    step("data root write");
    const std::string probe = std::string(data_root) + "/selftest-probe";
    errno = 0;
    rc = mkdir(probe.c_str(), 0700);
    add("create folder in data root", rc == 0, detail::call(rc, errno));
    if (rc == 0) (void)rmdir(probe.c_str());

    step("temp folder");
    const char *temp = std::getenv("TMPDIR");
    add("TMPDIR set", temp && *temp, temp ? std::string("TMPDIR=") + temp : "unset");
    if (temp && *temp) {
        const std::string lock_folder = std::string(temp) + "/dkrr-install-locks-" + std::to_string(geteuid());
        errno = 0;
        rc = mkdir(lock_folder.c_str(), 0700);
        if (rc < 0 && errno == EEXIST) rc = 0;
        add("create lock folder in TMPDIR", rc == 0, detail::call(rc, errno));
    }

    step("stack sizes");
    {
        // Thread stack size decides whether a large local buffer is safe in a worker thread.
        pthread_attr_t attributes;
        size_t stack = 0;
        errno = 0;
        const int attr = pthread_attr_init(&attributes);
        const int got = attr == 0 ? pthread_attr_getstacksize(&attributes, &stack) : attr;
        if (attr == 0) pthread_attr_destroy(&attributes);
        add("default thread stack size", got == 0 && stack > 0, got == 0 ? "bytes=" + std::to_string(stack) : detail::call(got, errno));
        struct rlimit limit {};
        errno = 0;
        rc = getrlimit(RLIMIT_STACK, &limit);
        add("main stack limit", rc == 0, rc == 0 ? "soft=" + std::to_string((long long)limit.rlim_cur) : detail::call(rc, errno));
    }

    step("journal");
    add("crash journal open", dkrr_diag_journal_ready() != 0, std::string("build=") + dkrr_diag_build());
    (void)rmdir(folder.c_str());

    step("write result");
    std::string json = "{\"schema\":1,\"build\":\"" + detail::json_text(build) + "\",\"time\":" +
                       std::to_string(static_cast<long long>(std::time(nullptr))) + ",\"results\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        json += std::string(i ? "," : "") + "{\"name\":\"" + detail::json_text(results[i].name) + "\",\"ok\":" +
                (results[i].ok ? "true" : "false") + ",\"detail\":\"" + detail::json_text(results[i].detail) + "\"}";
    }
    json += "]}\n";
    const std::string target = logs + "/selftest.json", temporary = target + ".tmp";
    if (FILE *file = std::fopen(temporary.c_str(), "w")) {
        std::fputs(json.c_str(), file);
        std::fflush(file);
        std::fclose(file);
        (void)chmod(temporary.c_str(), 0666);
        if (rename(temporary.c_str(), target.c_str()) != 0) (void)unlink(temporary.c_str());
    }
    (void)unlink(marker.c_str());
    dkrr_diag_step("selftest", "finished");
    return results;
}
}
