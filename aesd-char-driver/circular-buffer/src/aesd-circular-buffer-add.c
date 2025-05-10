#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    if (!buffer || !add_entry || !add_entry->buffptr)
    {
        DEBUG_LOG("Invalid parameters in add_entry\n");
        return;
    }

    DEBUG_LOG("Adding entry of size %zu at position %d\n", add_entry->size, buffer->in_offs);

    // Add the entry at the current in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;

    // If buffer is full, advance out_offs
    if (buffer->full)
    {
        DEBUG_LOG("Buffer full, advancing out_offs from %d\n", buffer->out_offs);
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Advance in_offs and update full flag
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    buffer->full = (buffer->in_offs == buffer->out_offs);

    DEBUG_LOG("Buffer state after add: in=%d, out=%d, full=%d\n", buffer->in_offs, buffer->out_offs, buffer->full);
}
