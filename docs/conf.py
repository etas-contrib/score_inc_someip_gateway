# *******************************************************************************
# Copyright (c) 2024 Contributors to the Eclipse Foundation
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

import json
from pathlib import Path

project = "SOME/IP Gateway"
project_url = "https://eclipse-score.github.io/inc_someip_gateway/"
version = "0.1"

extensions = ["score_sphinx_bundle"]

# ==============================================================================
# Open SOME/IP specification: external needs with GitHub blob line links
# ==============================================================================
# The specification is published only as source on GitHub -- GitHub Pages is not
# enabled, so there are no rendered HTML anchors to link to. Its needs.json does
# carry `docname` and `lineno` for every need, so each requirement can be linked
# to the exact line of the blob instead.
#
# The commit is pinned: line numbers shift whenever upstream edits the files.
SPEC_REPO = "https://github.com/some-ip-com/open-someip-spec"
SPEC_COMMIT = "4860461802c23f41c6227befcb25cc80e7d68771"

needs_string_links = {
    "spec_ref": {
        "regex": r"^(?P<doc>[a-z0-9-]+):(?P<line>\d+)$",
        "link_url": f"{SPEC_REPO}/blob/{SPEC_COMMIT}/src/{{{{doc}}}}.rst#L{{{{line}}}}",
        "link_name": "{{doc}}.rst:{{line}}",
        "options": ["hash"],
    },
}
