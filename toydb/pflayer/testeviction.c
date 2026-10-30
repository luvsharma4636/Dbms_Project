
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"

int main() {
    int fd;
    int pagenum;
    int error;
    int i;
    char *pagebuf;

    PF_Init();
    PF_SetBufferSize(2);

    error = PF_CreateFile("eviction_test");
    if(error != PFE_OK) {
        PF_PrintError("Create file");
        return 1;
    }

    fd = PF_OpenFileWithPolicy("eviction_test", PF_LRU);
    if(fd < 0) {
        PF_PrintError("Open file");
        return 1;
    }

    // Create three pages
    for(i=0; i<3; i++) {
        error = PF_AllocPage(fd, &pagenum, &pagebuf);
        if(error != PFE_OK) {
            PF_PrintError("Allocate page");
            return 1;
        }

        error = PF_UnfixPage(fd, pagenum, FALSE);
        if(error != PFE_OK) {
            PF_PrintError("Unfix page");
            return 1;
        }
    }

    PF_CloseFile(fd);

    fd = PF_OpenFileWithPolicy("eviction_test", PF_LRU);
    if(fd < 0) {
        PF_PrintError("Reopen file");
        return 1;
    }

    PF_ResetStats();

    // Load Page 0 and modify it
    error = PF_GetThisPage(fd, 0, &pagebuf);
    if(error != PFE_OK) {
        PF_PrintError("Read Page 0");
        return 1;
    }

    pagebuf[0] = 'X';

    error = PF_MarkDirty(fd, 0);
    if(error != PFE_OK) {
        PF_PrintError("Mark dirty");
        return 1;
    }

    error = PF_UnfixPage(fd, 0, FALSE);
    if(error != PFE_OK) {
        PF_PrintError("Unfix Page 0");
        return 1;
    }

    // Load Page 1
    error = PF_GetThisPage(fd, 1, &pagebuf);
    if(error != PFE_OK) {
        PF_PrintError("Read Page 1");
        return 1;
    }

    PF_UnfixPage(fd, 1, FALSE);

    // Page 0 is now the LRU page.
    // Loading Page 2 should evict Page 0.
    error = PF_GetThisPage(fd, 2, &pagebuf);
    if(error != PFE_OK) {
        PF_PrintError("Read Page 2");
        return 1;
    }

    PF_UnfixPage(fd, 2, FALSE);

    printf("\nBuffer after eviction:\n");
    PFbufPrint();

    printf("\nStatistics before closing:\n");
    PF_PrintStats();

    if(PFstats.logicalReads == 3 &&
       PFstats.logicalWrites == 1 &&
       PFstats.physicalReads == 3 &&
       PFstats.physicalWrites == 1 &&
       PFstats.bufferHits == 0 &&
       PFstats.bufferMisses == 3) {
        printf("\nEVICTION TEST PASSED\n");
    } else {
        printf("\nEVICTION TEST FAILED\n");
        return 1;
    }

    PF_CloseFile(fd);

    // Reopen and verify the modified data persisted
    fd = PF_OpenFile("eviction_test");

    error = PF_GetThisPage(fd, 0, &pagebuf);
    if(error != PFE_OK) {
        PF_PrintError("Verify Page 0");
        return 1;
    }

    printf("Data after reopening: %c\n", pagebuf[0]);

    if(pagebuf[0] == 'X') {
        printf("DATA PERSISTENCE TEST PASSED\n");
    } else {
        printf("DATA PERSISTENCE TEST FAILED\n");
        return 1;
    }

    PF_UnfixPage(fd, 0, FALSE);
    PF_CloseFile(fd);
    PF_DestroyFile("eviction_test");

    return 0;
}
