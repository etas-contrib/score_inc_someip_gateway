<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Open SOME/IP specification — needs export

`needs.json` is the sphinx-needs export of the Open SOME/IP specification,
pinned to upstream commit `4860461802c23f41c6227befcb25cc80e7d68771`.

It holds requirement identifiers, attribute values and source locations only.
The specification's own wording — requirement text, section and heading titles —
is not in this file and must never be committed here: the specification is
licensed Community-Spec-1.0, not Apache-2.0.

    Upstream: https://github.com/some-ip-com/open-someip-spec
    Licence:  SOME/IP Community Specification License (SPDX: Community-Spec-1.0)
              (c) Technica Engineering GmbH — NOT Apache-2.0.

## Re-pinning to a newer specification commit

A normal build runs none of this; the checked-in `needs.json` is its only input.
Follow the steps below only when moving to a newer upstream commit.

Set these two first, and the commands can then be pasted as they are:

    GATEWAY=<absolute path to this repository>
    COMMIT=<upstream commit to pin>

**1. Work outside the repository**, so the clone and the virtualenv cannot be
picked up by the build:

    mkdir -p ~/open-someip-repin && cd ~/open-someip-repin

**2. Clone the specification** and check out the commit being pinned:

    git clone https://github.com/some-ip-com/open-someip-spec.git spec
    git -C spec checkout "$COMMIT"

**3. Install the specification's own build dependencies** into a virtualenv:

    python3 -m venv venv
    ./venv/bin/pip install -r spec/requirements.txt

**4. Run the specification's sphinx build.** `needs_build_json=1` is what makes
it emit `build/needs.json`, the export this directory vendors:

    ./venv/bin/python -m sphinx -b html -D needs_build_json=1 spec/src build

**5. Strip the specification's wording and write the vendored file.** Use the
script, not `cp`: a plain copy carries the specification's prose, which the
licence does not allow us to commit here.

    python3 "$GATEWAY/tools/extneeds/repin_vendor_spec_needs.py" \
        build/needs.json "$GATEWAY/third_party/open_someip_spec/needs.json"

It prints how many requirements it kept and how much it dropped.

**6. Update the pinned commit in both places** that carry it, so the line numbers
in `needs.json` and the links built from them cannot drift apart:

  - `third_party/open_someip_spec/README.md` — the line near the top of this file
  - `docs/conf.py` — `SPEC_COMMIT`

**7. Rebuild the documentation and check it**, back in the repository:

    cd "$GATEWAY"
    bazel run //:docs        # writes the HTML to _build/
    bazel run //:docs_check  # metamodel and traceability checks

The mirrored requirements land in `_build/requirements/extspec/index.html`.

**8. Review the diff of `needs.json`.** The file is pretty-printed with sorted
keys for exactly this purpose; the upstream build emits it as a single line,
which is unreviewable.
