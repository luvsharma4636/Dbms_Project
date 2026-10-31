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
    int result;

    /* Initialize page */
    slotted_init_page(page);

    /* Insert a record */
    slot_no = slotted_insert_record(
        page,
        record,
        strlen(record)
    );

    if (slot_no == -1) {
        printf("Insertion failed\n");
        return 1;
    }

    printf("Inserted record in slot: %d\n", slot_no);

    /* Delete the record */
    result = slotted_delete_record(page, slot_no);

    if (result == -1) {
        printf("Deletion failed\n");
        return 1;
    }

    printf("Record deleted successfully\n");

    /* Read the header */
    memcpy(&header, page, sizeof(PageHeader));

    /* Read the deleted slot */
    memcpy(
        &slot,
        page + sizeof(PageHeader) + slot_no * sizeof(Slot),
        sizeof(Slot)
    );

    printf("\nAfter deletion:\n");
    printf("Slot count: %u\n", header.slot_count);
    printf("Deleted slot offset: %u\n", slot.offset);
    printf("Deleted slot length: %u\n", slot.length);

    if (slot.length == 0 && header.slot_count == 1) {
        printf("\nDELETE TEST PASSED\n");
    } else {
        printf("\nDELETE TEST FAILED\n");
        return 1;
    }

    return 0;
}