#include <stdio.h>
#include <string.h>
#include "pf.h"
#include "pftypes.h"

int main() {
    int fd;
    int pagenum;
    int error;
    char *pagebuf;

    PF_Init();

    error = PF_CreateFile("dirty_test");
    if(error != PFE_OK) {
        PF_PrintError("Create file");
        return 1;
    }

    fd = PF_OpenFile("dirty_test");
    if(fd < 0) {
        PF_PrintError("Open file");
        return 1;
    }

    error = PF_AllocPage(fd, &pagenum, &pagebuf);
    if(error != PFE_OK) {
        PF_PrintError("Allocate page");
        return 1;
    }

    strcpy(pagebuf, "Hello ToyDB");

    error = PF_MarkDirty(fd, pagenum);
    if(error != PFE_OK) {
        PF_PrintError("Mark dirty");
        return 1;
    }

    printf("After marking dirty:\n");
    PFbufPrint();

    error = PF_UnfixPage(fd, pagenum, FALSE);
    if(error != PFE_OK) {
        PF_PrintError("Unfix page");
        return 1;
    }

    PF_CloseFile(fd);

    fd = PF_OpenFile("dirty_test");
    if(fd < 0) {
        PF_PrintError("Reopen file");
        return 1;
    }

    error = PF_GetThisPage(fd, pagenum, &pagebuf);
    if(error != PFE_OK) {
        PF_PrintError("Read page");
        return 1;
    }

    printf("\nData after reopening: %s\n", pagebuf);

    if(strcmp(pagebuf, "Hello ToyDB") == 0) {
        printf("TEST PASSED: Data persisted correctly\n");
    } else {
        printf("TEST FAILED: Data mismatch\n");
    }

    PF_UnfixPage(fd, pagenum, FALSE);
    PF_CloseFile(fd);
    PF_DestroyFile("dirty_test");

    return 0;
}