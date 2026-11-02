#include <stdio.h>
#include <string.h>
#include "slotted.h"

int main()
{
    char page[SLOTTED_PAGE_SIZE];

    char record1[] = "Alice";
    char record2[] = "Bob";
    char record3[] = "Charlie";

    PageHeader header;
    Slot slot0, slot1, slot2;

    int s0, s1, s2;
    int old_free_end;

    slotted_init_page(page);

    /* Insert three records */
    s0 = slotted_insert_record(page, record1, strlen(record1));
    s1 = slotted_insert_record(page, record2, strlen(record2));
    s2 = slotted_insert_record(page, record3, strlen(record3));

    if (s0 < 0 || s1 < 0 || s2 < 0) {
        printf("Insertion failed\n");
        return 1;
    }

    memcpy(&header, page, sizeof(PageHeader));
    old_free_end = header.free_end;

    printf("Before deletion and compaction:\n");
    printf("Slot count: %u\n", header.slot_count);
    printf("Free end: %u\n", header.free_end);

    /* Delete the middle record */
    if (slotted_delete_record(page, s1) != 0) {
        printf("Deletion failed\n");
        return 1;
    }

    /* Compact the page */
    if (slotted_compact_page(page) != 0) {
        printf("Compaction failed\n");
        return 1;
    }

    memcpy(&header, page, sizeof(PageHeader));

    /* Read all three slots */
    memcpy(&slot0, page + sizeof(PageHeader), sizeof(Slot));

    memcpy(&slot1,
           page + sizeof(PageHeader) + sizeof(Slot),
           sizeof(Slot));

    memcpy(&slot2,
           page + sizeof(PageHeader) + 2 * sizeof(Slot),
           sizeof(Slot));

    printf("\nAfter compaction:\n");
    printf("Slot count: %u\n", header.slot_count);
    printf("Free end: %u\n", header.free_end);

    printf("\nSlot 0: %.*s\n",
           (int)slot0.length, page + slot0.offset);

    printf("Slot 1 length: %u\n", slot1.length);

    printf("Slot 2: %.*s\n",
           (int)slot2.length, page + slot2.offset);

    /* Verify results */
    if (slot0.length == strlen(record1) &&
        memcmp(page + slot0.offset, record1, slot0.length) == 0 &&
        slot1.length == 0 &&
        slot2.length == strlen(record3) &&
        memcmp(page + slot2.offset, record3, slot2.length) == 0 &&
        header.slot_count == 3 &&
        header.free_end > old_free_end) {

        printf("\nCOMPACTION TEST PASSED\n");
    } else {
        printf("\nCOMPACTION TEST FAILED\n");
        return 1;
    }

    return 0;
}