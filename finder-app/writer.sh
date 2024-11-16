#!/bin/bash

# Check if the required parameters are provided
if [ $# -ne 2 ]; then
  echo "Error: Two arguments required."
  echo "Usage: $0 <file_path> <text_string>"
  exit 1
fi

# Assign parameters to variables
writefile=$1
writestr=$2

# Create the directory path if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Attempt to write to the file
echo "$writestr" > "$writefile" 2>/dev/null

# Check if the write was successful
if [ $? -ne 0 ]; then
  echo "Error: Could not create or write to the file $writefile."
  exit 1
fi

echo "Successfully wrote to $writefile."
