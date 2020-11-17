#! /bin/bash
# This script check if we have the tests patterns and download them if needed
# then run all available tests

# Do we have a patterns directory
if [ ! -d "patterns" ]; then
	rm -f ./bcg729-patterns.zip
   # no pattern directory: download it from 
   wget http://linphone.org/bc-downloads/bcg729-patterns-v1.1.0.zip
   if [ -e bcg729-patterns-v1.1.0.zip ]; then
	# check file
	if [[ `openssl md5 bcg729-patterns-v1.1.0.zip | grep -c f223c39eb471350124e56978760858f7` -ne 0 ]]; then
		# file ok, unzip it
		unzip bcg729-patterns-v1.1.0.zip
	   	if [[ $? -ne 0 ]]; then
			echo "Error: unable to unzip correctly bcg729-patterns-v1.1.0.zip, try to do it manually"
		else	
			rm bcg729-patterns-v1.1.0.zip
		fi
	else
		echo "Error: bad checksum on bcg729-patterns-v1.1.0.zip downloaded from http://linphone.org/bc-downloads/.\nTry again"
		exit 1
	fi
   else
	echo "Error: Unable to download bcg729-patterns-v1.1.0.zip pattern archive from http://linphone.org/bc-downloads/"
	exit 1
   fi
fi

# run all the tests
@CMAKE_CURRENT_BINARY_DIR@/testCampaign all 
