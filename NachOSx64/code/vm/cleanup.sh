#!/bin/bash

#delete all .o
rm nachos
echo "deleting nachos executable"
rm -f *.o
echo "deleting object files"

#prompt to user that cleanup is finished
echo "Cleanup completed."