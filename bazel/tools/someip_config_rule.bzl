"""A macro for generating a binary SOME/IP configuration."""

def generate_someip_config(name, json, visibility = None):
    """Wraps a genrule to convert a .json config to a .bin file."""

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
