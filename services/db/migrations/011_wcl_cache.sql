-- Answers from WarcraftLogs, kept for a few hours.
--
-- The public API allows 3,600 points an hour. One rankings page costs one or two, so a panel
-- opened by many people for the same spec and fight should cost one query, not one each. The
-- stored body is the aggregate we serve, not WarcraftLogs' raw response: it is what the page
-- needs, and it keeps the table small.
CREATE TABLE wcl_cache (
    key        text        PRIMARY KEY,
    body       jsonb       NOT NULL,
    fetched_at timestamptz NOT NULL DEFAULT now()
);
