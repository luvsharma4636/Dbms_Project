#include <stdio.h>
#include <string.h>
#include "slotted.h"

int main()
{
    char page[SLOTTED_PAGE_SIZE];
    char output[100];

    char record1[] = "Alice";
    char record2[] = "Bob";
    char record3[] = "Charlie";

    int s1, s2, s3;
    int current_slot = 0;
    int result;

    uint16_t record_len;

    slotted_init_page(page);

    /* Insert three records */
    s1 = slotted_insert_record(page, record1, strlen(record1));
    s2 = slotted_insert_record(page, record2, strlen(record2));
    s3 = slotted_insert_record(page, record3, strlen(record3));

    if (s1 < 0 || s2 < 0 || s3 < 0) {
        printf("Insertion failed\n");
        return 1;
    }

    /* Delete Bob */
    if (slotted_delete_record(page, s2) != 0) {
        printf("Deletion failed\n");
        return 1;
    }

    printf("Sequential scan results:\n");

    while (1) {

        result = slotted_get_next_record(
            page,
            &current_slot,
            output,
            sizeof(output),
            &record_len
        );

        if (result == 0)
            break;

        if (result == -1) {
            printf("Scan failed\n");
            return 1;
        }

        printf("%.*s\n", (int)record_len, output);
    }

    printf("\nSCAN TEST PASSED\n");

    return 0;
}