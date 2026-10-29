#include <stdio.h>
#include "pf.h"
#include "pftypes.h"

void testPolicy(int policy, char *filename) {
    int fd;
    int pagenum;
    int error;
    int i;
    char *pagebuf;

    PF_Init();
    PF_SetBufferSize(3);

    error = PF_CreateFile(filename);
    if(error != PFE_OK) {
        PF_PrintError("Create file");
        return;
    }

    fd = PF_OpenFileWithPolicy(filename, policy);
    if(fd < 0) {
        PF_PrintError("Open file");
        return;
    }

    for(i=0; i<4; i++) {
        error = PF_AllocPage(fd, &pagenum, &pagebuf);
        if(error != PFE_OK) {
            PF_PrintError("Allocate page");
            return;
        }

        PF_UnfixPage(fd, pagenum, FALSE);
    }

    PF_CloseFile(fd);

    fd = PF_OpenFileWithPolicy(filename, policy);

    int sequence[] = {0, 1, 2, 0, 3};

    for(i=0; i<5; i++) {
        error = PF_GetThisPage(fd, sequence[i], &pagebuf);

        if(error != PFE_OK) {
            PF_PrintError("Get page");
            return;
        }

        PF_UnfixPage(fd, sequence[i], FALSE);
    }

    printf("\nPolicy: %s\n", policy == PF_LRU ? "LRU" : "MRU");
    PFbufPrint();

    PF_CloseFile(fd);
    PF_DestroyFile(filename);
}

int main() {
    testPolicy(PF_LRU, "test_lru");
    testPolicy(PF_MRU, "test_mru");

    return 0;
}