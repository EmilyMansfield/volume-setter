#!/usr/bin/env bash
tools_dir="./cmake-build-debug/vcpkg_installed/x64-windows/tools"
protos="./protos/declvol/v1/declvol.proto"
protoc="$tools_dir/protobuf/protoc.exe"

# gRPC is not currently used, but if it were then this is the command to run.
#"$protoc" -I ./protos/ --grpc_out src --plugin protoc-gen-grpc="$tools_dir/grpc/grpc_cpp_plugin.exe" $protos
"$protoc" -I ./protos/ --cpp_out src $protos
