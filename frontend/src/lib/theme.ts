import { useCallback, useEffect, useState } from "react";

export type ThemeChoice = "system" | "light" | "dark";

const KEY = "ttm-theme";

/**
 * Theme selection, stamped onto <html data-theme>.
 *
 * Three states rather than two. "system" is the default and means *no* stamp, which is what
 * lets the `prefers-color-scheme` layer in tokens.css apply -- a visitor who has never
 * touched the toggle gets whatever their OS asked for. Only an explicit choice writes an
 * attribute, and only then does it override the system.
 *
 * Storage is wrapped because it throws in a private window with site data blocked, and a
 * remembered theme is a convenience: losing it must not take the page with it.
 */
export function useTheme() {
  const [choice, setChoice] = useState<ThemeChoice>(() => {
    try {
      const stored = localStorage.getItem(KEY);
      if (stored === "light" || stored === "dark") return stored;
    } catch {
      /* private window, blocked storage -- fall through to the system preference */
    }
    return "system";
  });

  useEffect(() => {
    const root = document.documentElement;
    if (choice === "system") root.removeAttribute("data-theme");
    else root.setAttribute("data-theme", choice);
    try {
      if (choice === "system") localStorage.removeItem(KEY);
      else localStorage.setItem(KEY, choice);
    } catch {
      /* not worth surfacing: the theme still applies for this visit */
    }
  }, [choice]);

  // What is actually on screen, which is what a toggle label has to reflect.
  const resolved = useCallback((): "light" | "dark" => {
    if (choice !== "system") return choice;
    return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  }, [choice]);

  const toggle = useCallback(() => {
    setChoice(resolved() === "dark" ? "light" : "dark");
  }, [resolved]);

  return { choice, setChoice, toggle, resolved };
}
