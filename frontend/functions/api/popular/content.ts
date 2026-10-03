import { cached, currentContent, failure, WclError, type Env } from "../../_lib/wcl";

/** The raids and Mythic+ season that top-player builds can be read from right now. */
export const onRequestGet: PagesFunction<Env> = async ({ request, env }) => {
  try {
    return await cached(request, "content", 24 * 3600, () => currentContent(env));
  } catch (e) {
    return e instanceof WclError ? failure(e.status, e instanceof WclError && e.status === 503 ? e.message : `WarcraftLogs: ${e.message}`) : failure(500, String(e));
  }
};
