
#ifndef SLOTTED_H
#define SLOTTED_H

#include <stdint.h>

#define SLOTTED_PAGE_SIZE 4096

typedef struct {
    uint16_t slot_count;
    uint16_t free_start;
    uint16_t free_end;
} PageHeader;

typedef struct {
    uint16_t offset;
    uint16_t length;
} Slot;

/* Initialize an empty slotted page */
void slotted_init_page(char *page);

/* Insert a record and return its slot number */
int slotted_insert_record(char *page,
                          const char *record,
                          uint16_t record_len);

int slotted_delete_record(char *page, int slot_no);

int slotted_compact_page(char *page);

/*
 * Returns:
 *  1  = A record was successfully retrieved
 *  0  = No more records
 * -1  = Invalid input or insufficient output buffer
 */
int slotted_get_next_record(
    char *page,
    int *current_slot,
    char *record,
    uint16_t record_capacity,
    uint16_t *record_len
);
#endif
