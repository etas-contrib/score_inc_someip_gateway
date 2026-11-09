#!/usr/bin/env python3
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
"""Prepare the Open SOME/IP needs export for vendoring.

The specification is licensed Community-Spec-1.0, not Apache-2.0. Its needs
export carries no requirement prose, but the section headings and the headings'
titles are the specification's own wording. Nothing in this repository reads
them, so they are removed before the file is committed: the vendored file then
holds identifiers, source locations and attribute values only.

Not part of the documentation build. Run by hand as part of
re-pinning; see third_party/open_someip_spec/README.md.
"""

import json
import sys

# The only fields the documentation build reads: identity, where the
# requirement sits in the specification sources, and the attribute values the
# S-CORE metamodel needs. Everything else -- including the specification's own
# wording in `title`, `section_name` and `sections` -- is dropped.
KEEP = ("id", "type", "docname", "lineno", "reqtype", "security", "safety", "status")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} <upstream needs.json> <vendored needs.json>")
    src, dst = sys.argv[1], sys.argv[2]

    doc = json.load(open(src))
    dropped_needs = dropped_fields = 0
    for version in doc.get("versions", {}).values():
        kept = {}
        for key, need in version.get("needs", {}).items():
            if need.get("type") != "feat_req":
                dropped_needs += 1  # headings carry no requirement
                continue
            dropped_fields += sum(1 for f in need if f not in KEEP)
            kept[key] = {f: need[f] for f in KEEP if f in need}
        version["needs"] = kept
        version["needs_amount"] = len(kept)
        properties = version.get("needs_schema", {}).get("properties", {})
        for field in list(properties):
            if field not in KEEP:
                properties.pop(field)

    # Pretty-printed with sorted keys: the upstream build emits a single line,
    # which is unreviewable in a diff.
    json.dump(doc, open(dst, "w"), indent=1, sort_keys=True)
    open(dst, "a").write("\n")
    print(
        f"kept {sum(len(v['needs']) for v in doc['versions'].values())} needs, "
        f"dropped {dropped_needs} headings and {dropped_fields} unused fields"
    )


if __name__ == "__main__":
    main()
