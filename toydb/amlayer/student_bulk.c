#include <stdio.h>
#include <string.h>
#include "../pflayer/pf.h"
#include "../pflayer/slotted.h"
#include "am.h"

extern int PF_CreateFile(char *fname);
extern int PF_OpenFile(char *fname);
extern int PF_AllocPage(int fd, int *pagenum, char **pagebuf);
extern int PF_UnfixPage(int fd, int pagenum, int dirty);
extern int PF_CloseFile(int fd);
extern int PF_GetFirstPage(int fd, int *pagenum, char **pagebuf);
extern int PF_GetNextPage(int fd, int *pagenum, char **pagebuf);\

#define MAX_STUDENTS 20000

typedef struct {
    char rollNo[8];
    int recordId;
} StudentEntry;

StudentEntry students[MAX_STUDENTS];
char sortedKeys[MAX_STUDENTS][8];
int sortedIds[MAX_STUDENTS];

int compareStudents(const void *a, const void *b) {
    StudentEntry *x = (StudentEntry *)a;
    StudentEntry *y = (StudentEntry *)b;

    return memcmp(x->rollNo, y->rollNo, 8);
}

extern void qsort(void *base, size_t count, size_t size,int (*compare)(const void *, const void *));

int scanIds[MAX_STUDENTS];

