#!/bin/bash
       
#Title           :assignment1_package.sh
#description     :This script will package assignment1 for submission.
#Author		     :Suvigya Vijay <suvigyav@buffalo.edu>
#Version         :2.0
#===================================================================================================

# https://gist.github.com/davejamesmiller/1965569
function ask {
    while true; do
 
        if [ "${2:-}" = "Y" ]; then
            prompt="Y/n"
            default=Y
        elif [ "${2:-}" = "N" ]; then
            prompt="y/N"
            default=N
        else
            prompt="y/n"
            default=
        fi
 
        # Ask the question
        read -p "$1 [$prompt] " REPLY
 
        # Default?
        if [ -z "$REPLY" ]; then
            REPLY=$default
        fi
 
        # Check if the reply is valid
        case "$REPLY" in
            Y*|y*) return 0 ;;
            N*|n*) return 1 ;;
        esac
 
    done
}

echo
echo -n "Enter your Github Classroom Team Name and press [ENTER]: "
read ubitname

if [ -d "./pa1" ]; 
then
    echo "Directory with given pa1 exists"
else
    echo "No directory named pa1 found. Try again!"
    exit 0
fi

echo "Verifying contents ..."

echo
echo "C/C++ file with main function: "
FILE=`find ./pa1/src/ -name "assignment1.c" -o -name "assignment1.cpp"`
if [ -n "$FILE" ];
then
    echo "File $FILE exists"
    if grep -q "int main[ ]*([^\)]*)" $FILE
    then
        echo "File $FILE contains a 'int main()' function definition"
    else
        echo "File $FILE does NOT contain the 'int main()' function definition"
        exit 0
    fi
else
    echo "Missing main C file or file named incorrectly!"
    exit 0
fi

echo
echo "Makefile: "
FILE=./pa1/Makefile
if [ -f $FILE ];
then
    echo "File $FILE exists"
else
    echo "Missing Makefile or file named incorrectly!"
    exit 0
fi

echo
echo "Packaging ..."
cd pa1/ && tar --exclude='./logs' -zcvf ../${ubitname}_pa1.tar * && cd ..
echo "Done!"
echo
echo "!!!IMPORTANT: Your submission is NOT done!!!"
echo "Please follow the instructions on Piazza to submit your assignment."