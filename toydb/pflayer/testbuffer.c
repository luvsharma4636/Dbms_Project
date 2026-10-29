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
    PF_SetBufferSize(3);

    error = PF_CreateFile("buffer_test");
    if(error != PFE_OK) {
        PF_PrintError("Create file");
        return 1;
    }

    fd = PF_OpenFile("buffer_test");
    if(fd < 0) {
        PF_PrintError("Open file");
        return 1;
    }

    for(i=0; i<8; i++) {
        error = PF_AllocPage(fd, &pagenum, &pagebuf);
        if(error != PFE_OK) {
            PF_PrintError("Allocate page");
            return 1;
        }

        printf("Allocated page %d\n", pagenum);

        error = PF_UnfixPage(fd, pagenum, FALSE);
        if(error != PFE_OK) {
            PF_PrintError("Unfix page");
            return 1;
        }
    }

    printf("\nBuffer contents:\n");
    PFbufPrint();

    PF_CloseFile(fd);
    PF_DestroyFile("buffer_test");

    return 0;
}