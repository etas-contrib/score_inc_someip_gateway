
# SOME/IP Gateway

The gateway is divided into a gateway daemon (gatewayd) which contains the network-independent logic (payload serialization, etc.) and the SOME/IP daemon (someipd) which binds to the concrete SOME/IP stack.
The IPC interface between the gatewayd and the someipd serves as isolation boundary between ASIL and QM context and also allows to replace the network stack without touching the main gateway logic.

![SOME/IP Gateway Architecture](docs/architecture/score-someip-car-window-overview.drawio.png)


---

## 🚀 Getting Started

### Clone the Repository

```sh
git clone https://github.com/eclipse-score/inc_someip_gateway.git
cd inc_someip_gateway
```

### Start the daemons

Start the daemons in this order:

```sh
bazel run //src/gatewayd
```

and in a separate terminal

```sh
bazel run //src/someipd
```

### Run Example app

```sh
bazel run //examples/car_window_sim:car_window_controller
```

If you type `open` or `close` the command will be sent via network.


### Dockerized integration test POC

For integration tests, a docker based approach was taken.
As a proof of concept `docker compose` can be used to build, setup and run the containers.
In the future a pytest based setup can be implemented to orchestrate the containers.

Build the docker containers:

```sh
docker compose --project-directory tests/integration/docker_setup/ build
```

Start up the containers:

```sh
docker compose --project-directory tests/integration/docker_setup/ up
```

Those containers are pre-configured (IP adresses, multicast route, ...).
The someipd-1 container already starts up the `gatewayd` and the `someipd`.

In Wireshark the network traffic can be seen by capturing on `any` with `ip.addr== 192.168.87.2 || ip.addr ==192.168.87.3`.

On the client side, start up the `sample_client` in another shell:

```sh
docker exec -it --env VSOMEIP_CONFIGURATION=/home/source/tests/integration/sample_client/vsomeip.json docker_setup-client-1 /home/source/bazel-bin/tests/integration/sample_client/sample_client
```

Finally start the benchmark on the someipd-1 container in a third shell:

```sh
docker exec -it docker_setup-someipd-1 /home/source/bazel-bin/tests/performance_benchmarks/ipc_benchmarks
```

### Gatewayd Config Schema Validation

#### Generate a new Gatewayd Config Schema
To generate a Gatewayd Config Schema make sure you have the latest `gatewayd_config.fbs` and `gatewayd_config.json` in `src/gatewayd/etc/` and execute the following bazel command:
```
bazel build //src/gatewayd:generate_gatewayd_config_schema
```

This should lead to the following output:
```
vscode ➜ /workspaces/inc_someip_gateway (main) $ bazel build //src/gatewayd:generate_gatewayd_config_schema
INFO: Analyzed target //src/gatewayd:generate_gatewayd_config_schema (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //src/gatewayd:generate_gatewayd_config_schema up-to-date:
  bazel-bin/src/gatewayd/gatewayd_config.schema.json
INFO: Elapsed time: 0.261s, Critical Path: 0.01s
INFO: 2 processes: 1 internal, 1 linux-sandbox.
INFO: Build completed successfully, 2 total actions
```

After the task ran successful you can retrieve the schema file from `bazel-bin/src/gatewayd/gatewayd_config.schema.json`. Copy this over to `src/gatewayd/` and overwrite the existing file.

#### Validate Gatewayd Config against Schema
To validate the `gatewayd_config.json` against the schema make sure it is placed in `src/gatewayd/gatewayd_config.json` and execute the following bazel command:
```
bazel test //src/gatewayd:gatewayd_config_schema_validation
```

In case of a successful run it should print the following:
```
vscode ➜ /workspaces/inc_someip_gateway (main) $ bazel test //src/gatewayd:gatewayd_config_schema_validation
INFO: Analyzed target //src/gatewayd:gatewayd_config_schema_validation (0 packages loaded, 0 targets configured).
INFO: Found 1 test target...
Target //src/gatewayd:gatewayd_config_schema_validation up-to-date:
  bazel-bin/src/gatewayd/gatewayd_config_schema_validation
INFO: Elapsed time: 0.285s, Critical Path: 0.05s
INFO: 2 processes: 2 linux-sandbox.
INFO: Build completed successfully, 2 total actions
//src/gatewayd:gatewayd_config_schema_validation                         PASSED in 0.0s

Executed 1 out of 1 test: 1 test passes.
```

In case of a failed test it will output something comparably to the following:
```
vscode ➜ /workspaces/inc_someip_gateway (main) $ bazel test //src/gatewayd:gatewayd_config_schema_validation
INFO: Analyzed target //src/gatewayd:gatewayd_config_schema_validation (0 packages loaded, 0 targets configured).
FAIL: //src/gatewayd:gatewayd_config_schema_validation (Exit 1) (see /var/cache/bazel/00be67bdc422f509e09cd5f794aa5d1b/execroot/_main/bazel-out/k8-fastbuild/testlogs/src/gatewayd/gatewayd_config_schema_validation/test.log)
INFO: From Testing //src/gatewayd:gatewayd_config_schema_validation:
==================== Test output for //src/gatewayd:gatewayd_config_schema_validation:
ERROR: '/local_service_instances/0' - '{"events":[{"event_name":"window_control"}],"instance_specifier":"gatewayd/application_window_control","invalid_attribute":"an invalid attribute"}': validation failed for additional property 'invalid_attribute': instance invalid as per false-schema
schema validation failed
Test Failed: Json validator performed json validation of 'src/gatewayd/etc/gatewayd_config.json' against 'src/gatewayd/etc/gatewayd_config.schema.json' schema with exit code 1, meanwhile rule attribute 'expected_failure' is equal to 'false'.
This means in case expected_failure=false then expected exit code should be zero (valid json). Otherwise, if expected_failure=true then exit code is non-zero (invalid json).
Note: by default expected_failure is false.
================================================================================
INFO: Found 1 test target...
Target //src/gatewayd:gatewayd_config_schema_validation up-to-date:
  bazel-bin/src/gatewayd/gatewayd_config_schema_validation
INFO: Elapsed time: 0.270s, Critical Path: 0.05s
INFO: 2 processes: 2 linux-sandbox.
INFO: Build completed, 1 test FAILED, 2 total actions
//src/gatewayd:gatewayd_config_schema_validation                         FAILED in 0.0s
  /var/cache/bazel/00be67bdc422f509e09cd5f794aa5d1b/execroot/_main/bazel-out/k8-fastbuild/testlogs/src/gatewayd/gatewayd_config_schema_validation/test.log

Executed 1 out of 1 test: 1 fails locally.
```

If closer inspection is needed the log file of the run can be found in `testlogs/src/gatewayd/gatewayd_config_schema_validation/test.log`.
