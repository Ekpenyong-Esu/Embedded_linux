
#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                          size_t char_offset,
                                                                          size_t *entry_offset_byte_rtn)
{
    if (!buffer || !entry_offset_byte_rtn)
    {
        DEBUG_LOG("Invalid parameters in find_entry\n");
        return NULL;
    }

    size_t current_pos = 0;
    uint8_t current_idx = buffer->out_offs;
    uint8_t entries_checked = 0;
    uint8_t total_entries;

    // Calculate total valid entries
    if (buffer->full)
    {
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else if (buffer->in_offs >= buffer->out_offs)
    {
        total_entries = buffer->in_offs - buffer->out_offs;
    }
    else
    {
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - buffer->out_offs + buffer->in_offs;
    }

    DEBUG_LOG("Searching for offset %zu in %d entries\n", char_offset, total_entries);

    // Search through valid entries
    while (entries_checked < total_entries)
    {
        size_t entry_size = buffer->entry[current_idx].size;

        // Check if offset falls within this entry
        if (char_offset < current_pos + entry_size)
        {
            *entry_offset_byte_rtn = char_offset - current_pos;
            DEBUG_LOG("Found offset in entry %d at relative offset %zu\n", current_idx, *entry_offset_byte_rtn);
            return &buffer->entry[current_idx];
        }

        current_pos += entry_size;
        current_idx = (current_idx + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entries_checked++;
    }

    DEBUG_LOG("Offset %zu not found in buffer\n", char_offset);
    return NULL;
}
