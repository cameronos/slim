#!/bin/sh

clear
echo "Building SLIM..."
rm slim.c
rm slim

echo "Pulling current SLIM code..."
curl -o slim.c https://raw.githubusercontent.com/cameronos/slim/refs/heads/main/slim.c
echo "Building executable..."
gcc -O2 -o slim slim.c -lX11 -lImlib2
sudo mv slim /usr/local/bin

echo "SLIM is ready to use, and was moved to /usr/local/bin."
