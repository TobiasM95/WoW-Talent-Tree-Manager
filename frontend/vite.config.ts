import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    host: true,
    port: 5173,
    // /api is the site's Cloudflare Pages Functions. In development they run under
    // `wrangler pages dev` (port 8788 by default); proxying keeps every fetch same-origin.
    proxy: {
      "/api": { target: process.env.TTM_FUNCTIONS_URL ?? "http://localhost:8788", changeOrigin: true },
    },
  },
});
