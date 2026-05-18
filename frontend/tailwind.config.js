/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      // Menambahkan definisi z-index agar bisa pakai z-100, z-101, dll
      zIndex: {
        100: "100",
        101: "101",
        200: "200",
      },
      // Menambahkan definisi min-width
      minWidth: {
        30: "120px",
      },
      // Menambahkan definisi max-width
      maxWidth: {
        300: "1200px",
      },
    },
  },
  plugins: [],
};
