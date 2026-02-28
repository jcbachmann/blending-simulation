FROM debian:bookworm-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
 && rm -rf /var/lib/apt/lists/*

COPY . .

RUN mkdir -p build \
 && cd build \
 && cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VISUALIZER=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_CLI=ON \
    -DBUILD_FAST_SIMULATOR=ON \
    -DBUILD_DETAILED_SIMULATOR=ON \
 && cmake --build . --target BlendingSimulatorCli -j$(nproc) \
 && strip BlendingSimulatorCli/BlendingSimulatorCli

FROM gcr.io/distroless/cc

WORKDIR /app

COPY --from=builder /workspace/build/BlendingSimulatorCli/BlendingSimulatorCli ./

USER nonroot:nonroot

ENTRYPOINT ["./BlendingSimulatorCli"]