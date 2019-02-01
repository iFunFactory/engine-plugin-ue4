# Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
#
# This work is confidential and proprietary to iFunFactory Inc. and
# must not be used, disclosed, copied, or distributed without the prior
# consent of iFunFactory Inc.

echo "It is a file to respond to the 4.21.2 CachedCPPEnvironments issue."
echo "Start changing file extension ".cc" to ".cpp""

root_path=${PWD}

echo "Checking exist funapi plugin folder"

command=$(find Plugins/ -name Funapi | wc -l)

if [ $command -eq 0 ]; then
  echo "Not exist funapi plugin folder"
  read -p "Press any key to exit"
  exit
fi

cd Plugins/Funapi/

find ./ -name "*.cc" -exec bash -c 'mv "$1" "${1%.cc}".cpp' - '{}' \;

cd ../..

echo "Check yours .cc files that in Source/ directory"
echo "That will .cc file change to .cpp"
read -p "Continue press any key "

cd Source

find ./ -name "*.cc" -exec bash -c 'mv "$1" "${1%.cc}".cpp' - '{}' \;