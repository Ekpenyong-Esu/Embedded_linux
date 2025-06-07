/**
 * @file aesd-circular-buffer-remove.c
 * @brief Implementation of AESD circular buffer entry removal functionality
 *
 * This file contains the implementation of the circular buffer entry removal
 * function used by the AESD character driver. It handles safe removal of the
 * oldest entry from the buffer, updating buffer state and indices appropriately.
 *
 * @author Mahonri Wozny
 * @date Created: November 2024
 * @version 1.0
 *
 * Features:
 * - Safe entry removal with buffer state validation
 * - Proper handling of empty buffer conditions
 * - Circular index management with wrap-around logic
 * - Buffer state tracking and debugging support
 */

#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

/**
 * @brief Remove the oldest entry from the circular buffer
 *
 * This function removes the oldest (first-in) entry from the circular buffer
 * by advancing the output offset and clearing the entry data. It maintains
 * proper circular buffer semantics and updates the buffer state accordingly.
 *
 * The removal process:
 * 1. Validates the buffer pointer is not NULL
 * 2. Checks if the buffer is empty (nothing to remove)
 * 3. Clears the entry at the current output position
 * 4. Advances the output offset with proper wrap-around
 * 5. Updates the buffer full flag (no longer full after removal)
 * 6. Logs the removal operation for debugging
 *
 * Buffer empty condition:
 * - Buffer is empty when not full AND input offset equals output offset
 * - Attempting to remove from empty buffer is a no-op with debug log
 *
 * @param buffer Pointer to the circular buffer structure
 *               Must not be NULL, otherwise function returns early
 *
 * @note This function does NOT free the memory pointed to by buffptr
 * @note After removal, the buffer will never be in full state
 * @note The function is safe to call on empty buffers (no-op)
 * @note Wrap-around is handled automatically using modulo arithmetic
 */
void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer)
{
    // Validate input parameter to prevent null pointer dereference
    if (!buffer)
    {
        DEBUG_LOG("Invalid buffer parameter in remove_entry\n");
        return;
    }

    // Check if buffer is empty - cannot remove from empty buffer
    // Empty condition: not full AND input offset equals output offset
    if (!buffer->full && (buffer->in_offs == buffer->out_offs))
    {
        DEBUG_LOG("Attempted to remove from empty buffer\n");
        return;
    }

    DEBUG_LOG("Removing entry at position %d\n", buffer->out_offs);

    // Clear the entry at the current output position
    // Note: This does NOT free the memory, just clears the reference
    buffer->entry[buffer->out_offs].buffptr = NULL;
    buffer->entry[buffer->out_offs].size = 0;

    // Advance output offset with wrap-around using modulo arithmetic
    // This ensures we stay within the bounds of the circular buffer
    buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // Buffer is no longer full after removing an entry
    buffer->full = false;

    DEBUG_LOG("Buffer state after remove: in=%d, out=%d, full=%d\n", buffer->in_offs, buffer->out_offs, buffer->full);
}
