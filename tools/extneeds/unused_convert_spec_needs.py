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
"""Convert the Open SOME/IP needs.json into a form the S-CORE metamodel accepts.

Unused. Nothing runs this script: no target consumes the `spec_needs_json`
genrule that wraps it, and the re-pinning procedure in
third_party/open_someip_spec/README.md does not call it either. It belongs to
the earlier approach of importing the specification as sphinx-needs external
needs, which `generate_spec_requirements.py` replaced by mirroring each
requirement as a local `feat_req`.

Caveats, both deliberate decisions rather than mechanical translation:

* Upstream records `safety: TBD` and `security: TBD`. The metamodel rejects
  TBD, so they are mapped to QM / NO. That assigns a safety classification to
  someone else's requirements and should be signed off, not assumed.
* The vendored input holds identifiers, source locations and attribute values
  only; tools/extneeds/repin_vendor_spec_needs.py drops the specification's own
  wording before the file is committed.
"""

import argparse
import json
import re

REQTYPE = {"Requirement": "Functional", "Information": "Non-Functional"}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("src", help="needs.json produced by the specification's own sphinx build")
    ap.add_argument("dst", help="converted needs.json to write")
    ap.add_argument("commit", help="specification commit the source build was made from")
    args = ap.parse_args()

    doc = json.load(open(args.src))
    version = next(iter(doc["versions"]))
    needs = doc["versions"][version]["needs"]

    out = {}
    for need in needs.values():
        match = re.fullmatch(r"feat_req_(someip[a-z]*)_(\d+)", need["id"])
        if not match:
            raise SystemExit(f"unexpected identifier {need['id']!r} in vendored needs")
        if not need.get("lineno") or not need.get("docname"):
            raise SystemExit(f"{need['id']}: missing docname/lineno, blob link would be wrong")

        need = dict(need)
        need["id"] = f"feat_req__extspec__{match.group(1)}_{match.group(2)}"
        # sphinx-needs requires a non-null title. Upstream titles are a single
        # emoji, and the headings' titles are the specification's own wording
        # and are stripped before vendoring, so derive one from the identifier.
        need["title"] = f"Open SOME/IP {match.group(1)} {match.group(2)}"
        need["reqtype"] = REQTYPE.get(need.get("reqtype"), "Non-Functional")
        if need.get("security") == "TBD":
            need["security"] = "NO"
        if need.get("safety") == "TBD":
            need["safety"] = "QM"
        out[need["id"]] = need

    doc["versions"][version]["needs"] = out
    doc["versions"][version]["needs_amount"] = len(out)
    # Read back by docs/conf.py, so the blob links can never point at a
    # different commit than the one the line numbers were taken from.
    doc["spec_commit"] = args.commit

    json.dump(doc, open(args.dst, "w"), indent=1)
    print(f"converted {len(out)} needs, pinned at {args.commit}")


if __name__ == "__main__":
    main()
