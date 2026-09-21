import { useCallback, useEffect, useState } from "react";

/**
 * Copy the current view's link.
 *
 * The URL is already kept current, so this is only a convenience -- but a visible one,
 * because a user has no way to know the address bar is tracking what they painted unless
 * something says so.
 *
 * The clipboard API needs a secure context and a user gesture, and it still rejects in
 * enough situations (an iframe without permission, a browser that has never granted it) that
 * a silent failure would be a real outcome. So there is a fallback, and the button reports
 * which one happened rather than claiming success either way.
 */

type Result = "idle" | "copied" | "failed";

export function ShareButton({ label = "Copy link" }: { label?: string }) {
  const [result, setResult] = useState<Result>("idle");

  useEffect(() => {
    if (result === "idle") return;
    const timer = setTimeout(() => setResult("idle"), 2200);
    return () => clearTimeout(timer);
  }, [result]);

  const copy = useCallback(async () => {
    const url = window.location.href;
    try {
      await navigator.clipboard.writeText(url);
      setResult("copied");
      return;
    } catch {
      /* fall through to the selection fallback */
    }
    try {
      // Older path, and the one that still works without clipboard permission.
      const field = document.createElement("textarea");
      field.value = url;
      field.setAttribute("readonly", "");
      field.style.position = "fixed";
      field.style.opacity = "0";
      document.body.appendChild(field);
      field.select();
      const ok = document.execCommand("copy");
      document.body.removeChild(field);
      setResult(ok ? "copied" : "failed");
    } catch {
      setResult("failed");
    }
  }, []);

  return (
    <button
      type="button"
      className="btn w-full py-1 text-[12px]"
      onClick={() => void copy()}
      aria-live="polite"
    >
      {result === "copied"
        ? "Link copied"
        : result === "failed"
          ? "Copy failed — select the address bar"
          : label}
    </button>
  );
}
