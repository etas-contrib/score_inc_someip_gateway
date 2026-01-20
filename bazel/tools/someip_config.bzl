"""A macro for generating a binary SOME/IP configuration."""

def generate_someip_config(name, json, visibility = "//visibility:private"):
    """Wraps a genrule to convert a .json config to a .bin file.

    Args:
        name: Name of the test target
        json: Label or path to the JSON file to validate
        visibility: The visibility to use. (default: //visibility:private)
    """

    base_filename = json.split("/")[-1]
    output_filename = base_filename.replace(".json", ".bin")

    native.genrule(
        name = name,
        srcs = [
            json,
            "//src/gatewayd:etc/gatewayd_config.fbs",
        ],
        outs = [output_filename],
        cmd = """
            $(location @flatbuffers//:flatc) --binary \\
                -o $(@D) \\
                $(location //src/gatewayd:etc/gatewayd_config.fbs) \\
                $(location {json_label})
        """.format(
            json_label = json,
        ),
        tools = ["@flatbuffers//:flatc"],
        message = "Generating binary config from " + json,
        visibility = visibility,
    )

"""Macro for validating SOME/IP configuration files against JSON schemas."""

def validate_someip_config_test(name, json, expect_failure = False, visibility = "//visibility:private"):
    """Validates a JSON configuration file against its corresponding schema.

    Args:
        name: Name of the test target
        json: Label or path to the JSON file to validate
        expect_failure: If True, the test passes when validation fails (default: False)
        visibility: The visibility to use. (default: //visibility:private)
    """

    schema = "//src/gatewayd:etc/gatewayd_config.schema.json"

    native.sh_test(
        name = name,
        size = "small",
        srcs = ["//bazel/tools/validators:json_schema_validator.sh"],
        args = [
            "$(location %s)" % json,
            "$(location %s)" % schema,
            "true" if expect_failure else "false",
        ],
        data = [
            json,
            schema,
        ],
        tags = ["lint"],
        visibility = visibility,
    )
