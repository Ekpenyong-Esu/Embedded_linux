/**
 * @file aesd-circular-buffer-add.c
 * @brief Add entry implementation for AESD circular buffer
 *
 * This file implements the functionality to add new entries to the
 * circular buffer used by the AESD character driver.
 *
 * @author Assignment Team
 * @date June 7, 2025
 */

#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

/**
 * @brief Add a new entry to the circular buffer
 * @param buffer Pointer to the circular buffer structure
 * @param add_entry Pointer to the buffer entry to add
 *
 * This function adds a new entry to the circular buffer at the current
 * in_offs position. The function handles buffer wrap-around and overflow:
 *
 * 1. Validates input parameters
 * 2. Copies the entry to the current input position
 * 3. If buffer is full, advances output offset to maintain circular behavior
 * 4. Advances input offset and updates full flag
 *
 * When the buffer is full and a new entry is added, the oldest entry
 * is effectively overwritten. The calling code is responsible for
 * freeing any memory associated with the overwritten entry.
 */
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /* Input validation */
    if (!buffer || !add_entry || !add_entry->buffptr)
    {
        DEBUG_LOG("Invalid parameters in add_entry\n");
        return;
    }

    DEBUG_LOG("Adding entry of size %zu at position %d\n", add_entry->size, buffer->in_offs);

    /* Copy the entry to the current input position */
    buffer->entry[buffer->in_offs] = *add_entry;

    /* Handle buffer overflow - advance output position if buffer is full */
    if (buffer->full)
    {
        DEBUG_LOG("Buffer full, advancing out_offs from %d\n", buffer->out_offs);
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    /* Advance input position with wrap-around */
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    /* Update full flag - buffer is full when input catches up to output */
    buffer->full = (buffer->in_offs == buffer->out_offs);

    DEBUG_LOG("Buffer state after add: in=%d, out=%d, full=%d\n", buffer->in_offs, buffer->out_offs, buffer->full);
}
