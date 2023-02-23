#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(dirname "${BASH_SOURCE}")"

# for dev work
if [[ -z ${PM_INSTALL_PATH+x} ]]; then
    PM_INSTALL_PATH="$SCRIPT_DIR/../packman"
fi

set +u
source "$PM_INSTALL_PATH/packman" pull "$SCRIPT_DIR/deps.packman.xml" -p "linux-$(arch)"
set -u

export LD_LIBRARY_PATH=$PM_python_PATH/lib
export PYTHONPATH="$SCRIPT_DIR/../../_build/target-deps/jsonref/python"

$PM_PYTHON -I -m pip install flake8

# ignore style errors that don't really matter
# C901: function too complex
# E722: don't use bare except
# E722: whitespace before :
# E305: not enough blank lines between things
# E302: not enough blank lines between things
# E303: too many blank lines
# W391: blank line at end of file
# E261: at least two spaces before inline comment
$PM_PYTHON -I -m flake8 "$SCRIPT_DIR/structuredlog.py" --extend-ignore C901,E203,E722,E305,E302,E303,W391,E261

