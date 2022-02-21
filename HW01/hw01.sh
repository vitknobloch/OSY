#!/bin/bash
ERROR_PRINTED=false
DO_ZIP=false
declare -a ALL_FILES

#process options
while getopts ":zh" opt
do
case $opt in
h) #help
    echo "HELP for hw01 script"
    echo "--------------------"
    echo "hw01 lets you find information about file type and content."
    echo "Run the script and then type 'PATH ' and a path (relative or absolute) and hit ENTER"
    echo "The script will display whether the path is valid and what kind of file it points to"
    echo "You can use this repeatedly, end the script with EOF signal (ctrl+D)"
    echo "--------------------"
    echo "options:"
    echo "-h -> display this help"
    echo "-z -> wrap all found files to archive 'output.tgz' in current working directory"
    exit 0
    ;;
z) #zip files
    DO_ZIP=true
    ;;
?) exit 2
;;
esac
done

#process input
while read -r LINE
do
if [ "${LINE:0:5}" = "PATH " ]
then
    #extract path
    L_PATH="${LINE#PATH }"
    #check path existance
    if stat "$L_PATH" >/dev/null 2>/dev/null
    then
        #get file type
        TYPE=$(stat -c %A "$L_PATH")
        #check for read permission
        if [ "${TYPE:1:1}" = "-" ]
        then
            exit 2
        fi
        #extract file type
        TYPE="${TYPE:0:1}"
        case $TYPE in
        -) #file
        LINE_COUNT=$(wc -l <"$L_PATH")
        if [ "$LINE_COUNT" -gt 0 ]
        then
        FIRST_LINE=$(head -n 1 "$L_PATH")
        else
        FIRST_LINE=""
        fi
        ALL_FILES+=("$L_PATH")
        echo "FILE '$L_PATH' $LINE_COUNT '$FIRST_LINE'"
        ;;
        d) #directory
        echo "DIR '$L_PATH'"
        ;;
        l) #link
        REAL_PATH=$(readlink "$L_PATH")
        echo "LINK '$L_PATH' '$REAL_PATH'"
        ;;
        esac
    else #path doesn't exist
        ERROR_PRINTED=true
        echo "ERROR '$L_PATH'" >&2
    fi
fi

done

#create zip file if requested
if [ $DO_ZIP = "true" ] && [ ${#ALL_FILES[*]} -gt 0 ]
then
    tar czf output.tgz "${ALL_FILES[@]}" >/dev/null 2>/dev/null
fi

#output 1 if a searched path didn't exist
if [ $ERROR_PRINTED = "true" ]
then
    exit 1
else
    exit 0
fi