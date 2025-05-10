#include "../include/aesd-circular-buffer-common.h"
#include "../include/aesd-circular-buffer.h"

void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    if (!buffer)
    {
        DEBUG_LOG("Invalid buffer parameter in init\n");
        return;
    }

    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
    DEBUG_LOG("Buffer initialized\n");
}
