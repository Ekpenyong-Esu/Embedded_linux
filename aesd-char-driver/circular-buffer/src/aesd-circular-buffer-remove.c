#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer)
{
    if (!buffer)
    {
        DEBUG_LOG("Invalid buffer parameter in remove_entry\n");
        return;
    }

    // Check if buffer is empty
    if (!buffer->full && (buffer->in_offs == buffer->out_offs))
    {
        DEBUG_LOG("Attempted to remove from empty buffer\n");
        return;
    }

    DEBUG_LOG("Removing entry at position %d\n", buffer->out_offs);

    // Clear the entry
    buffer->entry[buffer->out_offs].buffptr = NULL;
    buffer->entry[buffer->out_offs].size = 0;

    // Advance out_offs and update full flag
    buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    buffer->full = false;

    DEBUG_LOG("Buffer state after remove: in=%d, out=%d, full=%d\n", buffer->in_offs, buffer->out_offs, buffer->full);
}
