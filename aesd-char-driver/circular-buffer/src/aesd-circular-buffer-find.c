/**
 * @file aesd-circular-buffer-find.c
 * @brief Implementation of AESD circular buffer entry search functionality
 *
 * This file contains the implementation of the circular buffer entry search
 * function used by the AESD character driver. It provides efficient lookup
 * of buffer entries based on absolute character position within the logical
 * concatenated buffer content.
 *
 * @author Mahonri Wozny
 * @date Created: November 2024
 * @version 1.0
 *
 * Features:
 * - Efficient entry lookup by absolute character offset
 * - Proper handling of circular buffer wraparound logic
 * - Support for both full and partial buffer states
 * - Relative offset calculation within found entries
 * - Comprehensive parameter validation and error handling
 */

#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

/**
 * @brief Find the buffer entry containing the specified character offset
 *
 * This function searches through the circular buffer to find which entry
 * contains the character at the specified absolute offset. The offset is
 * treated as if all buffer entries were concatenated in order, providing
 * a logical view of the buffer contents as a single continuous stream.
 *
 * Search algorithm:
 * 1. Calculate the total number of valid entries in the buffer
 * 2. Iterate through entries starting from the output offset (oldest)
 * 3. For each entry, check if the target offset falls within its bounds
 * 4. If found, calculate the relative offset within that entry
 * 5. Return pointer to the entry and set relative offset
 *
 * Buffer state handling:
 * - Full buffer: All entries are valid, iterate through all positions
 * - Partial buffer: Only entries between out_offs and in_offs are valid
 * - Empty buffer: No valid entries, returns NULL immediately
 *
 * @param buffer Pointer to the circular buffer structure to search
 *               Must not be NULL, otherwise function returns NULL
 * @param char_offset Absolute character offset to search for (0-based)
 *                    This is the position as if all entries were concatenated
 * @param entry_offset_byte_rtn Pointer to store the relative offset within
 *                              the found entry. Must not be NULL.
 *
 * @return Pointer to the buffer entry containing the specified offset,
 *         or NULL if offset is not found or parameters are invalid
 *
 * @note The search starts from the oldest entry (out_offs position)
 * @note Wrap-around is handled automatically for circular buffer traversal
 * @note The returned relative offset is 0-based within the found entry
 * @note This function supports both kernel and userspace environments
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                          size_t char_offset,
                                                                          size_t *entry_offset_byte_rtn)
{
    // Validate input parameters to prevent null pointer dereference
    if (!buffer || !entry_offset_byte_rtn)
    {
        DEBUG_LOG("Invalid parameters in find_entry\n");
        return NULL;
    }

    size_t current_pos = 0;                 // Running total of characters processed
    uint8_t current_idx = buffer->out_offs; // Start from oldest entry
    uint8_t entries_checked = 0;            // Counter for loop termination
    uint8_t total_entries;                  // Total valid entries to search

    // Calculate total valid entries based on buffer state
    if (buffer->full)
    {
        // All entries are valid when buffer is full
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else if (buffer->in_offs >= buffer->out_offs)
    {
        // Simple case: no wraparound, entries are contiguous
        total_entries = buffer->in_offs - buffer->out_offs;
    }
    else
    {
        // Wraparound case: entries span from out_offs to end, then 0 to in_offs
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - buffer->out_offs + buffer->in_offs;
    }

    DEBUG_LOG("Searching for offset %zu in %d entries\n", char_offset, total_entries);

    // Search through valid entries in chronological order (oldest to newest)
    while (entries_checked < total_entries)
    {
        size_t entry_size = buffer->entry[current_idx].size;

        // Check if the target offset falls within the current entry's range
        // Range: [current_pos, current_pos + entry_size)
        if (char_offset < current_pos + entry_size)
        {
            // Found the entry containing the target offset
            // Calculate relative offset within this entry
            *entry_offset_byte_rtn = char_offset - current_pos;

            DEBUG_LOG("Found offset in entry %d at relative offset %zu\n", current_idx, *entry_offset_byte_rtn);
            return &buffer->entry[current_idx];
        }

        // Move to next entry: update position and advance index with wraparound
        current_pos += entry_size;
        current_idx = (current_idx + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entries_checked++;
    }

    // Offset not found in any valid buffer entry
    DEBUG_LOG("Offset %zu not found in buffer\n", char_offset);
    return NULL;
}
