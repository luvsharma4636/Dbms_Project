
#include <stdio.h>
#include <stdlib.h>
#include "pf.h"
#include "pftypes.h"

#define NUM_PAGES 100
#define BUFFER_SIZE 10
#define NUM_OPERATIONS 1000

int pageSequence[NUM_OPERATIONS];
int operationSequence[NUM_OPERATIONS];

void generateWorkload() {
    int i;

    for(i=0; i<NUM_OPERATIONS; i++) {
        pageSequence[i] = i%11;
        operationSequence[i] = (i*37)%100;
    }
}

int runExperiment(int policy, int writePercent, FILE *csv) {
    int fd;
    int i;
    int error;
    int pagenum;
    int writes = 0;
    char *pagebuf;
    long totalIO;
    double hitRate;
    char *policyName;

    policyName = policy == PF_LRU ? "LRU" : "MRU";

    fd = PF_OpenFileWithPolicy("benchmark_data", policy);
    if(fd < 0) {
        PF_PrintError("Open benchmark file");
        return 1;
    }

    PF_ResetStats();

    for(i=0; i<NUM_OPERATIONS; i++) {
        pagenum = pageSequence[i];

        error = PF_GetThisPage(fd, pagenum, &pagebuf);
        if(error != PFE_OK) {
            PF_PrintError("Get page");
            return 1;
        }

        if(operationSequence[i] < writePercent) {
            pagebuf[0] = (char)(i%256);

            error = PF_MarkDirty(fd, pagenum);
            if(error != PFE_OK) {
                PF_PrintError("Mark dirty");
                return 1;
            }

            writes++;
        }

        error = PF_UnfixPage(fd, pagenum, FALSE);
        if(error != PFE_OK) {
            PF_PrintError("Unfix page");
            return 1;
        }
    }

    // Include dirty-page writes performed during closing.
    error = PF_CloseFile(fd);
    if(error != PFE_OK) {
        PF_PrintError("Close benchmark file");
        return 1;
    }

    totalIO = PFstats.physicalReads + PFstats.physicalWrites;

    hitRate = 100.0*PFstats.bufferHits/
              (PFstats.bufferHits + PFstats.bufferMisses);

    printf("%-6s %3d/%-3d %6ld %6ld %6ld %6ld %7.2f%%\n",
           policyName,
           100-writePercent, writePercent,
           PFstats.physicalReads,
           PFstats.physicalWrites,
           totalIO,
           PFstats.bufferHits,
           hitRate);

    fprintf(csv, "%s,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%.2f\n",
            policyName,
            100-writePercent,
            writePercent,
            NUM_PAGES,
            BUFFER_SIZE,
            PFstats.logicalReads,
            PFstats.logicalWrites,
            PFstats.physicalReads,
            PFstats.physicalWrites,
            PFstats.bufferHits,
            PFstats.bufferMisses,
            totalIO,
            hitRate);

    return 0;
}

int main() {
    int fd;
    int i;
    int pagenum;
    int error;
    int policy;
    int ratios[] = {0, 25, 50, 75, 100};
    char *pagebuf;
    FILE *csv;

    PF_Init();
    PF_SetBufferSize(BUFFER_SIZE);

    // Create the database once, before measurements.
    error = PF_CreateFile("benchmark_data");
    if(error != PFE_OK) {
        PF_PrintError("Create benchmark file");
        return 1;
    }

    fd = PF_OpenFile("benchmark_data");
    if(fd < 0) {
        PF_PrintError("Open benchmark file");
        return 1;
    }

    for(i=0; i<NUM_PAGES; i++) {
        error = PF_AllocPage(fd, &pagenum, &pagebuf);
        if(error != PFE_OK) {
            PF_PrintError("Allocate page");
            return 1;
        }

        pagebuf[0] = 0;

        error = PF_MarkDirty(fd, pagenum);
        if(error != PFE_OK) {
            PF_PrintError("Mark initial page");
            return 1;
        }

        error = PF_UnfixPage(fd, pagenum, FALSE);
        if(error != PFE_OK) {
            PF_PrintError("Unfix initial page");
            return 1;
        }
    }

    error = PF_CloseFile(fd);
    if(error != PFE_OK) {
        PF_PrintError("Close initial file");
        return 1;
    }

    generateWorkload();

    csv = fopen("benchmark_results.csv", "w");
    if(csv == NULL) {
        perror("CSV file");
        return 1;
    }

    fprintf(csv, "Policy,ReadPercent,WritePercent,");
    fprintf(csv, "DatabasePages,BufferSize,LogicalReads,");
    fprintf(csv, "LogicalWrites,PhysicalReads,PhysicalWrites,");
    fprintf(csv, "BufferHits,BufferMisses,TotalPhysicalIO,HitRate\n");

    printf("\nLRU vs MRU Performance Comparison\n\n");
    printf("Policy R/W     PRead  PWrite TotalIO Hits   HitRate\n");
    printf("---------------------------------------------------\n");

    for(policy=PF_LRU; policy<=PF_MRU; policy++) {
        for(i=0; i<5; i++) {
            error = runExperiment(policy, ratios[i], csv);
            if(error != 0) {
                fclose(csv);
                return 1;
            }
        }
    }

    fclose(csv);

    error = PF_DestroyFile("benchmark_data");
    if(error != PFE_OK) {
        PF_PrintError("Destroy benchmark file");
        return 1;
    }

    printf("\nResults saved to benchmark_results.csv\n");

    return 0;
}
