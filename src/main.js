import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";
import editorWorker from "monaco-editor/esm/vs/editor/editor.worker?worker";
import jsonWorker from "monaco-editor/esm/vs/language/json/json.worker?worker";
import cssWorker from "monaco-editor/esm/vs/language/css/css.worker?worker";
import htmlWorker from "monaco-editor/esm/vs/language/html/html.worker?worker";
import tsWorker from "monaco-editor/esm/vs/language/typescript/ts.worker?worker";
import "monaco-editor/min/vs/editor/editor.main.css";
import "./style.css";

globalThis.MonacoEnvironment = {
  getWorker(_moduleId, label) {
    if (label === "json") return new jsonWorker();
    if (label === "css" || label === "scss" || label === "less") return new cssWorker();
    if (label === "html" || label === "handlebars" || label === "razor") return new htmlWorker();
    if (label === "typescript" || label === "javascript") return new tsWorker();
    return new editorWorker();
  },
};

monaco.editor.defineTheme("dwcode-night", {
  base: "vs-dark",
  inherit: true,
  rules: [
    { token: "comment", foreground: "727D8D", fontStyle: "italic" },
    { token: "keyword", foreground: "FF4D9D" },
    { token: "string", foreground: "B8E986" },
    { token: "number", foreground: "FFB454" },
    { token: "type", foreground: "46D9E8" },
  ],
  colors: {
    "editor.background": "#0B0E14",
    "editor.foreground": "#D8DFE9",
    "editorLineNumber.foreground": "#46515F",
    "editorLineNumber.activeForeground": "#35D9E7",
    "editorCursor.foreground": "#35D9E7",
    "editor.selectionBackground": "#16404D88",
    "editor.lineHighlightBackground": "#111821",
    "editorIndentGuide.background1": "#202936",
    "editorIndentGuide.activeBackground1": "#35535D",
    "editorWidget.background": "#10151D",
    "editorSuggestWidget.background": "#10151D",
    "editorSuggestWidget.border": "#293542",
    "editorOverviewRuler.border": "#0B0E14",
  },
});

const initialCode = `import { createSignal } from "solid-js";

type RuntimeStatus = "ready" | "idle" | "offline";

const [status, setStatus] = createSignal<RuntimeStatus>("ready");

export function bootWorkspace(projectName: string) {
  const startedAt = performance.now();

  console.info("[DWCODE] booting " + projectName);
  setStatus("ready");

  return {
    status: status(),
    startupMs: Math.round(performance.now() - startedAt),
  };
}
`;

const samples = {
  "main.ts": { language: "typescript", code: initialCode },
  "workspace.ts": {
    language: "typescript",
    code: `export type Workspace = {\n  name: string;\n  root: string;\n  files: string[];\n};\n\nexport function summarize(workspace: Workspace) {\n  return {\n    ...workspace,\n    fileCount: workspace.files.length,\n    ready: true,\n  };\n}\n`,
  },
  "theme.css": {
    language: "css",
    code: `:root {\n  --dw-bg: #090c12;\n  --dw-panel: #10141c;\n  --dw-cyan: #35d9e7;\n  --dw-magenta: #fb4c9a;\n  --dw-text: #d8dfe9;\n}\n\n.workspace {\n  color: var(--dw-text);\n  background: var(--dw-bg);\n}\n`,
  },
  "README.md": {
    language: "markdown",
    code: `# DWCode\n\nA keyboard-first Cybercore editor.\n\n- Native Qt Quick desktop shell\n- Monaco editing surface\n- Optional Helix-inspired modal mode\n- Local-first workspace\n`,
  },
};

const editor = monaco.editor.create(document.querySelector("#editor"), {
  value: initialCode,
  language: "typescript",
  theme: "dwcode-night",
  automaticLayout: true,
  fontFamily: '"JetBrains Mono", "Cascadia Code", monospace',
  fontSize: 14,
  lineHeight: 23,
  fontLigatures: true,
  minimap: { enabled: true, scale: 1 },
  scrollBeyondLastLine: false,
  smoothScrolling: true,
  cursorBlinking: "smooth",
  padding: { top: 18, bottom: 24 },
  tabSize: 2,
  wordWrap: "off",
  bracketPairColorization: { enabled: true },
  guides: { bracketPairs: true, indentation: true },
  suggest: { preview: true },
});

window.addEventListener("resize", () => editor.layout());
window.dwcodeEditor = editor;
window.dwcodeOpenSample = (name) => {
  const sample = samples[name] ?? samples["main.ts"];
  editor.setValue(sample.code);
  monaco.editor.setModelLanguage(editor.getModel(), sample.language);
};
window.dwcodeSetMode = (normalMode) => {
  document.body.dataset.mode = normalMode ? "normal" : "insert";
};

editor.onKeyDown((event) => {
  if (event.browserEvent.key === "Escape" && document.body.dataset.mode !== "normal") {
    window.dwcodeSetMode(true);
    event.preventDefault();
    return;
  }
  if (document.body.dataset.mode !== "normal" || event.ctrlKey || event.metaKey || event.altKey)
    return;

  const key = event.browserEvent.key;
  if (key === "i") {
    window.dwcodeSetMode(false);
    event.preventDefault();
    return;
  }
  const directions = { h: "left", j: "down", k: "up", l: "right" };
  if (directions[key]) {
    editor.trigger("dwcode-modal", "cursorMove", { direction: directions[key] });
    event.preventDefault();
    return;
  }
  if (key === "x") {
    editor.trigger("dwcode-modal", "deleteRight", null);
    event.preventDefault();
  }
});
