#!/bin/bash

# Lanceur automatique de audioclient et audioserver
make
if [ $? -eq 0 ]; then
	xterm -e "./audioserver" &
	sleep 1
	xterm -e "padsp ./audioclient"
fi
