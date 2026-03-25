#!/bin/bash
# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
# SPDX-License-Identifier: Apache-2.0
# Demo script to run all serializer plugins

set -e

echo "=== Building all serializer plugins ==="
bazel build //examples_serializers/serializer_plugin_static:static_demo \
            //examples_serializers/serializer_plugin_dlopen:dlopen_demo \
            //examples_serializers/serializer_plugin_dlopen:libbig_endian_plugin.so \
            //examples_serializers/serializer_plugin_service:service_demo \
            //examples_serializers/serializer_plugin_ipc:ipc_host \
            //examples_serializers/serializer_plugin_ipc:serializer_child

echo ""
echo "=== Testing Static Plugin (Big Endian) ==="
./bazel-bin/examples_serializers/serializer_plugin_static/static_demo big_endian

echo ""
echo "=== Testing Static Plugin (Little Endian) ==="
./bazel-bin/examples_serializers/serializer_plugin_static/static_demo little_endian

echo ""
echo "=== Testing DLOpen Plugin ==="
./bazel-bin/examples_serializers/serializer_plugin_dlopen/dlopen_demo \
  ./bazel-bin/examples_serializers/serializer_plugin_dlopen/libbig_endian_plugin.so

echo ""
echo "=== Testing Service Plugin ==="
./bazel-bin/examples_serializers/serializer_plugin_service/service_demo

echo ""
echo "=== Testing IPC Plugin ==="
./bazel-bin/examples_serializers/serializer_plugin_ipc/ipc_host \
  ./bazel-bin/examples_serializers/serializer_plugin_ipc/serializer_child

echo ""
echo "=== Running Unit Tests ==="
bazel test //examples_serializers:serializer_plugin_test

echo ""
echo "=== All serializer plugins working! ==="
