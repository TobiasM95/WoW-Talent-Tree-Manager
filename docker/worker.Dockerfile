# The solve worker: the C++ engine plus a thin Python supervisor.
#
# Built in one image so the worker can exec the solver directly rather than orchestrating
# a second container per job. The engine stage is identical to docker/engine.Dockerfile.

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

FROM python:3.12-slim
ENV PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1
WORKDIR /app

COPY services/ingest/requirements.txt /tmp/ingest-requirements.txt
COPY services/db/requirements.txt /tmp/db-requirements.txt
RUN pip install --no-cache-dir -r /tmp/ingest-requirements.txt -r /tmp/db-requirements.txt

COPY --from=build /src/build/ttm-solver /usr/local/bin/ttm-solver
COPY services ./services
COPY tools ./tools

RUN useradd --create-home app && chown -R app:app /app
USER app

ENV TTM_SOLVER=/usr/local/bin/ttm-solver
CMD ["python", "services/worker/worker.py"]
