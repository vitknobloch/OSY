#!/bin/bash

function print_help
{
    echo "HELP for hw02 script"
    echo "--------------------"
    echo "hw02 lets you search for PDF files, numberes lines of text, "
    echo "split text into sentences and add dirpath to your includes in C files"
    echo "--------------------"
    echo "options:"
    echo "-h -> display this help"
    echo "-a -> find all pdf files in current directory"
    echo "-b -> reads stdin and outputs only lines starting with numbers (without the numbers)"
    echo "-c -> reads stdin and outputs every sentence on a new line"
    echo "-d <prefix> -> reads C program from stdin and adds <prefix> before all include paths"
}

function find_pdf
{
    declare -a FILES
    for file in * .*
    do
        if [[ "$file" =~ .*\.([pP][dD][fF])$ ]]; then # [[ "$file" =~ .*\.((PDF)|(pdf))$ ]]; then
            FILES+=("$file")
        fi
    done

    for file in "${FILES[@]}"
    do
        echo "$file"
    done
}

function numbered_lines
{
    sed -n '/^[+-]\?[0-9]\+/p'| sed 's/^[+-]\?[0-9]\+//'
}

function sentence_newline
{
    tr '\n' ' ' | grep -o '[[:upper:]][^\.\?\!]*[\.\?\!]'
}

function includes
{
    IFS=$'\n'
    while read -r LINE
    do
    if [[ "$LINE" =~ ([[:space:]]*\#[[:space:]]*include[[:space:]]*<[^>\n]*>)|([[:space:]]*\#[[:space:]]*include[[:space:]]*\"[^\"\n]*\") ]]
    then
        sed "s/#[[:space:]]*include[[:space:]]*[<\"]/&${OPTARG//\//\\/}/" <<<"$LINE"
    else
        echo "$LINE"
    fi
    done
    unset IFS
}

OPT_USED=false
while getopts ":habcd:" opt;
do
    OPT_USED=true
    case $opt in
    h) #help
        print_help
        exit 0
        ;;
    a) #PDF files
        find_pdf
        exit 0
        ;;
    b) #numbered lines
        numbered_lines
        exit 0
        ;;
    c) #new-line sentences
        sentence_newline
        exit 0
        ;;
    d) #C includes
        includes
        exit 0
        ;;
    ?) #unkown opt
    print_help
    exit 1
        ;;
    esac
done

if [ "$OPT_USED" = "false" ]; then
    print_help
    exit 1
fi