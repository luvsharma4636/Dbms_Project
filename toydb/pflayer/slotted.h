
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

#endif
