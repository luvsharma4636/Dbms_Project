
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"

int main() {
    int fd;
    int pagenum;
    int error;
    int i;
    int sequence[] = {0, 1, 0, 2};
    char *pagebuf;

    PF_Init();
    PF_SetBufferSize(2);

    error = PF_CreateFile("stats_test");
    if(error != PFE_OK) {
        PF_PrintError("Create file");
        return 1;
    }

    fd = PF_OpenFile("stats_test");
    if(fd < 0) {
        PF_PrintError("Open file");
        return 1;
    }

    // Create 3 pages for testing
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

    // Reopen the file and reset statistics
    fd = PF_OpenFileWithPolicy("stats_test", PF_LRU);
    if(fd < 0) {
        PF_PrintError("Reopen file");
        return 1;
    }

    PF_ResetStats();

    // Read pages in a predefined sequence
    for(i=0; i<4; i++) {
        pagenum = sequence[i];

        error = PF_GetThisPage(fd, pagenum, &pagebuf);
        if(error != PFE_OK) {
            PF_PrintError("Get page");
            return 1;
        }

        printf("Accessed page %d\n", pagenum);

        // Modify page 0 on its first access
        if(i == 0) {
            pagebuf[0] = 'A';

            error = PF_MarkDirty(fd, pagenum);
            if(error != PFE_OK) {
                PF_PrintError("Mark dirty");
                return 1;
            }
        }

        error = PF_UnfixPage(fd, pagenum, FALSE);
        if(error != PFE_OK) {
            PF_PrintError("Unfix page");
            return 1;
        }
    }

    // Closing flushes remaining dirty pages
    error = PF_CloseFile(fd);
    if(error != PFE_OK) {
        PF_PrintError("Close file");
        return 1;
    }

    PF_PrintStats();

    if(PFstats.logicalReads == 4 &&
       PFstats.logicalWrites == 1 &&
       PFstats.physicalReads == 3 &&
       PFstats.physicalWrites == 1 &&
       PFstats.bufferHits == 1 &&
       PFstats.bufferMisses == 3) {
        printf("\nTEST PASSED\n");
    } else {
        printf("\nTEST FAILED\n");
    }

    PF_DestroyFile("stats_test");

    return 0;
}
