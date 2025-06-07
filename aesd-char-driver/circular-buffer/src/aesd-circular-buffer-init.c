/**
 * @file aesd-circular-buffer-init.c
 * @brief Implementation of AESD circular buffer initialization functionality
 *
 * This file contains the implementation of the circular buffer initialization
 * function used by the AESD character driver for managing command buffers.
 * The initialization ensures all buffer state is properly reset to defaults.
 *
 * @author Mahonri Wozny
 * @date Created: November 2024
 * @version 1.0
 *
 * Features:
 * - Safe buffer initialization with parameter validation
 * - Zero-initialization of all buffer entries and state variables
 * - Debug logging for initialization tracking
 */

#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

/**
 * @brief Initialize a circular buffer structure to default state
 *
 * This function safely initializes a circular buffer by zeroing all memory
 * and setting all internal state variables to their default values. This
 * ensures the buffer is in a known good state before use.
 *
 * The initialization process:
 * 1. Validates the buffer pointer is not NULL
 * 2. Zero-initializes all buffer memory including:
 *    - All buffer entries (pointers and sizes)
 *    - Input and output offset indices
 *    - Full flag state
 * 3. Logs successful initialization for debugging
 *
 * @param buffer Pointer to the circular buffer structure to initialize
 *               Must not be NULL, otherwise function returns early
 *
 * @note This function is safe to call multiple times on the same buffer
 * @note After initialization, the buffer will be empty (not full)
 * @note Both in_offs and out_offs will be set to 0
 */
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    // Validate input parameter to prevent null pointer dereference
    if (!buffer)
    {
        DEBUG_LOG("Invalid buffer parameter in init\n");
        return;
    }

    // Zero-initialize the entire buffer structure
    // This sets all buffer entries to NULL/0, offsets to 0, and full flag to false
    memset(buffer, 0, sizeof(struct aesd_circular_buffer));

    DEBUG_LOG("Buffer initialized\n");
}
