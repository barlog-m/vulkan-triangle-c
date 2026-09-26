#include "utils.h"

#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "constants.h"
#include "log.h"

char* get_base_path(void)
{
    char exe[MAX_PATH];

#ifdef _WIN32
    GetModuleFileNameA(NULL, exe, sizeof(exe));
    char* last = strrchr(exe, '\\');
    if (last) {
        *(last + 1) = '\0';
    }

#elifdef __APPLE__
    uint32_t size = sizeof(exe);
    _NSGetExecutablePath(exe, &size);
    char* last = strrchr(exe, '/');
    if (last) {
        *(last + 1) = '\0';
    }

#else
    const ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len == -1) {
        return nullptr;
    }
    exe[len] = '\0';
    char* last = strrchr(exe, '/');
    if (last) {
        *(last + 1) = '\0';
    }
#endif

    return strdup(exe);
}

uint32_t* read_binary_file(const char* filename, size_t* size)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        LOG_ERRNO_ERROR(filename);
        return nullptr;
    }

    struct stat file_stat;
    if (fstat(fileno(file), &file_stat) != 0) {
        LOG_ERRNO_ERROR("fstat");
        fclose(file);
        return nullptr;
    }

    if (file_stat.st_size <= 0 || (size_t)file_stat.st_size > SIZE_MAX) {
        LOG_ERRNO_ERROR("file size out of bounds");
        fclose(file);
        return nullptr;
    }

    *size = (size_t)file_stat.st_size;
    uint32_t* buffer = malloc(*size);
    if (!buffer) {
        LOG_ERRNO_ERROR("malloc buffer");
        fclose(file);
        return nullptr;
    }

    const size_t read_size = fread(buffer, 1, *size, file);
    fclose(file);

    if (read_size != *size) {
        LOG_ERRNO_ERROR("fread");
        free(buffer);
        return nullptr;
    }

    return buffer;
}
