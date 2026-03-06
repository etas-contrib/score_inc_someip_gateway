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

# Prerequisites:
#   - lcov / genhtml installed:  sudo apt-get install -y lcov
#   - Bazel configured with the x86_64-linux toolchain
#
# Usage:
#   ./run_coverage_local.sh

set -e

REPORT_DIR="cpp_coverage"

# UT targets — these are tagged "manual" so they must be named explicitly.
# They are standalone GoogleTest binaries that run natively on Linux.
UT_TARGETS=(
    "//tests/UT/LocalServiceInstance_UT:LocalServiceInstance_UT"
    "//tests/UT/RemoteServiceInstance_UT:RemoteServiceInstance_UT"
)

echo "============================================================"
echo " Running code coverage analysis (x86_64-linux)"
echo "============================================================"
echo ""
echo "Targets:"
for t in "${UT_TARGETS[@]}"; do
    echo "  - ${t}"
done
echo ""

# Resolve llvm-cov from the LLVM toolchain in the Bazel output base.
# This is needed by collect_cc_coverage.sh to export profdata → lcov.
LLVM_COV_PATH=$(find "$(bazel info output_base 2>/dev/null)/external" \
    -path "*/toolchains_llvm*/bin/llvm-cov" 2>/dev/null | head -1)

if [ -z "${LLVM_COV_PATH}" ]; then
    echo "Warning: Could not auto-detect llvm-cov path."
    echo "         Coverage may fall back to profdata format."
    echo ""
    LLVM_COV_ARGS=()
else
    echo "Using llvm-cov: ${LLVM_COV_PATH}"
    echo ""
    LLVM_COV_ARGS=(--test_env=LLVM_COV="${LLVM_COV_PATH}")
fi

bazel coverage \
    --config=x86_64-linux \
    --nocheck_visibility \
    "${LLVM_COV_ARGS[@]}" \
    "${UT_TARGETS[@]}"

echo ""
echo "============================================================"
echo " Generating HTML coverage report"
echo "============================================================"

OUTPUT_PATH=$(bazel info output_path)
COVERAGE_DAT="${OUTPUT_PATH}/_coverage/_coverage_report.dat"

if [ ! -f "${COVERAGE_DAT}" ]; then
    echo "Error: Coverage data file not found at ${COVERAGE_DAT}"
    echo ""
fi

genhtml "${COVERAGE_DAT}" \
    -o="${REPORT_DIR}" \
    --show-details \
    --legend \
    --function-coverage \
    --branch-coverage

echo ""
echo "============================================================"
echo " Coverage report generated successfully!"
echo " Location: ${REPORT_DIR}/index.html"
echo "============================================================"

# Open in browser if possible
if command -v xdg-open &> /dev/null; then
    xdg-open "${REPORT_DIR}/index.html"
fi
