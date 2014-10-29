#!/bin/bash
echo "Content-Type: text/plain"
echo ""
echo "metaproxy/etc/cgi.sh"
pwd
env
if test $CONTENT_LENGTH; then
	echo "Echo content of length: $CONTENT_LENGTH:"
	read -n $CONTENT_LENGTH c
	echo "$c"
fi


