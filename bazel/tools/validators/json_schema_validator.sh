#!/bin/bash
set -euo pipefail

JSON_FILE="$1"
SCHEMA_FILE="$2"
EXPECT_FAILURE="${3:-false}"

# Check if jsonschema is available
if ! python3 -c "import jsonschema" 2>/dev/null; then
    echo "Error: python3-jsonschema is not installed."
    echo "Install it with: sudo apt install python3-jsonschema"
    exit 2
fi

RESULT=0
python3 << EOF || RESULT=\$?
import json
import sys
try:
    import jsonschema
    with open('$JSON_FILE') as json_file, open('$SCHEMA_FILE') as schema_file:
        instance = json.load(json_file)
        schema = json.load(schema_file)
        jsonschema.validate(instance, schema)
    print('Validation passed')
except jsonschema.ValidationError as e:
    print(f'Validation failed: {e.message}')
    sys.exit(1)
except Exception as e:
    print(f'Error: {e}')
    sys.exit(2)
EOF

if [ "$EXPECT_FAILURE" = "true" ]; then
    if [ $RESULT -eq 0 ]; then
        echo "Expected validation to fail, but it passed!"
        exit 1
    else
        echo "Validation failed as expected"
        exit 0
    fi
else
    exit $RESULT
fi
