#include <stdio.h>
#include <string.h>
#include "slotted.h"

int main()
{
    char page[SLOTTED_PAGE_SIZE];

    char record_a[1500];
    char record_b[1500];
    char record_c[2000];

    PageHeader header;
    Slot slot_a, slot_b;

    int slot_a_no, slot_b_no, slot_c_no;

    memset(record_a, 'A', sizeof(record_a));
    memset(record_b, 'B', sizeof(record_b));
    memset(record_c, 'C', sizeof(record_c));

    /* Initialize page */
    slotted_init_page(page);

    /* Insert two large records */
    slot_a_no = slotted_insert_record(
        page, record_a, sizeof(record_a));

    slot_b_no = slotted_insert_record(
        page, record_b, sizeof(record_b));

    if (slot_a_no < 0 || slot_b_no < 0) {
        printf("Initial insertion failed\n");
        return 1;
    }

    printf("Inserted records A and B\n");

    /* Delete record A, creating a gap */
    if (slotted_delete_record(page, slot_a_no) != 0) {
        printf("Deletion failed\n");
        return 1;
    }

    printf("Deleted record A\n");

    memcpy(&header, page, sizeof(PageHeader));

    printf("Available contiguous space before C: %u bytes\n",
           header.free_end - header.free_start);

    /*
     * Record C needs 2000 bytes.
     * It cannot fit in the current contiguous free space.
     * Insertion should automatically compact the page.
     */
    slot_c_no = slotted_insert_record(
        page, record_c, sizeof(record_c));

    if (slot_c_no < 0) {
        printf("Automatic compaction/insertion failed\n");
        return 1;
    }

    printf("\nRecord C inserted in slot: %d\n", slot_c_no);

    /* Read updated slots */
    memcpy(&header, page, sizeof(PageHeader));

    memcpy(&slot_a,
           page + sizeof(PageHeader) + slot_a_no * sizeof(Slot),
           sizeof(Slot));

    memcpy(&slot_b,
           page + sizeof(PageHeader) + slot_b_no * sizeof(Slot),
           sizeof(Slot));

    printf("Slot count: %u\n", header.slot_count);
    printf("Slot A length: %u\n", slot_a.length);
    printf("Slot B length: %u\n", slot_b.length);

    /* Verify both records */
    if (slot_c_no == slot_a_no &&
        slot_a.length == sizeof(record_c) &&
        slot_b.length == sizeof(record_b) &&
        memcmp(page + slot_a.offset,
               record_c, sizeof(record_c)) == 0 &&
        memcmp(page + slot_b.offset,
               record_b, sizeof(record_b)) == 0) {

        printf("\nAUTOMATIC COMPACTION TEST PASSED\n");
    } else {
        printf("\nAUTOMATIC COMPACTION TEST FAILED\n");
        return 1;
    }

    return 0;
}