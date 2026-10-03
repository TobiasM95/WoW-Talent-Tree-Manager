import { cached, convert, currentContent, entryIndex, failure, rankings, summarise, WclError, type Env, type Player, type Tree } from "../../_lib/wcl";

/**
 * What the top-ranked players of a specialisation run, in this tool's terms:
 * GET /api/popular/<spec tree key>?zone=&encounter=all|<id>&difficulty=&pages=
 *
 * The spec's trees are read from the site's own static data, the same files the page draws,
 * so each real build is also checked against exactly the trees a visitor sees.
 */
export const onRequestGet: PagesFunction<Env> = async ({ request, env, params }) => {
  const specKey = ([] as string[]).concat(params.spec ?? []).join("/");
  const url = new URL(request.url);
  const zone = Number(url.searchParams.get("zone"));
  const encounter = url.searchParams.get("encounter") ?? "all";
  const difficulty = Number(url.searchParams.get("difficulty") ?? 5);
  const pages = Math.min(3, Math.max(1, Number(url.searchParams.get("pages") ?? 1)));
  if (!Number.isInteger(zone) || zone <= 0) return failure(422, "zone is required");

  const asset = async <T>(path: string): Promise<T | null> => {
    const r = await env.ASSETS.fetch(new URL(path, request.url));
    return r.ok ? ((await r.json()) as T) : null;
  };
  const spec = await asset<Tree>(`/data/trees/${specKey}.json`);
  if (!spec || spec.kind !== "spec") return failure(404, `no current spec tree '${specKey}'`);
  const index = await asset<{ trees: { key: string; className: string; specName: string | null }[] }>("/data/retail/index.json");
  const keys = (index?.trees ?? []).filter((t) => t.className === spec.className && t.specName === spec.specName).map((t) => t.key);
  const trees = (await Promise.all(keys.map((k) => asset<Tree>(`/data/trees/${k}.json`)))).filter((t): t is Tree => t !== null);
  const byKey = new Map(trees.map((t) => [t.key, t]));

  try {
    return await cached(request, `popular:v3:${specKey}:${zone}:${encounter}:${difficulty}:${pages}`, 6 * 3600, async () => {
      const content = await currentContent(env);
      const info = content.find((z) => z.zoneId === zone);
      if (!info) throw new WclError(`zone ${zone} is not current content`, 404);
      const encounters = encounter === "all" ? info.encounters : info.encounters.filter((e) => String(e.id) === encounter);
      if (!encounters.length) throw new WclError(`no encounter ${encounter} in ${info.name}`, 404);
      const entries = entryIndex(trees);
      const players: Player[] = [];
      for (const enc of encounters) {
        for (const row of await rankings(env, spec.className, spec.specName!, enc.id, difficulty, pages)) {
          players.push(convert(row, entries));
        }
      }
      return summarise(players, byKey, info, encounters, difficulty);
    });
  } catch (e) {
    if (!(e instanceof WclError)) return failure(500, String(e));
    return failure(e.status, e.status === 502 ? `WarcraftLogs: ${e.message}` : e.message);
  }
};
