#!/bin/sh

# Check if the required parameters are provided
if [ $# -ne 2 ]; then
  echo "Error: Two arguments required."
  echo "Usage: $0 <directory_path> <search_string>"
  exit 1
fi

# Assign parameters to variables
filesdir=$1
searchstr=$2

# Check if filesdir is a valid directory
if [ ! -d "$filesdir" ]; then
  echo "Error: $filesdir is not a directory."
  exit 1
fi

# Find the number of files in the directory and subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Find the number of lines containing the search string
match_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Print the result
echo "The number of files are $file_count and the number of matching lines are $match_count"
