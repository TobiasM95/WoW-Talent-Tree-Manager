-- Keep a custom project's design itself, not only the trees derived from it.
--
-- The editor reopens a saved project from its design. Rebuilding that from the derived tree
-- records made the derivation part of the project's identity: when the derivation changed (a
-- choice node's name, in the first version), an opened project saved back as a *different*
-- project. The design is the source of truth; the tree records are regenerated from it.
ALTER TABLE custom_projects ADD COLUMN design jsonb;

-- Anything saved before this has no design to reopen from and was written by that first
-- derivation. There is nothing of value in it yet, so it goes.
DELETE FROM trees WHERE game = 'custom';
DELETE FROM custom_projects;
