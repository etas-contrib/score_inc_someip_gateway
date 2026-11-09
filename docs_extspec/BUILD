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

load("@score_docs_as_code//:docs.bzl", "docs_bundle")

# Mirrored Open SOME/IP requirements, generated from the vendored export so the
# source locations are never written by hand.
genrule(
    name = "spec_requirements_rst",
    srcs = ["//third_party/open_someip_spec:needs.json"],
    outs = ["index.rst"],
    cmd = "$(execpath //tools/extneeds:generate_spec_requirements) $< $@",
    tools = ["//tools/extneeds:generate_spec_requirements"],
)

docs_bundle(
    name = "docs_bundle",
    data = [":spec_requirements_rst"],
    visibility = ["//visibility:public"],
)
