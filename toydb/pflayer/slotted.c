
#include <stdio.h>
#include <string.h>
#include "slotted.h"

void slotted_init_page(char *page)
{
    PageHeader header;

    /* Clear the entire page */
    memset(page, 0, SLOTTED_PAGE_SIZE);

    /* Initialize the page header */
    header.slot_count = 0;
    header.free_start = sizeof(PageHeader);
    header.free_end = SLOTTED_PAGE_SIZE;

    /* Copy the header into the page */
    memcpy(page, &header, sizeof(PageHeader));
}


int slotted_insert_record(char *page,
                          const char *record,
                          uint16_t record_len)
{
    PageHeader header;
    Slot slot;

    int i;
    int slot_no = -1;
    int new_slot_needed;
    int slot_pos;
    int new_offset;
    int required_space;
    int available_space;

    if (page == NULL || record == NULL || record_len == 0)
        return -1;

    /* Read the current page header */
    memcpy(&header, page, sizeof(PageHeader));

    if (header.free_start < sizeof(PageHeader) ||
    header.free_start > header.free_end ||
    header.free_end > SLOTTED_PAGE_SIZE)
    return -1;

    /* Look for a previously deleted slot */
    for (i = 0; i < header.slot_count; i++) {
        slot_pos = sizeof(PageHeader) + i * sizeof(Slot);

        memcpy(&slot, page + slot_pos, sizeof(Slot));

        if (slot.length == 0) {
            slot_no = i;
            break;
        }
    }

    new_slot_needed = (slot_no == -1);

    /* Calculate the required space */
    required_space = record_len;

    if (new_slot_needed)
        required_space += sizeof(Slot);

    available_space = header.free_end - header.free_start;

    /* Check whether enough contiguous space exists */
    if (required_space > available_space)
        return -1;

    /* Allocate record storage from the end of the page */
    new_offset = header.free_end - record_len;

    memcpy(page + new_offset, record, record_len);

    /* Create a new slot if necessary */
    if (new_slot_needed) {
        slot_no = header.slot_count;
        header.slot_count++;
        header.free_start += sizeof(Slot);
    }

    /* Store the record's location in the slot */
    slot.offset = new_offset;
    slot.length = record_len;

    slot_pos = sizeof(PageHeader) + slot_no * sizeof(Slot);

    memcpy(page + slot_pos, &slot, sizeof(Slot));

    /* Update the free-space boundary */
    header.free_end = new_offset;

    /* Write the updated header back */
    memcpy(page, &header, sizeof(PageHeader));

    return slot_no;
}

int slotted_delete_record(char *page, int slot_no)
{
    PageHeader header;
    Slot slot;
    int slot_pos;

    if (page == NULL)
        return -1;

    /* Read the page header */
    memcpy(&header, page, sizeof(PageHeader));

    /* Validate the slot number */
    if (slot_no < 0 || slot_no >= header.slot_count)
        return -1;

    /* Locate the slot entry */
    slot_pos = sizeof(PageHeader) + slot_no * sizeof(Slot);

    memcpy(&slot, page + slot_pos, sizeof(Slot));

    /* Check whether the record is already deleted */
    if (slot.length == 0)
        return -1;

    /* Mark the slot as deleted */
    slot.offset = 0;
    slot.length = 0;

    /* Write the updated slot back */
    memcpy(page + slot_pos, &slot, sizeof(Slot));

    return 0;
}