int compareIds(const void *a, const void *b) {
    int x = *(int *)a;
    int y = *(int *)b;

    if(x < y) {
        return -1;
    } else if(x > y) {
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    FILE *fp;
    int fd;
    int pageNum;
    int slot;
    int count = 0;
    int length;
    char *page;
    char record[4096];

    int indexFd;
    int result;

    int currentSlot;
    int recordId;
    int indexCount = 0;
    int scanResult;
    uint16_t recordLen;
    char rollNo[8];
    char *first;
    char *second;
    int scanFd;
    int scanCount = 0;
    int scanId;
    int foundId;
    char searchRoll[8] = "95302001";
    int checkPage;
    int checkSlot;
    int slotPos;
    PageHeader header;
    Slot slotInfo;
    int *seen;
    int duplicateIds = 0;
    int missingIds = 0;
    int invalidIds = 0;
    int expectedId;

    if(argc < 2) {
        printf("Usage: ./student_index <student_file>\n");
        return 1;
    }

    PF_Init();

    fp = fopen(argv[1], "r");

    if(fp == NULL) {
        printf("Cannot open Student file\n");
        return 1;
    }

    // Skip the first line
    fgets(record, sizeof(record), fp);

    if(PF_CreateFile("student_bulk.dat") != PFE_OK) {
        printf("Cannot create Student database file\n");
        fclose(fp);
        return 1;
    }

    fd = PF_OpenFile("student_bulk.dat");

    if(fd < 0) {
        printf("Cannot open Student database file\n");
        fclose(fp);
        return 1;
    }

    if(PF_AllocPage(fd, &pageNum, &page) != PFE_OK) {
        printf("Cannot allocate page\n");
        fclose(fp);
        PF_CloseFile(fd);
        return 1;
    }

    slotted_init_page(page);

    while(fgets(record, sizeof(record), fp) != NULL) {
        length = strlen(record);

        while(length > 0 && (record[length-1] == '\n' || record[length-1] == '\r')) {
            record[length-1] = '\0';
            length--;
        }

        if(length == 0) {
            continue;
        }

        slot = slotted_insert_record(page, record, length);

        if(slot == -1) {
            PF_UnfixPage(fd, pageNum, TRUE);

            if(PF_AllocPage(fd, &pageNum, &page) != PFE_OK) {
                printf("Cannot allocate new page\n");
                fclose(fp);
                PF_CloseFile(fd);
                return 1;
            }

            slotted_init_page(page);

            slot = slotted_insert_record(page, record, length);

            if(slot == -1) {
                printf("Record too large\n");
                fclose(fp);
                PF_UnfixPage(fd, pageNum, TRUE);
                PF_CloseFile(fd);
                return 1;
            }
        }

        count++;
    }

    PF_UnfixPage(fd, pageNum, TRUE);
    PF_CloseFile(fd);
    fclose(fp);

    // Create B+ tree index on student roll numbers
    result = AM_CreateIndex("student_bulk.dat", 0, 'c', 8);

    if(result != 0) {
        printf("Cannot create Student index\n");
        return 1;
    }

    // Open the index file
    indexFd = PF_OpenFile("student_bulk.dat.0");

    if(indexFd < 0) {
        printf("Cannot open Student index\n");
        return 1;
    }

    printf("Student B+ tree index created successfully\n");

    // Open the stored Student records
    fd = PF_OpenFile("student_bulk.dat");

    if(fd < 0) {
        printf("Cannot open Student records\n");
        PF_CloseFile(indexFd);
        return 1;
    }

    // Read the first page
    result = PF_GetFirstPage(fd, &pageNum, &page);

    while(result == PFE_OK) {
        currentSlot = 0;

        while(1) {
            scanResult = slotted_get_next_record(page, &currentSlot,
                                                record, sizeof(record)-1,
                                                &recordLen);

            if(scanResult == 0) {
                break;
            }

            if(scanResult < 0) {
                printf("Error reading Student record\n");
                return 1;
            }

            record[recordLen] = '\0';

            // Extract roll number from second field
            first = strchr(record, ';');

            if(first == NULL) {
                printf("Invalid Student record\n");
                printf("Page number: %d\n", pageNum);
                printf("Slot number: %d\n", currentSlot-1);
                printf("Record length: %d\n", recordLen);
                printf("Record: %.100s\n", record);
                printf("Records indexed before error: %d\n", indexCount);
                return 1;
            }

            second = strchr(first+1, ';');

            if(second == NULL || second-first-1 > 8) {
                printf("Invalid roll number\n");
                return 1;
            }

            memset(rollNo, 0, sizeof(rollNo));
            memcpy(rollNo, first+1, second-first-1);

            // Combine page number and slot number into record ID
            recordId = (pageNum << 16) | (currentSlot-1);

            // Store roll number and record ID for bulk loading
            if(indexCount >= MAX_STUDENTS) {
                printf("Student array is full\n");
                return 1;
            }

            memcpy(students[indexCount].rollNo, rollNo, 8);
            students[indexCount].recordId = recordId;

            indexCount++;
        }

        PF_UnfixPage(fd, pageNum, FALSE);
        result = PF_GetNextPage(fd, &pageNum, &page);
    }

    PF_CloseFile(fd);

    // Sort students by roll number
    qsort(students, indexCount, sizeof(StudentEntry), compareStudents);
    int i;
    // Prepare sorted keys and record IDs
    for(i=0; i<indexCount; i++) {
        memcpy(sortedKeys[i], students[i].rollNo, 8);
        sortedIds[i] = students[i].recordId;
    }

    // Build B+ tree using bulk loading
    result = AM_BulkLoad(indexFd, 'c', 8,
                        (char *)sortedKeys, sortedIds, indexCount);

    if(result != 0) {
        printf("Bulk loading failed\n");
        return 1;
    }

    printf("Total records bulk loaded: %d\n", indexCount);

    // Scan all records from the B+ tree index
    scanFd = AM_OpenIndexScan(indexFd, 'c', 8, EQUAL, NULL);

    if(scanFd < 0) {
        printf("Cannot open index scan\n");
        return 1;
    }

    while(1) {
        scanId = AM_FindNextEntry(scanFd);

        if(scanId == AME_EOF) {
            break;
        }

        if(scanId < 0) {
            printf("Index scan failed: %d\n", scanId);
            return 1;
        }

        if(scanCount >= MAX_STUDENTS) {
        printf("Too many records returned\n");
        return 1;
    }

    scanIds[scanCount] = scanId;
    scanCount++;
    }

    AM_CloseIndexScan(scanFd);

    printf("Records retrieved from index: %d\n", scanCount);

    // Compare all record IDs with the original Student records
    if(scanCount != indexCount) {
        printf("Record count mismatch\n");
        return 1;
    }

    qsort(sortedIds, indexCount, sizeof(int), compareIds);
    qsort(scanIds, scanCount, sizeof(int), compareIds);

    int mismatch = 0;

    for(i=0; i<indexCount; i++) {
        if(sortedIds[i] != scanIds[i]) {
            mismatch++;
        }
    }

    printf("Record ID mismatches: %d\n", mismatch);

    if(mismatch == 0) {
        printf("All record IDs verified successfully\n");
    } else {
        printf("Record ID verification failed\n");
        return 1;
    }


    // Search for one particular roll number
    scanFd = AM_OpenIndexScan(indexFd, 'c', 8, EQUAL, searchRoll);

    if(scanFd < 0) {
        printf("Cannot open roll number search\n");
        return 1;
    }

    foundId = AM_FindNextEntry(scanFd);

    if(foundId >= 0) {
        printf("Roll number 95302001 found\n");
        printf("Page number: %u\n", (unsigned int)foundId >> 16);
        printf("Slot number: %d\n", foundId & 0xFFFF);
        // Read the actual Student record using the record ID
        fd = PF_OpenFile("student_bulk.dat");

        if(fd < 0) {
            printf("Cannot open Student file\n");
            return 1;
        }

        checkPage = (unsigned int)foundId >> 16;
        checkSlot = foundId & 0xFFFF;

        result = PF_GetThisPage(fd, checkPage, &page);

        if(result != PFE_OK) {
            printf("Cannot read Student page\n");
            PF_CloseFile(fd);
            return 1;
        }

        memcpy(&header, page, sizeof(PageHeader));

        if(checkSlot >= header.slot_count) {
            printf("Invalid slot number\n");
            PF_UnfixPage(fd, checkPage, FALSE);
            PF_CloseFile(fd);
            return 1;
        }

        slotPos = sizeof(PageHeader) + checkSlot*sizeof(Slot);
        memcpy(&slotInfo, page + slotPos, sizeof(Slot));

        if(slotInfo.length == 0 || slotInfo.offset + slotInfo.length > SLOTTED_PAGE_SIZE) {
            printf("Invalid Student record location\n");
            PF_UnfixPage(fd, checkPage, FALSE);
            PF_CloseFile(fd);
            return 1;
        }

        memcpy(record, page + slotInfo.offset, slotInfo.length);
        record[slotInfo.length] = '\0';

        printf("Retrieved Student: %s\n", record);

        PF_UnfixPage(fd, checkPage, FALSE);
        PF_CloseFile(fd);
    } else {
        printf("Roll number not found\n");
    }

    AM_CloseIndexScan(scanFd);

    // Check duplicate roll numbers
    char duplicateRoll[8] = "97305818";
    int duplicateCount = 0;

    scanFd = AM_OpenIndexScan(indexFd, 'c', 8, EQUAL, duplicateRoll);

    if(scanFd < 0) {
        printf("Cannot open duplicate key scan\n");
        return 1;
    }

    while(1) {
        scanId = AM_FindNextEntry(scanFd);

        if(scanId == AME_EOF) {
            break;
        }

        if(scanId < 0) {
            printf("Duplicate scan failed\n");
            return 1;
        }

        duplicateCount++;
    }

    AM_CloseIndexScan(scanFd);

    printf("Records found for duplicate roll number: %d\n", duplicateCount);

    PF_CloseFile(indexFd);

    printf("Total Student records inserted: %d\n", count);

    return 0;
}