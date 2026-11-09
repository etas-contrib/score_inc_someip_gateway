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

load("@rules_python//python:py_binary.bzl", "py_binary")

# Only generate_spec_requirements below takes part in the documentation build.
#
# repin_vendor_spec_needs.py has no target at all: it runs as a loose script
# when pinning a newer Open SOME/IP commit, see
# third_party/open_someip_spec/README.md.
#
# unused_convert_spec_needs is dead. It belongs to the earlier approach of
# importing the specification as sphinx-needs external needs, which
# generate_spec_requirements replaced. Kept only until someone decides whether
# that approach comes back.
py_binary(
    name = "unused_convert_spec_needs",
    srcs = ["unused_convert_spec_needs.py"],
    visibility = ["//visibility:public"],
)

# Dead with the script above: nothing consumes this genrule, and its pinned
# commit is read by nothing. Built only when named explicitly.
genrule(
    name = "spec_needs_json",
    srcs = ["//third_party/open_someip_spec:needs.json"],
    outs = ["spec_needs.json"],
    cmd = "$(execpath :unused_convert_spec_needs) $< $@ " +
          "4860461802c23f41c6227befcb25cc80e7d68771",
    tools = [":unused_convert_spec_needs"],
    visibility = ["//visibility:public"],
)

py_binary(
    name = "generate_spec_requirements",
    srcs = ["generate_spec_requirements.py"],
    visibility = ["//visibility:public"],
)
