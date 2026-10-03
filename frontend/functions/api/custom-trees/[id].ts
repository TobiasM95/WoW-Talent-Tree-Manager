import { build, type Canon } from "../../../src/engine/custom";
import { failure, json, type Env } from "../../_lib/wcl";

/** A saved project: its design, to keep editing, and its trees, built from it, to plan with. */
export const onRequestGet: PagesFunction<Env> = async ({ env, params }) => {
  const pid = String(params.id);
  if (!/^[0-9a-f]{16}$/.test(pid)) return failure(400, "not a project id");
  const stored = await env.PROJECTS.get(`project:${pid}`);
  if (stored === null) return failure(404, `no custom project '${pid}'`);
  const design = JSON.parse(stored) as Canon;
  return json({ project: pid, name: design.name, sharedPointCap: design.sharedPointCap, trees: build(pid, design), design });
};
