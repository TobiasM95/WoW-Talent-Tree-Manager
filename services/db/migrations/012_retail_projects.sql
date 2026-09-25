-- Retail-style custom projects: one class tree, up to four specs and six hero trees.
--
-- A project now has one style, as the games do. Classic keeps vanilla's three tabs; retail
-- has a class tree, one to four spec trees and up to six hero trees -- eleven at most. The
-- validator enforces the shape per style; this only stops the column refusing the larger one.
ALTER TABLE custom_projects DROP CONSTRAINT custom_projects_tree_count_check;
ALTER TABLE custom_projects ADD CONSTRAINT custom_projects_tree_count_check
    CHECK (tree_count BETWEEN 1 AND 11);
