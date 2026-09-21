# Build the React app, then serve the static output with Caddy.
#
# No Node at runtime: the result of `vite build` is files, and the thing serving them is
# already terminating TLS and proxying the API. One container instead of two.

FROM node:22-alpine AS build
WORKDIR /app

# Dependencies first, so a source edit does not re-resolve the tree.
COPY frontend/package.json frontend/package-lock.json ./
# `npm ci` needs the lockfile to match; omit dev-only browser binaries, which are for tests.
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
RUN npm ci

COPY frontend/ ./
RUN npm run build

FROM caddy:2-alpine AS runtime
COPY --from=build /app/dist /srv
COPY docker/Caddyfile /etc/caddy/Caddyfile
