import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";
// editor.api.js is the bare core; this adds hover, find, folding, comments, rename, etc.
import "monaco-editor/esm/vs/editor/editor.all.js";
import editorWorker from "monaco-editor/esm/vs/editor/editor.worker?worker";
import jsonWorker from "monaco-editor/esm/vs/language/json/json.worker?worker";
import cssWorker from "monaco-editor/esm/vs/language/css/css.worker?worker";
import htmlWorker from "monaco-editor/esm/vs/language/html/html.worker?worker";
import tsWorker from "monaco-editor/esm/vs/language/typescript/ts.worker?worker";
import "monaco-editor/min/vs/editor/editor.main.css";
import "./style.css";
import "monaco-editor/esm/vs/basic-languages/monaco.contribution.js";
import cybercoreThemes from "./data/themes.json";

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
let dwcodeSettings = {};
const numberList = value => String(value || "").split(",").map(x => Number(x.trim())).filter(x => Number.isInteger(x) && x > 0 && x < 1000);
window.dwcodeApplySettings = preferences => {
  dwcodeSettings = preferences || {};
  const option = (key, fallback) => dwcodeSettings[key] === undefined ? fallback : dwcodeSettings[key];
  editor.updateOptions({
    fontFamily: option("editor/fontFamily", '"JetBrains Mono", "Cascadia Code", monospace'),
    fontSize: Number(option("editor/fontSize", 14)), lineHeight: Number(option("editor/lineHeight", 23)),
    fontLigatures: !!option("editor/fontLigatures", true), wordWrap: option("editor/wordWrap", "off"),
    minimap: { enabled: !!option("editor/minimap", true), scale: 1 }, lineNumbers: option("editor/lineNumbers", "on"),
    rulers: numberList(option("editor/rulers", "")), renderWhitespace: option("editor/renderWhitespace", "none"),
    bracketPairColorization: { enabled: !!option("editor/bracketGuides", true) },
    guides: { bracketPairs: !!option("editor/bracketGuides", true), indentation: !!option("editor/bracketGuides", true) },
    renderLineHighlight: option("editor/currentLineHighlight", true) ? "all" : "none",
    smoothScrolling: !!option("editor/smoothScrolling", true),
    autoIndent: option("editing/autoIndent", true) ? "advanced" : "none",
    autoClosingBrackets: option("editing/autoClosingBrackets", true) ? "always" : "never"
  });
  for (const model of models.values()) model.updateOptions({tabSize:Number(option("editing/tabSize",2)),insertSpaces:!!option("editing/insertSpaces",true)});
  if (document.body.dataset.mode === "normal") editor.updateOptions({cursorStyle: option("keyboard/modalCursor", "block")});
};
window.dwcodeOpenSample = (name) => {
  const sample = samples[name] ?? samples["main.ts"];
  editor.setValue(sample.code);
  monaco.editor.setModelLanguage(editor.getModel(), sample.language);
};
window.dwcodeSetMode = (normalMode) => {
  document.body.dataset.mode = normalMode ? "normal" : "insert";
  editor.updateOptions({cursorStyle: normalMode ? (dwcodeSettings["keyboard/modalCursor"] || "block") : "line"});
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

// Native capabilities exist only in the Qt WebChannel, never on the public HTTP server.
let native = null, applyingNative = false;
const models = new Map(), pendingLanguage = new Map(), inFlight = new Map();
let selectedTheme = cybercoreThemes.find(t => t.id === 'cyberpunk-chrome') || cybercoreThemes[0];
window.dwcodeThemes = cybercoreThemes;
window.dwcodeSetTheme = id => {
  const t = window.dwcodeThemes.find(t => t.id === id);
  if (!t) return;
  selectedTheme = t;
  // Monaco rejects theme names outside [a-z0-9-], e.g. the dotted DWCode ids.
  const name = 'dw-' + t.id.replace(/[^a-z0-9-]/gi, '-');
  monaco.editor.defineTheme(name, {
    base: t.appearance === 'light' ? 'vs' : 'vs-dark', inherit: true,
    rules: Object.entries(t.syntax).map(([token,color]) => ({token, foreground:color.replace('#','')})),
    colors: {'editor.background':t.surfaces.editor,'editor.foreground':t.text.primary,'editorCursor.foreground':t.accents.primary,
      'editorLineNumber.foreground':t.text.muted,'editorLineNumber.activeForeground':t.accents.primary,
      'editor.selectionBackground':t.accents.primary+'35','editor.lineHighlightBackground':t.surfaces.shell,
      'editorWidget.background':t.surfaces.raised,'editorSuggestWidget.background':t.surfaces.raised,
      'editorSuggestWidget.border':t.surfaces.border,'editorHoverWidget.background':t.surfaces.raised,
      'editorHoverWidget.border':t.surfaces.border,'editorIndentGuide.background1':t.surfaces.border}
  });
  monaco.editor.setTheme(name);
  document.body.style.background = t.surfaces.editor;
  try {localStorage.setItem('dwcode-theme', id)} catch {}
  window.dispatchEvent(new CustomEvent('dwcode-theme', {detail:t}));
};
// Pack snippets become real completions: declarative data only, no pack code runs.
const snippetProviders = [];
window.dwcodeApplySnippets = map => {
  for (const provider of snippetProviders.splice(0)) { try { provider.dispose() } catch {} }
  for (const [language, entries] of Object.entries(map || {})) {
    if (!Array.isArray(entries) || !entries.length) continue;
    snippetProviders.push(monaco.languages.registerCompletionItemProvider(language, {
      provideCompletionItems(model, position) {
        const word = model.getWordUntilPosition(position);
        const range = {startLineNumber: position.lineNumber, startColumn: word.startColumn, endLineNumber: position.lineNumber, endColumn: word.endColumn};
        return {suggestions: entries.map(snippet => ({
          label: snippet.prefix,
          kind: monaco.languages.CompletionItemKind.Snippet,
          insertText: snippet.body,
          insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          documentation: snippet.description || snippet.name || "",
          detail: "Pack snippet",
          range,
        }))};
      },
    }));
  }
};
try {window.dwcodeSetTheme(localStorage.getItem('dwcode-theme') || selectedTheme.id)} catch {window.dwcodeSetTheme(selectedTheme.id)}
const applyDocument = (doc, activate = true) => {
  if (!doc?.uri) return;
  applyingNative = true;
  let model = models.get(doc.uri);
  if (!model) {
    model = monaco.editor.createModel(doc.text, doc.language.replace(/react$/, ''), monaco.Uri.parse(doc.uri));
    models.set(doc.uri, model);
    model.onDidChangeContent(() => {
      if (applyingNative || !native) return;
      inFlight.set(doc.uri, (inFlight.get(doc.uri) || 0) + 1);
      native.updateDocument(doc.path, model.getValue(), ok => {
        inFlight.set(doc.uri, inFlight.get(doc.uri) - 1);
        if (!ok || dwcodeSettings["files/autoSave"] !== "after delay") return;
        clearTimeout(model.dwcodeAutoSaveTimer);
        model.dwcodeAutoSaveTimer = setTimeout(() => {
          if (native)
            native.updateDocument(doc.path, model.getValue(), saved => { if (saved) native.saveDocument(doc.path); });
        }, Number(dwcodeSettings["files/autoSaveDelay"] || 1000));
      });
    });
    model.updateOptions({tabSize:Number(dwcodeSettings["editing/tabSize"] || 2),insertSpaces:dwcodeSettings["editing/insertSpaces"] !== false});
  } else if (model.getValue() !== doc.text && !inFlight.get(doc.uri)) {
    // Skip while this view has edits in flight: the backend is echoing an older
    // text, and applying it would rewind the cursor and cancel completion.
    const state = editor.saveViewState();
    model.setValue(doc.text);
    if (editor.getModel() === model && state) editor.restoreViewState(state);
  }
  if (activate) editor.setModel(model);
  applyingNative = false;
};
window.dwcodeSave = () => {
  if (!native || !editor.getModel()?.uri || editor.getModel().uri.scheme !== 'file') return false;
  const path = editor.getModel().uri.fsPath;
  native.updateDocument(path, editor.getValue(), ok => {if (ok) native.saveDocument(path)});
  return true;
};
editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, () => window.dwcodeSave());
const range = r => r ? ({startLineNumber:r.start.line+1,startColumn:r.start.character+1,endLineNumber:r.end.line+1,endColumn:r.end.character+1}) : undefined;
function requestLanguage(method, model, position) {
  if (!native?.lspReady || model.uri.scheme !== 'file') return Promise.resolve(null);
  const params = {textDocument:{uri:model.uri.toString()},position:{line:position.lineNumber-1,character:position.column-1}};
  return new Promise(resolve => {
    native.languageRequest(method, params, id => {
      if (id < 0) {resolve(null); return}
      const timeout = setTimeout(() => {pendingLanguage.delete(id);resolve(null)}, 11000);
      pendingLanguage.set(id, result => {clearTimeout(timeout);resolve(result)});
    });
  });
}
const completionKinds = [monaco.languages.CompletionItemKind.Text,monaco.languages.CompletionItemKind.Method,monaco.languages.CompletionItemKind.Function,monaco.languages.CompletionItemKind.Constructor,monaco.languages.CompletionItemKind.Field,monaco.languages.CompletionItemKind.Variable,monaco.languages.CompletionItemKind.Class,monaco.languages.CompletionItemKind.Interface,monaco.languages.CompletionItemKind.Module,monaco.languages.CompletionItemKind.Property,monaco.languages.CompletionItemKind.Unit,monaco.languages.CompletionItemKind.Value,monaco.languages.CompletionItemKind.Enum,monaco.languages.CompletionItemKind.Keyword,monaco.languages.CompletionItemKind.Snippet,monaco.languages.CompletionItemKind.Color,monaco.languages.CompletionItemKind.File,monaco.languages.CompletionItemKind.Reference,monaco.languages.CompletionItemKind.Folder,monaco.languages.CompletionItemKind.EnumMember,monaco.languages.CompletionItemKind.Constant,monaco.languages.CompletionItemKind.Struct,monaco.languages.CompletionItemKind.Event,monaco.languages.CompletionItemKind.Operator,monaco.languages.CompletionItemKind.TypeParameter];
for (const language of ['typescript','javascript','cpp','c','rust','python']) {
  monaco.languages.registerCompletionItemProvider(language, {
    triggerCharacters:['.','>'],
    async provideCompletionItems(model, position) {
      const response = await requestLanguage('textDocument/completion', model, position);
      const items = Array.isArray(response) ? response : response?.items || [];
      const word = model.getWordUntilPosition(position);
      return {suggestions:items.map(item => ({label:item.label,kind:completionKinds[(item.kind||1)-1] ?? monaco.languages.CompletionItemKind.Text,
        detail:item.detail,documentation:typeof item.documentation === 'string' ? item.documentation : item.documentation?.value,
        insertText:item.textEdit?.newText ?? item.insertText ?? item.label,sortText:item.sortText,filterText:item.filterText,
        range:range(item.textEdit?.range || item.textEdit?.replace) || {startLineNumber:position.lineNumber,endLineNumber:position.lineNumber,startColumn:word.startColumn,endColumn:word.endColumn}}))};
    }
  });
  monaco.languages.registerHoverProvider(language, {async provideHover(model, position) {
    const result = await requestLanguage('textDocument/hover',model,position);
    if (!result?.contents) return null;
    const values = Array.isArray(result.contents) ? result.contents : [result.contents];
    return {range:range(result.range),contents:values.map(v => ({value:typeof v === 'string'?v:v.value,isTrusted:false,supportHtml:false}))};
  }});
  monaco.languages.registerDefinitionProvider(language, {async provideDefinition(model,position) {
    const result = await requestLanguage('textDocument/definition',model,position);
    return (Array.isArray(result)?result:result?[result]:[]).filter(x => (x.uri||x.targetUri)?.startsWith(monaco.Uri.file(native.root+'/').toString())).map(x => ({uri:monaco.Uri.parse(x.uri||x.targetUri),range:range(x.range||x.targetSelectionRange)}));
  }});
}
monaco.editor.registerEditorOpener({openCodeEditor(_source, uri, selection) {
  if (!native || uri.scheme !== 'file') return false;
  native.openFile(uri.fsPath, doc => {applyDocument(doc);if (selection) {editor.setSelection(selection);editor.revealRangeInCenter(selection)}});
  return true;
}});
function connectNative() {
  if (!globalThis.qt?.webChannelTransport) return;
  if (!globalThis.QWebChannel) {
    const script = document.createElement('script');
    script.src = new URL('qwebchannel.js', location.href).href;
    script.onload = connectNative;
    document.head.append(script);
    return;
  }
  new QWebChannel(qt.webChannelTransport, channel => {
    native = channel.objects.native;
    let workspaceRoot = native.root;
    native.documentActivated.connect(doc => applyDocument(doc));
    native.documentUpdated.connect(doc => applyDocument(doc,false));
    native.workspaceChanged.connect(() => {
      if (workspaceRoot !== native.root) {workspaceRoot=native.root;editor.setModel(null);for (const m of models.values())m.dispose();models.clear()}
    });
    // Enabling or disabling a pack can both change the catalog and select a
    // different theme, so the theme list is re-read before each apply.
    native.themeChanged.connect(() => { window.dwcodeThemes = native.themes; window.dwcodeSetTheme(native.theme.id); });
    native.packsChanged.connect(() => { window.dwcodeThemes = native.themes; window.dwcodeApplySnippets(native.packSnippets); });
    native.preferencesChanged.connect(preferences => window.dwcodeApplySettings(preferences));
    native.languageReply.connect((id,result,error) => {pendingLanguage.get(id)?.(error?null:result);pendingLanguage.delete(id)});
    native.diagnosticsReceived.connect((uri,diagnostics) => {
      const model = models.get(uri);if (!model) return;
      monaco.editor.setModelMarkers(model,'dwcode-lsp', diagnostics.map(d => ({...range(d.range),message:d.message,severity:({1:8,2:4,3:2,4:1})[d.severity]||2,source:d.source,code:d.code?.toString()})));
    });
    native.lspChanged.connect(() => {if(!native.lspReady) for(const m of models.values())monaco.editor.setModelMarkers(m,'dwcode-lsp',[])});
    window.dwcodeThemes = native.themes;
    window.dwcodeApplySnippets(native.packSnippets);
    window.dwcodeSetTheme(native.theme.id);
    window.dwcodeApplySettings(native.preferences);
    native.activeDocument(doc => applyDocument(doc));
    window.dwcodeNativeReady = true;
  });
}
connectNative();
