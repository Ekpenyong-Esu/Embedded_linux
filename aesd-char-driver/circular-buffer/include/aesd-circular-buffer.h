/*
 * aesd-circular-buffer.h
 *
 *  Created on: March 1st, 2020
 *      Author: Dan Walkes
 */

#ifndef AESD_CIRCULAR_BUFFER_H
#define AESD_CIRCULAR_BUFFER_H

#ifdef __KERNEL__
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#define AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED 10

struct aesd_buffer_entry
{
    /**
     * A location where the buffer contents in buffptr are stored
     */
    const char *buffptr;
    /**
     * Number of bytes stored in buffptr
     */
    size_t size;
};

struct aesd_circular_buffer
{
    /**
     * An array of pointers to memory allocated for the most recent write operations
     */
    struct aesd_buffer_entry entry[AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
    /**
     * The current location in the entry structure where the next write should
     * be stored.
     */
    uint8_t in_offs;
    /**
     * The first location in the entry structure to read from
     */
    uint8_t out_offs;
    /**
     * set to true when the buffer entry structure is full
     */
    bool full;
};

/**
 * @brief Find the buffer entry containing the specified character offset
 * @param buffer The circular buffer to search in
 * @param char_offset The absolute character offset to find (0-based)
 * @param entry_offset_byte_rtn Pointer to store the relative offset within the found entry
 * @return Pointer to the buffer entry containing the offset, or NULL if not found
 *
 * This function treats all buffer entries as a single concatenated stream and
 * finds which entry contains the character at the specified absolute position.
 */
extern struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                                 size_t char_offset,
                                                                                 size_t *entry_offset_byte_rtn);

/**
 * @brief Add a new entry to the circular buffer
 * @param buffer The circular buffer to add to
 * @param add_entry The buffer entry to add to the circular buffer
 *
 * Adds a new entry to the circular buffer at the current input position.
 * If the buffer is full, the oldest entry will be overwritten.
 * The function handles wrap-around and buffer state management automatically.
 */
extern void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer,
                                           const struct aesd_buffer_entry *add_entry);

/**
 * Removes the oldest entry in the circular buffer
 * @param buffer The circular buffer to remove from
 *
 * Note: This function updates the out_offs to point to
 * the next oldest entry and clears the full flag if necessary.
 * It does not free any memory allocated for the buffer entry.
 */
extern void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer);

/**
 * @brief Initialize a circular buffer to its default state
 * @param buffer The circular buffer structure to initialize
 *
 * This function initializes all buffer entries to NULL/0, sets the input
 * and output offsets to 0, and clears the full flag. After initialization,
 * the buffer will be in an empty state ready for use.
 */
extern void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer);

/**
 * @brief Macro to iterate over all entries in the circular buffer
 * @param entryptr A struct aesd_buffer_entry* that will be set to each entry
 * @param buffer The struct aesd_circular_buffer* describing the buffer
 * @param index A uint8_t stack allocated variable used as loop index
 *
 * This macro creates a for loop to iterate over each member of the circular buffer.
 * It is particularly useful when you've allocated memory for circular buffer entries
 * and need to free it, or when you need to process all entries regardless of their
 * validity state.
 *
 * @warning This macro iterates over ALL buffer positions, including empty ones.
 *          Check entry->buffptr for NULL to identify valid entries.
 *
 * Example usage:
 * @code
 * uint8_t index;
 * struct aesd_circular_buffer buffer;
 * struct aesd_buffer_entry *entry;
 * AESD_CIRCULAR_BUFFER_FOREACH(entry, &buffer, index) {
 *     if (entry->buffptr != NULL) {
 *         free(entry->buffptr);
 *     }
 * }
 * @endcode
 */
#define AESD_CIRCULAR_BUFFER_FOREACH(entryptr, buffer, index)                                                          \
    for (index = 0, entryptr = &((buffer)->entry[index]); index < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;             \
         index++, entryptr = &((buffer)->entry[index]))


#endif /* AESD_CIRCULAR_BUFFER_H */
