#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(dirname "${BASH_SOURCE}")"

# for dev work
if [[ -z ${PM_INSTALL_PATH+x} ]]; then
    PM_INSTALL_PATH="$SCRIPT_DIR/../packman"
fi

if [[ "$(uname)" == "Darwin" ]]; then
    TARGET="macos-universal"
else
    TARGET="linux-$(arch)"
fi


set +u
source "$PM_INSTALL_PATH/packman" pull "$SCRIPT_DIR/deps.packman.xml" -p "$TARGET"
set -u

export LD_LIBRARY_PATH=$PM_python_PATH/lib
export PYTHONPATH="${PM_jsonref_PATH}/python:${PM_repo_man_PATH}:${PM_repo_format_PATH}:${PM_MODULE_DIR}:${PYTHONPATH-}"
$PM_PYTHON -s -S -u "$SCRIPT_DIR/structuredlog.py" "$@"

