-- Talent icons, cached one file per name.
--
-- Deliberately not the packed atlas the native app shipped. An atlas is a single blob that
-- must be regenerated whenever any one icon changes, cannot be cached per-icon by a browser,
-- and forces every client to download every icon to draw one tree. Individual files behind
-- an immutable cache header are what a web client actually wants.
--
-- In Postgres rather than a volume because the numbers say it is free: 2,094 distinct icons
-- across all 160 trees at ~2.3 KB each is about 5 MB, and keeping it here means one fewer
-- mount, no separate backup path, and the API already has a connection pool.
--
-- Icons are Blizzard's assets. Nothing here is committed to the repository or baked into an
-- image -- see docs/02-target/icons.md for the position and how to turn it off.
CREATE TABLE icons (
    name         text     NOT NULL,
    size         smallint NOT NULL,
    -- Upstream's HTTP status. Non-200 rows are the negative cache: without them, every
    -- request for an icon upstream does not have would re-ask upstream forever.
    status       smallint NOT NULL,
    content_type text,
    bytes        bytea,
    -- Content hash, so the API can answer a conditional GET without reading the bytes.
    etag         text,
    source       text     NOT NULL,
    attempts     smallint NOT NULL DEFAULT 1,
    fetched_at   timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (name, size),
    CONSTRAINT icons_body_matches_status
        CHECK ((status = 200) = (bytes IS NOT NULL)),
    CONSTRAINT icons_ok_rows_are_complete
        CHECK (status <> 200 OR (content_type IS NOT NULL AND etag IS NOT NULL)),
    -- A 200 that is 40 bytes long is an error page, not an icon. Upstream returning one
    -- should not poison the cache with something a client will fail to decode.
    CONSTRAINT icons_bytes_sane
        CHECK (bytes IS NULL OR octet_length(bytes) BETWEEN 256 AND 262144),
    CONSTRAINT icons_size_known CHECK (size IN (18, 36, 56)),
    -- Upstream paths are built from this name. Anything outside the alphabet Blizzard uses
    -- is either a transform bug or an attempt to walk out of the path.
    CONSTRAINT icons_name_shape CHECK (name ~ '^[a-z0-9_]{2,128}$')
);

COMMENT ON TABLE icons IS
    'Per-name icon cache. Rows with status <> 200 are the negative cache. Derived data:
     safe to TRUNCATE, refilled by services/db/sync_icons.py.';

-- The coverage query (/health) and the sync job both ask "which names have no usable row".
CREATE INDEX icons_usable_idx ON icons (name) WHERE status = 200;
