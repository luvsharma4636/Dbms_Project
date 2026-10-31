#include <stdio.h>
#include <string.h>
#include "slotted.h"

int main()
{
    char page[SLOTTED_PAGE_SIZE];
    char record[] = "101;Alice;Computer Science";
    PageHeader header;
    Slot slot;
    int slot_no;

    /* Step 1: Initialize the page */
    slotted_init_page(page);

    memcpy(&header, page, sizeof(PageHeader));

    printf("After initialization:\n");
    printf("Slot count: %u\n", header.slot_count);
    printf("Free start: %u\n", header.free_start);
    printf("Free end: %u\n", header.free_end);

    /* Step 2: Insert a record */
    slot_no = slotted_insert_record(
        page,
        record,
        strlen(record)
    );

    if (slot_no == -1) {
        printf("Record insertion failed!\n");
        return 1;
    }

    /* Step 3: Read the updated header */
    memcpy(&header, page, sizeof(PageHeader));

    printf("\nAfter insertion:\n");
    printf("Inserted slot: %d\n", slot_no);
    printf("Slot count: %u\n", header.slot_count);
    printf("Free start: %u\n", header.free_start);
    printf("Free end: %u\n", header.free_end);

    /* Step 4: Read the slot directory entry */
    memcpy(
        &slot,
        page + sizeof(PageHeader) + slot_no * sizeof(Slot),
        sizeof(Slot)
    );

    printf("\nSlot information:\n");
    printf("Record offset: %u\n", slot.offset);
    printf("Record length: %u\n", slot.length);

    /* Step 5: Verify the stored record */
    printf("Stored record: %.*s\n",
           (int)slot.length,
           page + slot.offset);

    if (slot.length == strlen(record) &&
        memcmp(page + slot.offset, record, slot.length) == 0) {
        printf("\nTEST PASSED\n");
    } else {
        printf("\nTEST FAILED\n");
        return 1;
    }

    return 0;
}