"""A macro for generating a SOME/IP configuration binary."""

def generate_someip_config_bin(name, json_file_path, output_filename, visibility = "//visibility:private"):
    """Generates a SOME/IP Config binary based on the specified JSON file.

    Args:
        name: Name of the test target
        json_file_path: Label or path to the JSON file to validate
        output_filename: The name of the file to be used for the output
        visibility: The visibility to use. (default: //visibility:private)
    """

    base_filename = json_file_path.split("/")[-1]
    bin_filename = base_filename.replace(".json", ".bin")

    commands = [
        "$(location @flatbuffers//:flatc) --binary",
        "-o $(@D)",
        "$(location @score_someip_gateway//src/gatewayd:etc/gatewayd_config.fbs)",
        "$(location %s)" % json_file_path,
    ]

    if not output_filename.endswith(".bin"):
        fail("The output_filename must end with '.bin'")

    if bin_filename != output_filename:
        rename_command = "&& mv $(@D)/%s $(@D)/%s" % (bin_filename, output_filename)
        commands.append(rename_command)

    native.genrule(
        name = name,
        srcs = [
            json_file_path,
            "@score_someip_gateway//src/gatewayd:etc/gatewayd_config.fbs",
        ],
        outs = [output_filename],
        cmd = " ".join(commands),
        tools = ["@flatbuffers//:flatc"],
        message = "Generating binary config from " + json_file_path,
        visibility = visibility,
    )

"""Macro for validating a SOME/IP JSON configuration file against its schema."""

def validate_someip_config_test(name, json_file_path, expect_failure = False, visibility = "//visibility:private"):
    """Validates a SOME/IP JSON configuration file against its corresponding schema.

    Args:
        name: Name of the test target
        json_file_path: Label or path to the JSON file to validate
        expect_failure: If True, the test passes when validation fails (default: False)
        visibility: The visibility to use. (default: //visibility:private)
    """

    schema = "@score_someip_gateway//src/gatewayd:etc/gatewayd_config.schema.json"

    native.sh_test(
        name = name,
        size = "small",
        srcs = ["@score_someip_gateway//bazel/tools/validators:json_schema_validator.sh"],
        args = [
            "$(location %s)" % json_file_path,
            "$(location %s)" % schema,
            "true" if expect_failure else "false",
        ],
        data = [
            json_file_path,
            schema,
        ],
        tags = ["lint"],
        visibility = visibility,
    )
