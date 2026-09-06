#!/bin/bash

set -e
BAZEL="${BAZEL:-bazel}"
EXECUTION_ROOT=$($BAZEL info execution_root)
$BAZEL build \
      --config win64 \
      --define EXECUTION_ROOT="$EXECUTION_ROOT" \
      --sandbox_debug \
      --workspace_status_command tools/buildstamp/get_workspace_status \
      -c opt :z2edit-windows "$@"

VERSION=$(grep BUILD_GIT_VERSION bazel-out/volatile-status.txt | cut -f2 -d' ')

cp bazel-bin/z2edit-windows.zip z2edit-windows-${VERSION}.zip
