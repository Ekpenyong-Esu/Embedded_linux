# AESD Character Driver Implementation

## Overview
This implementation adds llseek and ioctl support to the AESD character driver as requested.

## Features Implemented

### 1. llseek Support
- **Location**: `char-driver/src/aesd-char-fileops.c` - `aesd_llseek()` function
- **Functionality**:
  - Supports SEEK_SET, SEEK_CUR, and SEEK_END operations
  - Uses cumulative size of written entries for calculations
  - Returns -EINVAL for out-of-bound seeks
  - Thread-safe with mutex protection

### 2. ioctl Support
- **Location**: `char-driver/src/aesd-char-fileops.c` - `aesd_unlocked_ioctl()` function
- **Command**: `AESDCHAR_IOCSEEKTO`
- **Parameters**:
  - `write_cmd`: Command index (0-based)
  - `write_cmd_offset`: Byte offset within that command
- **Validation**:
  - Validates command index against available entries
  - Validates offset within the specified command
  - Returns -EINVAL for invalid parameters
  - Uses `copy_from_user()` to retrieve values safely

### 3. Socket Application Updates
- **Location**: `server/aesdsocket_app/src/thread_manager.c`
- **Functionality**:
  - Parses socket input for `AESDCHAR_IOCSEEKTO:X,Y` format
  - Extracts X (command) and Y (offset) values
  - Calls `ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto_struct)`
  - Does NOT write the ioctl string to the driver
  - Performs `read()` on the same FD and sends content back over socket

## Implementation Details

### Helper Functions
- `aesd_get_total_buffer_size()`: Calculates cumulative size of all valid entries
- Proper bounds checking for both llseek and ioctl operations
- Thread-safe operations with mutex protection

### File Operations Structure
Updated `aesd_fops` to include:
```c
.llseek = aesd_llseek,
.unlocked_ioctl = aesd_unlocked_ioctl,
```

### Error Handling
- Returns appropriate error codes (-EINVAL, -EFAULT, -ERESTARTSYS)
- Proper cleanup of resources on errors
- Validation of all input parameters

## Testing
- Created test programs for validation
- Socket test script for end-to-end testing
- Proper error case testing

## Files Modified
1. `char-driver/include/aesdchar.h` - Added function declarations
2. `char-driver/src/aesd-char-fileops.c` - Implemented llseek and ioctl
3. `server/aesdsocket_app/src/thread_manager.c` - Added ioctl command parsing
4. Created test files for validation

## Compilation Status
- Driver compiles successfully with kernel build system
- Socket application compiles without errors
- All features ready for testing
