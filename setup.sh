#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BIN_DIR="$SCRIPT_DIR/bin/"

if [ -d "$BIN_DIR" ]; then
  # Add the directory to the PATH if it exists
  export PATH="$BIN_DIR:$PATH"
  echo "Added $BIN_DIR to PATH"
else
  echo "Error: $BIN_DIR does not exist."
fi