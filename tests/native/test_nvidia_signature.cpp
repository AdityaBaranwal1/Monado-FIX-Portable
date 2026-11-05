// T257: verify the production interposer without loading it or requiring an RTX.
#include <stdio.h>
#include "../../runtime/host/nvidia_signature.h"
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    wchar_t path[32768], self[32768];
    char absolute[32768];
    if (!GetFullPathNameA(argv[1], sizeof absolute, absolute, NULL) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, absolute, -1, path, 32768) ||
        !GetModuleFileNameW(NULL, self, 32768)) return 2;
    bool official = xcde_nvidia_signature(path);
    bool unsigned_test = xcde_nvidia_signature(self);
    printf("NVIDIA production signature: %s; unsigned test rejected: %s\n", official ? "PASS" : "FAIL", !unsigned_test ? "PASS" : "FAIL");
    return official && !unsigned_test ? 0 : 1;
}
