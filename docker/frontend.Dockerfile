# Build the React app, then serve the static output with Caddy.
#
# No Node at runtime: the result of `vite build` is files, and the thing serving them is
# already terminating TLS and proxying the API. One container instead of two.

FROM node:22-alpine AS build
WORKDIR /app

# pnpm, at the version package.json pins. Corepack ships with Node, so the version that
# builds the image is the version named in the repo rather than whatever the base image
# happens to carry.
COPY frontend/package.json ./
RUN corepack enable && corepack install

# Dependencies first, so a source edit does not re-resolve the tree.
COPY frontend/pnpm-lock.yaml ./
# --frozen-lockfile fails rather than quietly resolving something new, which is the whole
# point of building from a lockfile. Playwright's browsers are for the test suites and have
# no business in an image that only runs vite build.
ENV PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1
RUN pnpm install --frozen-lockfile

COPY frontend/ ./
RUN pnpm run build

FROM caddy:2-alpine AS runtime
COPY --from=build /app/dist /srv
COPY docker/Caddyfile /etc/caddy/Caddyfile
