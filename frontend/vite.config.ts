import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    host: true,
    port: 5173,
    // The API is a separate container. Proxying in dev keeps every fetch same-origin, so
    // there is no CORS configuration that exists only for development.
    proxy: {
      "/api": {
        target: process.env.TTM_API_URL ?? "http://localhost:8001",
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api/, ""),
      },
    },
  },
});
