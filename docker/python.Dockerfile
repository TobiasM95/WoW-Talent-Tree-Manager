# Shared image for the Python services (migrate, ingest, loader).
# One image rather than three: they share nearly all dependencies, and a single layer
# cache keeps rebuilds fast during development.

FROM python:3.12-slim

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1

WORKDIR /app

COPY services/ingest/requirements.txt /tmp/ingest-requirements.txt
COPY services/db/requirements.txt /tmp/db-requirements.txt
COPY services/api/requirements.txt /tmp/api-requirements.txt
RUN pip install --no-cache-dir         -r /tmp/ingest-requirements.txt         -r /tmp/db-requirements.txt         -r /tmp/api-requirements.txt

# Source is bind-mounted in compose so edits do not need a rebuild; copied here too so
# the image is runnable on its own.
COPY services ./services
COPY tools ./tools

RUN useradd --create-home app && chown -R app:app /app
USER app
