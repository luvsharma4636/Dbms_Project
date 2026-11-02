
#include <stdio.h>
#include <string.h>

#include "pf.h"
#include "pftypes.h"
#include "slotted.h"

/* PF functions used by this test */
extern int PF_CreateFile(char *fname);
extern int PF_DestroyFile(char *fname);
extern int PF_OpenFileWithPolicy(char *fname, int policy);
extern int PF_AllocPage(int fd, int *pagenum, char **pagebuf);
extern int PF_GetThisPage(int fd, int pagenum, char **pagebuf);
extern int PF_UnfixPage(int fd, int pagenum, int dirty);
extern int PF_CloseFile(int fd);

#define TEST_FILE "slotted_pf_test.dat"

int main()
{
    int fd;
    int pagenum;
    int result;
    int current_slot;
    char *page;
    char record[100];
    uint16_t record_len;
    int slot;
    int found;

    /* Step 1: Create a PF file */
    result = PF_CreateFile(TEST_FILE);

    if (result != PFE_OK) {
        printf("ERROR: Could not create PF file. Error = %d\n", result);
        return 1;
    }

    /* Step 2: Open the PF file */
    fd = PF_OpenFileWithPolicy(TEST_FILE, PF_LRU);

    if (fd < 0) {
        printf("ERROR: Could not open PF file. Error = %d\n", fd);
        PF_DestroyFile(TEST_FILE);
        return 1;
    }

    /* Step 3: Allocate a page */
    result = PF_AllocPage(fd, &pagenum, &page);

    if (result != PFE_OK) {
        printf("ERROR: Could not allocate page. Error = %d\n", result);
        PF_CloseFile(fd);
        PF_DestroyFile(TEST_FILE);
        return 1;
    }

    printf("Allocated PF page: %d\n", pagenum);

    /* Step 4: Initialize it as a slotted page */
    slotted_init_page(page);

    /* Step 5: Insert records */
    slot = slotted_insert_record(
        page, "101;Alice;Computer Science", 26
    );

    if (slot < 0) {
        printf("ERROR: Could not insert Alice\n");
        PF_UnfixPage(fd, pagenum, TRUE);
        PF_CloseFile(fd);
        PF_DestroyFile(TEST_FILE);
        return 1;
    }

    printf("Inserted Alice in slot %d\n", slot);

    slot = slotted_insert_record(
        page, "102;Bob;Mathematics", 19
    );

    if (slot < 0) {
        printf("ERROR: Could not insert Bob\n");
        PF_UnfixPage(fd, pagenum, TRUE);
        PF_CloseFile(fd);
        PF_DestroyFile(TEST_FILE);
        return 1;
    }

    printf("Inserted Bob in slot %d\n", slot);

    /* Step 6: Mark the page dirty and unfix it */
    result = PF_UnfixPage(fd, pagenum, TRUE);

    if (result != PFE_OK) {
        printf("ERROR: Could not unfix page. Error = %d\n", result);
        PF_CloseFile(fd);
        PF_DestroyFile(TEST_FILE);
        return 1;
    }

    printf("Page written and unfixed.\n");

    /* Step 7: Fetch the same page again */
    result = PF_GetThisPage(fd, pagenum, &page);

    if (result != PFE_OK) {
        printf("ERROR: Could not fetch page. Error = %d\n", result);
        PF_CloseFile(fd);
        PF_DestroyFile(TEST_FILE);
        return 1;
    }

    printf("\nReading records from PF page:\n");

    /* Step 8: Scan records from the slotted page */
    current_slot = 0;
    found = 0;

    while (1) {
        result = slotted_get_next_record(
            page,
            &current_slot,
            record,
            sizeof(record) - 1,
            &record_len
        );

        if (result == 0) {
            break;
        }

        if (result < 0) {
            printf("ERROR: Could not read record.\n");
            PF_UnfixPage(fd, pagenum, FALSE);
            PF_CloseFile(fd);
            PF_DestroyFile(TEST_FILE);
            return 1;
        }

        record[record_len] = '\0';
        printf("Slot %d: %s\n", current_slot-1, record);
        found++;
    }

    /* Step 9: Unfix the page after reading */
    PF_UnfixPage(fd, pagenum, FALSE);

    /* Step 10: Verify the records */
    if (found != 2) {
        printf("\nTEST FAILED: Expected 2 records, found %d\n", found);
    } else {
        printf("\nTEST PASSED: Both records survived PF page retrieval.\n");
    }

    /* Step 11: Close and destroy the test file */
    PF_CloseFile(fd);
    PF_DestroyFile(TEST_FILE);

    return (found == 2) ? 0 : 1;
}
