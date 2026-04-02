import express from "express";
import bodyParser from "body-parser";
import { spawn } from "child_process";
import path from "path";
import { fileURLToPath } from "url";

// ===== Support for __dirname in ES Modules =====
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = 5000;

app.use(bodyParser.json());

// ===== Enable CORS for local frontend =====
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");
  next();
});

// ===== Serve static files from frontend =====
app.use(express.static(path.join(__dirname, "../frontend")));

// ===== /evaluate endpoint (calls bool_eval.out) =====
app.post("/evaluate", (req, res) => {
  const userInput = req.body.input || "";
  const exePath = path.join(__dirname, "../native/build/bool_eval.exe");

  const child = spawn(exePath);

  let output = "";
  let errorOutput = "";

  child.stdout.on("data", (data) => {
    output += data.toString();
  });

  child.stderr.on("data", (data) => {
    errorOutput += data.toString();
  });

  child.on("close", (code) => {
    if (code !== 0) {
      res.status(500).json({ error: errorOutput || "Execution failed" });
    } else {
      res.json({ output: output.trim() });
    }
  });

  // Send the expression to stdin of your C program
  child.stdin.write(userInput + "\n");
  child.stdin.end();
});

// ===== /diagram endpoint (returns SVG diagram) =====
app.post("/diagram", (req, res) => {
  const { input } = req.body;
  if (!input) {
    return res.status(400).json({ error: "No input provided" });
  }

  // Replace quotes for easier display
  const expr = input.replace(/\s+/g, "").replace(/'/g, "¬");

  // Basic SVG generator — each term becomes one AND gate input to an OR gate
  const terms = expr.split("+");
  let svg = `
  <svg xmlns="http://www.w3.org/2000/svg" width="600" height="${terms.length * 150}" style="background:#f8fafc;border-radius:10px">
    <style>
      .gate { fill:white; stroke:#000; stroke-width:2; }
      .wire { stroke:#000; stroke-width:2; }
      text { font-family:monospace; font-size:14px; }
    </style>
  `;

  terms.forEach((term, i) => {
    const y = 100 + i * 150;
    const variables = term.match(/[A-Z]/g) || [];
    const nots = term.match(/¬[A-Z]/g) || [];

    variables.forEach((v, j) => {
      const x = 50 + j * 60;
      svg += `<text x="${x}" y="${y - 30}">${v}</text>`;
      svg += `<line x1="${x}" y1="${y - 20}" x2="${x}" y2="${y}" class="wire"/>`;

      if (nots.some(n => n.includes(v))) {
        svg += `<circle cx="${x}" cy="${y}" r="6" fill="white" stroke="black"/>`;
        svg += `<text x="${x - 3}" y="${y + 5}" font-size="10">¬</text>`;
      }
    });

    svg += `<rect x="200" y="${y - 30}" width="60" height="40" class="gate"/>`;
    svg += `<text x="220" y="${y - 5}">&amp;</text>`;
    svg += `<line x1="260" y1="${y - 10}" x2="340" y2="${y - 10}" class="wire"/>`;
  });

  const orY = 100 + (terms.length - 1) * 75;
  svg += `<rect x="340" y="${orY - 30}" width="60" height="40" class="gate"/>`;
  svg += `<text x="360" y="${orY - 5}">≥1</text>`;
  svg += `<line x1="400" y1="${orY - 10}" x2="500" y2="${orY - 10}" class="wire"/>`;
  svg += `<text x="510" y="${orY - 5}">F</text>`;
  svg += `</svg>`;

  res.json({ minimized: expr, svg });
});

app.listen(PORT, () => {
  console.log(`✅ Server running at http://localhost:${PORT}`);
});