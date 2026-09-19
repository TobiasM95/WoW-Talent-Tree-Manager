# Builds the TTM solver as a standalone worker binary.
#
# The engine is C++17 with no third-party dependencies -- no SIMD, no MSVC
# intrinsics, no libcurl in the Engine/CLI translation units -- so this needs
# nothing but a compiler and CMake.

FROM debian:bookworm-slim AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential cmake \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY Engine/src ./Engine/src
COPY CLI ./CLI

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j "$(nproc)"

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
 && apt-get install -y --no-install-recommends libstdc++6 \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --create-home --shell /usr/sbin/nologin ttm \
 && mkdir -p /var/lib/ttm /work \
 && chown -R ttm:ttm /var/lib/ttm /work

COPY --from=build /src/build/ttm-solver /usr/local/bin/ttm-solver

# Resources are mounted, not baked: presets change every patch and the image
# should not need rebuilding for a data update.
#
# The engine resolves a data directory unconditionally on startup (it was written
# for a desktop app that always had one), so this must exist and be writable even
# for a solve that reads its tree from an explicit --structure-file-path.
ENV TTM_DATA_DIR=/var/lib/ttm

USER ttm
WORKDIR /work
ENTRYPOINT ["/usr/local/bin/ttm-solver"]
