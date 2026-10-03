import { build, canonical, CustomTreeError, projectId } from "../../../src/engine/custom";
import { failure, json, type Env } from "../../_lib/wcl";

const BODY_LIMIT = 400_000;

/**
 * Save a project from the tree editor. Content-addressed: the id is the hash of the design,
 * so saving the same design twice is the same project and an edit is a new one. Nothing is
 * overwritten, which is why a stored design is never written twice (KV writes are the scarce
 * part of the free tier, reads are not).
 */
export const onRequestPost: PagesFunction<Env> = async ({ request, env }) => {
  const body = await request.text();
  if (body.length > BODY_LIMIT) return failure(413, `a project is at most ${BODY_LIMIT / 1000} kB`);
  let canon;
  try {
    canon = canonical(JSON.parse(body));
  } catch (e) {
    if (e instanceof SyntaxError) return failure(400, "the body is not JSON");
    if (e instanceof CustomTreeError) return failure(400, e.message);
    throw e;
  }
  const pid = await projectId(canon);
  const key = `project:${pid}`;
  if ((await env.PROJECTS.get(key)) === null) await env.PROJECTS.put(key, JSON.stringify(canon));
  const trees = build(pid, canon);
  return json({ project: pid, name: canon.name, sharedPointCap: canon.sharedPointCap, trees, warnings: [] }, 201);
};
