
#!/bin/sh
# Tester script for assignment 4
# Modified to use PATH-based executables

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
username=$(cat /etc/finder-app/conf/username.txt)

# Check if required executables exist in PATH
if ! command -v writer >/dev/null 2>&1; then
    echo "Error: writer not found in PATH"
    exit 1
fi

if ! command -v finder.sh >/dev/null 2>&1; then
    echo "Error: finder.sh not found in PATH"
    exit 1
fi

if [ $# -lt 3 ]
then
    echo "Using default value ${WRITESTR} for string to write"
    if [ $# -lt 1 ]
    then
        echo "Using default value ${NUMFILES} for number of files to write"
    else
        NUMFILES=$1
    fi
else
    NUMFILES=$1
    WRITESTR=$2
    WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"
mkdir -p "$WRITEDIR"

for i in $( seq 1 $NUMFILES)
do
    writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

# Save finder.sh output to assignment4-result.txt
finder.sh "$WRITEDIR" "$WRITESTR" > /tmp/assignment4-result.txt
OUTPUTSTRING=$(cat /tmp/assignment4-result.txt)

# remove temporary directories
rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
    echo "success"
    exit 0
else
    echo "failed: expected ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
    exit 1
fi
