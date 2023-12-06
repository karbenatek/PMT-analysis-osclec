#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BIN_DIR="$SCRIPT_DIR/bin/"
PY_DIR="$SCRIPT_DIR/scripts/"

if [ -d "$BIN_DIR" ]; then
  # Add the directory to the PATH if it exists
  export PATH="$BIN_DIR:$PATH"
  echo "Added $BIN_DIR to PATH"
else
  echo "Error: $BIN_DIR does not exist."
fi

if [ -d "$PY_DIR" ]; then
  # Add the directory to the PATH if it exists
  export PATH="$PY_DIR:$PATH"
  echo "Added $PY_DIR to PATH"
else
  echo "Error: $PY_DIR does not exist."
fi

# activate Python venv environment that meets INFN electronics dependencies
. ~/.venvs/PythonINFN/bin/activate

# mb check if environment exists and create it if not :) ... makefile thing
# pip install argparse os time sys cmd2 functools getpass numpy
# struct minimalmodbus==1.0.2
