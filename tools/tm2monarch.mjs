#!/usr/bin/env node
// DWCode grammar bridge: convert a TextMate/VS Code syntax grammar
// (`.tmLanguage.json`: `{ name, scopeName, patterns, repository, ... }`) into a
// Monaco Monarch language definition, per PACK_FORMAT.md's declarative-pack
// scope and the EXTENSIONS.md stage-2 rule that syntax definitions must be
// consumable without executing extension code.
//
//   node tools/tm2monarch.mjs <grammar.tmLanguage.json> [-o out.json] [--id <languageId>]
//   node tools/tm2monarch.mjs - [--id <languageId>] < grammar.tmLanguage.json   # read stdin
//   node tools/tm2monarch.mjs --self-test                                       # built-in suite
//
// Rules of engagement:
//   * The Monarch JSON goes to stdout (or to `-o`); every diagnostic —
//     unsupported constructs, dropped fields, unmapped scopes — goes to stderr,
//     so the JSON stays clean and pipeable.
//   * Conversion is best-effort and never throws: a malformed grammar yields
//     warnings plus whatever converted cleanly. Exit code 1 means unreadable
//     input, unwritable output, bad usage, or a failed `--self-test`; warnings
//     alone never change it.
//   * Output is deterministic (fixed key order, source-order rules, stable
//     state names) so repeated conversions diff cleanly.
//
// Three parts of this converter are genuinely tricky. Each is explained in a
// long comment where it happens rather than here:
//   1. state naming            — the "State naming" block above createNamer()
//   2. captures → token arrays — the "Captures → token arrays" block above tilePattern()
//   3. repository inlining     — the "Include inlining" block above expandInclude()
//
// Node >= 18, zero dependencies. `--self-test` additionally feeds every emitted
// definition through Monaco's own monarch compiler when `monaco-editor` is
// installed (monarchCompile.js is plain ESM with no browser imports, so it
// loads in Node); without node_modules the structural checks still run.

import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const DEFAULT_TOKEN = "source";
const MAX_INCLUDE_DEPTH = 32;

// TextMate scope → Monarch token. The first matching entry wins, so the table
// runs specific → general. `scopeMatches` treats a pattern as a prefix match on
// dot boundaries (pattern `keyword` matches scope `keyword.control.if`) and a
// trailing `*` swallows the rest (`entity.name.*` also matches `entity.name`
// itself), which is why `variable.parameter` must sit above `variable` and
// `keyword.operator` above `keyword.*`. Scopes that fall off the end of this
// table are reported on stderr and fall back to DEFAULT_TOKEN, so grammar
// authors can see exactly which scopes still need a mapping.
const SCOPE_TOKENS = [
  ["variable.parameter", "variable.parameter"],
  ["keyword.operator", "operator"],
  ["entity.name.function", "identifier.predeclared"],
  ["support.function", "identifier.predeclared"],
  ["meta.function-call", "identifier.predeclared"],
  ["entity.name.type", "type"],
  ["support.type", "type"],
  ["support.class", "type"],
  ["entity.name.tag", "keyword"],
  ["entity.name.*", "type"],
  ["storage.*", "keyword"],
  ["keyword.*", "keyword"],
  ["string.*", "string"],
  ["punctuation.definition.string", "string"],
  ["comment.*", "comment"],
  ["constant.numeric", "number"],
  ["constant.language", "keyword"],
  ["meta.separator", "delimiter"],
  ["invalid", "invalid"],
  ["variable", "variable"],
  ["source", "source"],
];

// Grammar-level fields we knowingly drop, with the reason reported on stderr
// as `ignored <value>`.
const IGNORED_GRAMMAR_FIELDS = {
  fileTypes: "\"fileTypes\": file association belongs to the language registration, not the Monarch tokenizer",
  firstLineMatch: "\"firstLineMatch\": Monarch has no first-line (shebang) detection",
  foldingStartMarker: "\"foldingStartMarker\": Monarch does no folding",
  foldingStopMarker: "\"foldingStopMarker\": Monarch does no folding",
  applyEndPatternLast: "\"applyEndPatternLast\": the end pattern is always tried before inner rules (TextMate's default)",
};

// Pure metadata: neither used nor worth a warning on every real grammar.
const SILENT_GRAMMAR_FIELDS = new Set(["uuid", "version"]);

const KNOWN_GRAMMAR_FIELDS = new Set([
  "name", "scopeName", "patterns", "repository",
  ...Object.keys(IGNORED_GRAMMAR_FIELDS),
  ...SILENT_GRAMMAR_FIELDS,
]);

const KNOWN_RULE_FIELDS = new Set([
  "include", "match", "captures", "begin", "end", "beginCaptures",
  "endCaptures", "patterns", "name", "contentName", "while",
  "applyEndPatternLast",
]);

// Oniguruma constructs JavaScript RegExp accepts but reads differently. These
// are emitted (they compile) and warned about; the fatal ones in
// detectPatternIssues() are dropped instead.
const SEMANTIC_BACKSLASH = [
  ["G", "pattern uses \"\\G\" (end of previous match), which JavaScript does not implement"],
  ["K", "pattern uses \"\\K\" (keep-out), which JavaScript does not implement"],
  ["h", "pattern uses \"\\h\" (horizontal whitespace), which JavaScript reads as a literal \"h\""],
  ["H", "pattern uses \"\\H\", which JavaScript does not implement"],
  ["R", "pattern uses \"\\R\" (generic newline), which JavaScript does not implement"],
  ["v", "pattern uses \"\\v\", which JavaScript reads as a vertical tab rather than horizontal whitespace"],
  ["X", "pattern uses \"\\X\" (grapheme cluster), which JavaScript does not implement"],
  ["Q", "pattern uses \"\\Q…\\E\" literal quoting, which JavaScript does not implement"],
];

const USAGE = [
  "usage:",
  "  node tools/tm2monarch.mjs <grammar.tmLanguage.json> [-o out.json] [--id <languageId>]",
  "  node tools/tm2monarch.mjs - [--id <languageId>] < grammar.tmLanguage.json",
  "  node tools/tm2monarch.mjs --self-test",
  "",
  "The Monarch definition is written to stdout unless -o names a file;",
  "diagnostics always go to stderr. Exit code 1 = unreadable input,",
  "unwritable output, bad usage, or a failed --self-test.",
].join("\n");

// ------------------------------------------------------------------ scopes

function scopeMatches(pattern, scope) {
  const patternParts = pattern.split(".");
  const scopeParts = scope.split(".");
  for (let i = 0; i < patternParts.length; i++) {
    if (patternParts[i] === "*") return true; // `*` matches the rest, including nothing
    if (i >= scopeParts.length) return false; // pattern longer than scope
    if (patternParts[i] !== scopeParts[i]) return false;
  }
  return true; // pattern exhausted: prefix match on a dot boundary
}

// Maps one scope name, warning once per distinct unmapped scope. Returns null
// when `scope` is not a scope at all (missing/empty), so callers can walk a
// candidate chain before giving up on DEFAULT_TOKEN.
function mapScopeToken(scope, warnAt, where) {
  if (typeof scope !== "string" || scope === "") return null;
  for (const [pattern, token] of SCOPE_TOKENS) {
    if (scopeMatches(pattern, scope)) return token;
  }
  warnAt(where, `scope "${scope}" has no token mapping; falling back to "${DEFAULT_TOKEN}"`);
  return DEFAULT_TOKEN;
}

// First candidate scope that maps to a token wins: capture-level scope, then
// whole-match (capture 0) scope, then the rule's own `name`.
function pickToken(candidates, warnAt, where) {
  for (const candidate of candidates) {
    const token = mapScopeToken(candidate, warnAt, where);
    if (token !== null) return token;
  }
  return DEFAULT_TOKEN;
}

// ------------------------------------------------------------------ regexes

// Converts one Oniguruma pattern to JavaScript, or returns null to drop the
// rule. The anchor rewrite is a single escape-aware pass so `\\A` (escaped
// backslash followed by A) survives untouched while `\A` becomes `^`.
//
// Why this is exact rather than approximate: Monarch compiles every rule as
// `^(?:pattern)`, strips a leading `^` into a "try only at line start" rule,
// and runs the regex against the rest of the current line. Monaco tokenizes
// line by line, so `\A` (absolute start) → `^` (line start) and `\z`/`\Z`
// (absolute end) → `$` (line end) preserve the TextMate meaning.
function convertPattern(raw, warnAt, where) {
  if (typeof raw !== "string") {
    warnAt(where, "pattern is not a string; rule dropped");
    return null;
  }
  let src = "";
  for (let i = 0; i < raw.length; i++) {
    const ch = raw[i];
    if (ch === "\\" && i + 1 < raw.length) {
      const next = raw[i + 1];
      if (next === "A") src += "^";
      else if (next === "z" || next === "Z") src += "$";
      else src += ch + next;
      i += 1;
      continue;
    }
    src += ch;
  }

  let fatal = false;
  for (const issue of detectPatternIssues(src)) {
    warnAt(where, issue.message);
    fatal = fatal || issue.fatal;
  }
  if (fatal) return null; // the issue messages already explain the drop

  // Monarch expands `@word` inside regex strings as language-attribute
  // references and throws when the attribute does not exist ("language
  // definition does not contain attribute 'media'"), so every literal `@`
  // must be doubled: monarch rewrites `@@` back to `@` before compiling.
  src = src.replace(/@/g, "@@");
  try {
    new RegExp(src);
  } catch (error) {
    warnAt(where, `rule dropped: JavaScript cannot compile this pattern (${error.message})`);
    return null;
  }
  return src;
}

// Constructs JavaScript cannot compile (fatal: rule dropped) or compiles with
// a different meaning (kept, but reported).
function detectPatternIssues(src) {
  const issues = [];
  const masked = maskRegExpNoise(src);
  const add = (fatal, message) => issues.push({ fatal, message });

  if (masked.includes("(?~")) {
    add(true, "pattern uses the Oniguruma absent-text construct \"(?~…)\" which JavaScript cannot compile; rule dropped");
  }
  if (/[*+?}]\+/.test(masked)) {
    add(true, "pattern uses a possessive quantifier (\"*+\", \"++\", \"?+\", \"}+\") which JavaScript cannot compile; rule dropped");
  }
  if (masked.includes("(?>")) {
    add(true, "pattern uses an atomic group \"(?>…)\" which JavaScript cannot compile; rule dropped");
  }
  if (masked.includes("(?|")) {
    add(true, "pattern uses a branch-reset group \"(?|…)\" which JavaScript cannot compile; rule dropped");
  }
  if (/\(\?[a-z]+[-:)]/.test(masked)) {
    add(true, "pattern uses an inline flag group \"(?i:…)\"; Node 18 (DWCode's floor) cannot compile it, so the rule is dropped");
  }
  if (/\(\?P/.test(masked)) {
    add(true, "pattern uses Python-style \"(?P…)\" group syntax which JavaScript cannot compile; rule dropped");
  }
  if (/\(\?(?:R\b|0\b|&|\d|')/.test(masked)) {
    add(true, "pattern uses an Oniguruma subroutine call (\"(?R)\", \"(?&name)\", \"(?1)\") which JavaScript cannot compile; rule dropped");
  }
  if (/\$[sS]\d/.test(src)) {
    add(true, "pattern contains \"$s1\"-style text that Monarch rewrites as state substitutions; rule dropped");
  }

  // Escape-aware scan for backslash constructs: `\G` must not fire on `\\G`.
  const backslashHits = new Set();
  for (let i = 0; i < src.length; i++) {
    if (src[i] !== "\\") continue;
    const next = src[i + 1];
    if (next === undefined) break;
    if ("GKhHRvXQ".includes(next)) backslashHits.add(next);
    i += 1;
  }
  for (const [letter, message] of SEMANTIC_BACKSLASH) {
    if (backslashHits.has(letter)) add(false, message);
  }
  if (/\[\[:[a-z]+:\]\]/.test(src)) {
    add(false, "pattern uses a POSIX class like \"[[:alpha:]]\" which JavaScript does not implement");
  }
  return issues;
}

// Blanks out escapes and character classes (same length) so quantifier checks
// cannot be fooled by `\+` or the literal `+` inside `[+*]`.
function maskRegExpNoise(src) {
  let out = "";
  for (let i = 0; i < src.length; i++) {
    const ch = src[i];
    if (ch === "\\") {
      out += "  ";
      i += 1;
      continue;
    }
    if (ch === "[") {
      let j = i + 1;
      if (src[j] === "^") j += 1;
      if (src[j] === "]") j += 1;
      while (j < src.length && src[j] !== "]") {
        if (src[j] === "\\") j += 1;
        j += 1;
      }
      if (j < src.length) j += 1; // include the closing bracket
      out += " ".repeat(j - i);
      i = j - 1;
      continue;
    }
    out += ch;
  }
  return out;
}

// ---------------------------------------------------------------------------
// Captures → token arrays (tricky part 2 of 3).
//
// Monarch expresses "tokenize capture group 1 as X, group 2 as Y" with a group
// action: an array used as the rule's action, whose i-th entry tokens capture
// group i+1. The runtime (monarchLexer) is strict about it:
//   • the array length must be EXACTLY the pattern's capture-group count;
//   • every matched character must belong to some group — monarch throws
//     "all characters should be matched in consecutive groups" otherwise
//     (zero-width text outside groups is fine);
//   • groups may be neither optional (unmatched groups arrive as null and
//     crash the length bookkeeping) nor nested (they double-count the match).
// TextMate captures rarely tile that nicely — `(\d+)\.(\d+)` leaves the dot
// uncovered — so this section analyzes the pattern and, when it is tileable,
// rewrites it by wrapping each positive-width gap in a synthesized capture
// group, remembering how the original group numbers moved. When it is not
// tileable the caller falls back to one whole-match token and warns.
// ---------------------------------------------------------------------------

function skipCharClass(src, i) {
  let j = i + 1;
  if (src[j] === "^") j += 1;
  if (src[j] === "]") j += 1;
  while (j < src.length && src[j] !== "]") {
    if (src[j] === "\\") j += 1;
    j += 1;
  }
  return j; // index of ']' (or end of input)
}

function classifyGroup(src, i) {
  if (src[i + 1] !== "?") return { capturing: true };
  if (src[i + 2] === ":" || src[i + 2] === "=" || src[i + 2] === "!") return { capturing: false };
  if (src.startsWith("(?<=", i) || src.startsWith("(?<!", i)) return { capturing: false };
  if (src.startsWith("(?<", i)) return { capturing: true }; // (?<name>…)
  if (src.startsWith("(?P", i)) return { capturing: false, problem: "Python-style \"(?P…\" group" };
  return { capturing: false, problem: "unrecognized \"(?…\" group construct" };
}

// True when a pattern span can only match the empty string: anchors and word
// boundaries, which need no wrapping to satisfy the coverage rule.
function isZeroWidthSpan(text) {
  for (let i = 0; i < text.length; i++) {
    const ch = text[i];
    if (ch === "\\") {
      const next = text[i + 1];
      if (next === "b" || next === "B" || next === "A" || next === "z" || next === "Z") {
        i += 1;
        continue;
      }
      return false;
    }
    if (ch === "^" || ch === "$") continue;
    return false;
  }
  return true;
}

// Analyzes a (already JavaScript-valid) pattern and, when possible, rewrites
// it into a perfect tiling of capture groups. Returns:
//   ok           — whether `regex` may be used for a group action;
//   regex        — the rewritten pattern (gap groups wrapped in);
//   count        — capture groups in the rewritten pattern (token array length);
//   originalCount— capture groups in the source pattern (TextMate capture
//                  numbers refer to these);
//   numberMap    — original group number → rewritten group number;
//   problems     — why tiling is impossible, for the warning.
function tilePattern(src) {
  const problems = [];
  const openGroups = [];
  const groups = [];
  for (let i = 0; i < src.length; i++) {
    const ch = src[i];
    if (ch === "\\") {
      const next = src[i + 1];
      if (next !== undefined && next >= "1" && next <= "9") {
        problems.push(`backreference "\\${next}" (wrapping gaps would shift group numbers)`);
      }
      i += 1;
      continue;
    }
    if (ch === "[") {
      i = skipCharClass(src, i);
      continue;
    }
    if (ch === "|" && openGroups.length === 0) {
      problems.push("top-level alternation \"|\" (a branch can leave characters uncovered)");
      continue;
    }
    if (ch === "(") {
      const classified = classifyGroup(src, i);
      if (classified.problem) problems.push(classified.problem);
      openGroups.push({ open: i, capturing: classified.capturing });
      continue;
    }
    if (ch === ")") {
      const opened = openGroups.pop();
      if (!opened) {
        problems.push("unbalanced \")\"");
        continue;
      }
      if (opened.capturing) {
        const after = src[i + 1];
        const quantified = after === "*" || after === "+" || after === "?" || after === "{";
        groups.push({ open: opened.open, close: i, depth: openGroups.length, quantified });
      }
      continue;
    }
  }
  if (openGroups.length) problems.push("unbalanced \"(\"");
  if (!groups.length) problems.push("pattern has no capture groups");
  for (const group of groups) {
    if (group.depth > 0) problems.push("nested capture group");
    if (group.quantified) problems.push("quantified or optional capture group");
  }
  groups.sort((a, b) => a.open - b.open);
  groups.forEach((group, index) => { group.number = index + 1; });

  const result = {
    ok: false,
    regex: src,
    count: 0,
    originalCount: groups.length,
    numberMap: new Map(),
    problems,
  };
  if (problems.length) return result;

  // Segment the pattern into top-level capture groups and the gaps between
  // them (a gap may still contain non-capturing groups or lookarounds).
  const top = groups.filter((group) => group.depth === 0);
  const segments = [];
  let cursor = 0;
  for (const group of top) {
    if (group.open > cursor) segments.push({ start: cursor, end: group.open, group: null });
    segments.push({ start: group.open, end: group.close + 1, group });
    cursor = group.close + 1;
  }
  if (cursor < src.length) segments.push({ start: cursor, end: src.length, group: null });

  let out = "";
  let count = 0;
  const numberMap = new Map();
  for (const segment of segments) {
    const text = src.slice(segment.start, segment.end);
    if (segment.group) {
      count += 1;
      numberMap.set(segment.group.number, count);
      out += text;
    } else if (isZeroWidthSpan(text)) {
      out += text; // anchors contribute nothing to the match length
    } else {
      count += 1;
      out += `(${text})`; // synthesized group so the gap is covered
    }
  }
  try {
    new RegExp(out);
  } catch (error) {
    problems.push(`rewritten pattern does not compile (${error.message})`);
    return result;
  }
  return { ok: true, regex: out, count, originalCount: groups.length, numberMap, problems: [] };
}

// ------------------------------------------------------------------ captures

function normalizeCaptures(captures, warnAt, where) {
  const result = new Map();
  if (captures === undefined || captures === null) return result;
  if (typeof captures !== "object" || Array.isArray(captures)) {
    warnAt(where, "\"captures\" is not an object; ignored");
    return result;
  }
  for (const [key, value] of Object.entries(captures)) {
    if (!/^\d+$/.test(key)) {
      warnAt(where, `capture key "${key}" is not a group number; ignored`);
      continue;
    }
    let scope = null;
    if (typeof value === "string") {
      scope = value;
    } else if (value !== null && typeof value === "object") {
      if (typeof value.name === "string") scope = value.name;
      else if (value.name !== undefined) warnAt(where, `capture "${key}" has a non-string "name"; ignored`);
    } else if (value !== null && value !== undefined) {
      warnAt(where, `capture "${key}" is neither a scope string nor a {name} object; ignored`);
    }
    result.set(Number(key), scope);
  }
  return result;
}

// Builds `{ regex, whole, group }` for one TextMate pattern. `whole` is the
// single-token action; `group` is the capture token array when Monarch can
// express it (see tilePattern above), otherwise null with a warning.
function planRuleAction(pattern, name, captures, warnAt, where) {
  const regex = convertPattern(pattern, warnAt, where);
  if (regex === null) return null;
  const caps = normalizeCaptures(captures, warnAt, where);
  const whole = pickToken([caps.get(0), name], warnAt, where);
  const groupCaptures = [...caps.keys()].filter((n) => n >= 1).sort((a, b) => a - b);
  if (groupCaptures.length === 0) return { regex, whole, group: null };

  const tiled = tilePattern(regex);
  if (!tiled.originalCount) {
    const keys = groupCaptures.map((n) => `"${n}"`).join(", ");
    warnAt(where, `captures ${keys} reference groups but the pattern has none; using one whole-match token`);
    return { regex, whole, group: null };
  }
  if (!tiled.ok) {
    warnAt(where, `captures cannot become a Monarch token group (${tiled.problems.join("; ")}); using one whole-match token`);
    return { regex, whole, group: null };
  }
  let usable = 0;
  for (const n of groupCaptures) {
    if (n > tiled.originalCount) warnAt(where, `capture group "${n}" does not exist in the pattern; ignored`);
    else usable += 1;
  }
  if (!usable) return { regex, whole, group: null };

  const tokens = [];
  for (let newNumber = 1; newNumber <= tiled.count; newNumber++) {
    let original = null;
    for (const [from, to] of tiled.numberMap) {
      if (to === newNumber) { original = from; break; }
    }
    const captureScope = original === null ? null : caps.get(original);
    tokens.push(pickToken([captureScope, caps.get(0), name], warnAt, where));
  }
  return { regex: tiled.regex, whole, group: tokens };
}

// Appends a state transition to a planned action. Group actions carry the
// transition on their LAST entry only: monarch applies `next` per group entry,
// so putting it on more than one entry would push the state twice.
function withNext(plan, next) {
  if (plan.group) {
    const action = plan.group.slice();
    const last = action[action.length - 1];
    action[action.length - 1] = typeof last === "string" ? { token: last, next } : { ...last, next };
    return action;
  }
  return { token: plan.whole, next };
}

// ------------------------------------------------------------------ naming

const kebab = (text) =>
  String(text).toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-+|-+$/g, "");

// ---------------------------------------------------------------------------
// State naming (tricky part 1 of 3).
//
// Monarch keys its tokenizer states by name and resolves every
// `next: "@name"` at COMPILE time — a reference to an undefined state aborts
// the whole monarch compile — so each name is claimed exactly once and the
// state slot is registered before any rule can point at it.
//
// Scope names become kebab-case (`string.quoted.double` →
// `string-quoted-double`). Dashes instead of dots matter: monarch's runtime
// `findRules()` walks dotted parents (`a.b.c` → `a.b` → `a`) when a state is
// missing, so dotted names could silently rebind to a parent state; dashes
// keep every state independent.
//
// Nameless begin/end rules (spec rule 9) still need a state: they get
// `<hint>-<n>`, where the hint is `string` for quote-like delimiters — the
// classic unnamed string rule lands on `@string-1` — and `state` otherwise.
// ---------------------------------------------------------------------------
function createNamer() {
  // `root` exists up front; `pop`/`push`/`popall` are Monarch keywords that a
  // state name could otherwise shadow (`next: "@pop"` always pops).
  const used = new Set(["root", "pop", "push", "popall"]);
  const anonymous = new Map();
  const claim = (base) => {
    let candidate = base;
    let suffix = 1;
    while (used.has(candidate)) {
      suffix += 1;
      candidate = `${base}-${suffix}`;
    }
    used.add(candidate);
    return candidate;
  };
  const baseFor = (rule) => {
    const named = typeof rule.name === "string" ? kebab(rule.name) : "";
    if (named) return named;
    const hint = typeof rule.begin === "string" && /["'`]/.test(rule.begin) ? "string" : "state";
    const next = (anonymous.get(hint) ?? 0) + 1;
    anonymous.set(hint, next);
    return `${hint}-${next}`;
  };
  return { claim, baseFor };
}

// ------------------------------------------------------------------ conversion

// Diagnostics are de-duplicated on the message itself (first location wins),
// so a grammar with fifty `while` rules reports once instead of fifty times.
function createWarnAt(diagnostics, reported) {
  return (where, message) => {
    if (reported.has(message)) return;
    reported.add(message);
    diagnostics.push(where ? `in ${where}: ${message}` : message);
  };
}

function scanTopLevelFields(grammar, warnAt) {
  for (const key of Object.keys(grammar)) {
    if (IGNORED_GRAMMAR_FIELDS[key]) warnAt("grammar", `ignored ${IGNORED_GRAMMAR_FIELDS[key]}`);
    else if (!KNOWN_GRAMMAR_FIELDS.has(key)) warnAt("grammar", `ignored unknown grammar field "${key}"`);
  }
}

function warnDroppedRuleFields(rule, warnAt, where) {
  if (rule.while !== undefined) {
    warnAt(where, "the \"while\" construct is not supported by Monarch: the field is dropped (TextMate while-rules have no Monarch equivalent)");
  }
  if (rule.contentName !== undefined) {
    warnAt(where, "the \"contentName\" scope cannot be represented in Monarch; inner rules keep their own tokens");
  }
  if (rule.applyEndPatternLast !== undefined) {
    warnAt(where, "\"applyEndPatternLast\" is ignored: the end pattern is always tried before inner rules");
  }
  for (const key of Object.keys(rule)) {
    if (!KNOWN_RULE_FIELDS.has(key)) warnAt(where, `ignored unsupported rule field "${key}"`);
  }
}

// Child context: same machinery, new position in the expansion path. The
// include stack is copied, so diamond-shaped includes still expand fully on
// each branch while true cycles stop at the stack check.
function enter(ctx, segment) {
  return { ...ctx, where: `${ctx.where}${segment}` };
}

function convertPatternList(list, ctx, target) {
  if (!Array.isArray(list)) return;
  list.forEach((rule, index) => convertRule(rule, enter(ctx, `[${index}]`), target));
}

function convertMatch(pattern, name, captures, ctx, target) {
  const plan = planRuleAction(pattern, name, captures, ctx.warnAt, ctx.where);
  if (plan === null) return;
  target.push([plan.regex, plan.group ?? plan.whole]);
}

function convertBeginEnd(rule, ctx, target) {
  const { warnAt } = ctx;
  if (rule.match !== undefined) warnAt(ctx.where, "\"match\" is ignored alongside \"begin\"/\"end\"");
  if (typeof rule.end !== "string") {
    if (rule.end !== undefined) warnAt(ctx.where, "\"end\" is not a string; rule dropped");
    warnAt(ctx.where, "rule has \"begin\" without \"end\": a Monarch state can never pop, so the rule is dropped");
    return;
  }
  const beginPlan = planRuleAction(rule.begin, rule.name, rule.beginCaptures, warnAt, ctx.where);
  if (beginPlan === null) return;
  const endPlan = planRuleAction(rule.end, rule.name, rule.endCaptures, warnAt, `${ctx.where} (end)`);
  if (endPlan === null) return;

  // Claim the name and register an empty state FIRST, so a failure later in
  // this rule can never leave a `next` pointing at a state that does not
  // exist (which Monaco's compiler treats as fatal).
  const stateName = ctx.namer.claim(ctx.namer.baseFor(rule));
  const stateRules = [];
  ctx.tokenizer[stateName] = stateRules;
  target.push([beginPlan.regex, withNext(beginPlan, `@${stateName}`)]);
  // End rule first: Monarch tries rules in order, and TextMate's default is
  // to check `end` before the inner patterns.
  stateRules.push([endPlan.regex, withNext(endPlan, "@pop")]);
  if (rule.patterns !== undefined) {
    if (!Array.isArray(rule.patterns)) {
      warnAt(ctx.where, "\"patterns\" is not an array; inner rules ignored");
    } else {
      convertPatternList(rule.patterns, enter(ctx, `→${stateName}`), stateRules);
    }
  }
}

// ---------------------------------------------------------------------------
// Include inlining (tricky part 3 of 3).
//
// Monarch has no per-rule includes: repository entries must be expanded at
// their `#name` sites. The context carries the stack of repository keys being
// expanded along the current path:
//   • a key already on the stack is a cycle — the rule was already emitted at
//     its first expansion, so the repeat is skipped and reported once;
//   • the same key reached through two different branches is NOT a cycle, and
//     expands again on each branch (exactly what TextMate's include does);
//   • `$self`/`$base` and external grammar includes have no Monarch
//     equivalent and are skipped with a warning.
// ---------------------------------------------------------------------------
function expandInclude(reference, ctx, target) {
  const { warnAt, repository } = ctx;
  if (reference === "$self" || reference === "$base") {
    warnAt(ctx.where, `include "${reference}" has no Monarch equivalent (a Monarch tokenizer cannot re-enter or share a grammar); skipped`);
    return;
  }
  if (reference.startsWith("#")) {
    const key = reference.slice(1);
    if (!Object.hasOwn(repository, key)) {
      warnAt(ctx.where, `include "${reference}" has no repository entry; skipped`);
      return;
    }
    if (ctx.stack.includes(key)) {
      warnAt(ctx.where, `include cycle at "${reference}": the rule is already emitted at its first expansion; this repeat is skipped`);
      return;
    }
    if (ctx.stack.length >= MAX_INCLUDE_DEPTH) {
      warnAt(ctx.where, `include nesting deeper than ${MAX_INCLUDE_DEPTH} at "${reference}"; skipped`);
      return;
    }
    const child = { ...enter(ctx, `→${reference}`), stack: [...ctx.stack, key] };
    convertRule(repository[key], child, target);
    return;
  }
  warnAt(ctx.where, `include "${reference}" points at another grammar; Monarch has one tokenizer per language, so external includes are skipped`);
}

function convertRule(rule, ctx, target) {
  const { warnAt } = ctx;
  if (rule === null || typeof rule !== "object" || Array.isArray(rule)) {
    warnAt(ctx.where, "pattern entry is not an object; skipped");
    return;
  }
  if (typeof rule.include === "string") {
    const extra = Object.keys(rule).filter((key) => key !== "include");
    if (extra.length) {
      warnAt(ctx.where, `rule combines "include" with ${extra.map((k) => `"${k}"`).join(", ")}; only the include is used`);
    }
    expandInclude(rule.include, ctx, target);
    return;
  }
  if (rule.include !== undefined) {
    warnAt(ctx.where, "\"include\" is not a string; skipped");
    return;
  }
  warnDroppedRuleFields(rule, warnAt, ctx.where);

  if (rule.begin !== undefined) {
    convertBeginEnd(rule, ctx, target);
    return;
  }
  if (rule.end !== undefined) {
    warnAt(ctx.where, "rule has \"end\" without \"begin\"; treating the end pattern as a plain match");
    convertMatch(rule.end, rule.name, rule.captures, ctx, target);
    return;
  }
  if (rule.match !== undefined) {
    if (rule.patterns !== undefined) warnAt(ctx.where, "\"patterns\" is ignored alongside \"match\"");
    convertMatch(rule.match, rule.name, rule.captures, ctx, target);
    return;
  }
  if (rule.patterns !== undefined) {
    if (!Array.isArray(rule.patterns)) {
      warnAt(ctx.where, "\"patterns\" is not an array; rule skipped");
      return;
    }
    convertPatternList(rule.patterns, ctx, target);
    return;
  }
  warnAt(ctx.where, "rule has no \"match\", \"begin\", or \"patterns\"; skipped");
}

// Converts a whole grammar. Never throws: unexpected errors end the conversion
// early with a diagnostic, and whatever was built is still returned as valid,
// deterministic JSON.
function convert(grammar, options = {}) {
  const diagnostics = [];
  const reported = new Set();
  const warnAt = createWarnAt(diagnostics, reported);

  const source = grammar !== null && typeof grammar === "object" && !Array.isArray(grammar) ? grammar : {};
  if (source !== grammar) warnAt("grammar", "grammar is not an object; emitting an empty tokenizer");

  const name = typeof source.name === "string" ? source.name : "";
  const scopeName = typeof source.scopeName === "string" ? source.scopeName : "";
  if (source.name !== undefined && !name) warnAt("grammar", "\"name\" is not a usable string; ignored");
  if (source.scopeName !== undefined && !scopeName) warnAt("grammar", "\"scopeName\" is not a usable string; ignored");

  // `name` → language id (kebab-cased), unless --id overrides it; without a
  // usable name the scope's last segment (`source.python` → `python`) is the
  // best fallback before a generic id.
  const derivedName = kebab(name);
  let language;
  if (typeof options.id === "string" && options.id !== "") {
    language = options.id;
  } else {
    const scopeTail = scopeName ? kebab(scopeName.split(".").pop()) : "";
    language = derivedName || scopeTail || "monarch-grammar";
    if (!derivedName) warnAt("grammar", `grammar has no usable "name"; language id is "${language}"`);
  }
  const aliases = [];
  if (derivedName && derivedName !== language) aliases.push(derivedName);
  if (scopeName && scopeName !== language && !aliases.includes(scopeName)) aliases.push(scopeName);

  let repository = {};
  if (source.repository !== undefined && source.repository !== null) {
    if (typeof source.repository === "object" && !Array.isArray(source.repository)) repository = source.repository;
    else warnAt("grammar", "\"repository\" is not an object; treated as empty");
  }

  // `root` must stay the first key: Monarch uses the first tokenizer state as
  // the start state when the definition has no `start`.
  const tokenizer = { root: [] };
  const namer = createNamer();

  try {
    scanTopLevelFields(source, warnAt);
    if (source.patterns !== undefined && !Array.isArray(source.patterns)) {
      warnAt("grammar", "\"patterns\" is not an array; the root state is empty");
    } else if (source.patterns === undefined && Object.keys(repository).length === 0) {
      warnAt("grammar", "grammar has no \"patterns\" or \"repository\"; the root state is empty");
    }
    const ctx = { where: "root", warnAt, namer, tokenizer, repository, stack: [] };
    convertPatternList(source.patterns, ctx, tokenizer.root);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    warnAt("grammar", `conversion stopped early (${message}); emitting the part that was converted`);
  }

  // Deterministic key order: language, aliases, defaultToken, tokenizer.
  const monarch = { language };
  if (aliases.length) monarch.aliases = aliases;
  monarch.defaultToken = DEFAULT_TOKEN;
  monarch.tokenizer = tokenizer;
  return { monarch, diagnostics };
}

// ------------------------------------------------------------------ self-test

// Monaco's monarch compiler (monarchCompile.js) is dependency-free ESM, so it
// can run in Node and act as an oracle: every emitted definition must survive
// the same strict compile the editor performs — regex validity, `@word`
// expansion, and `next:` state existence included. Absent node_modules the
// structural checks still run.
async function loadMonarchCompiler() {
  const file = join(
    dirname(fileURLToPath(import.meta.url)),
    "..", "node_modules", "monaco-editor", "esm", "vs", "editor",
    "standalone", "common", "monarch", "monarchCompile.js",
  );
  try {
    const module = await import(pathToFileURL(file).href);
    return typeof module.compile === "function" ? module.compile : null;
  } catch {
    return null;
  }
}

// Group-action invariant a rule must satisfy at Monarch runtime: array length
// equals the pattern's group count, and groups tile the whole match.
function groupInvariant(regexString, tokens, sample) {
  const match = new RegExp(regexString).exec(sample);
  if (!match) return { ok: false, reason: `sample "${sample}" does not match ${regexString}` };
  if (match.length - 1 !== tokens.length) {
    return { ok: false, reason: `group count ${match.length - 1} !== token count ${tokens.length}` };
  }
  const covered = match.slice(1).reduce((total, group) => total + (group == null ? NaN : group.length), 0);
  if (covered !== match[0].length) {
    return { ok: false, reason: `groups cover ${covered} of ${match[0].length} matched characters` };
  }
  return { ok: true, reason: "" };
}

async function runSelfTest() {
  const compileOracle = await loadMonarchCompiler();
  if (!compileOracle) {
    console.error("tm2monarch: self-test: monaco-editor not installed; skipping compiler-oracle checks");
  }
  let passed = 0;
  let failed = 0;

  const run = (name, fn) => {
    const errors = [];
    const t = {
      ok: (condition, label) => { if (!condition) errors.push(label); },
      eq: (actual, expected, label) => {
        const a = JSON.stringify(actual);
        const e = JSON.stringify(expected);
        if (a !== e) errors.push(`${label}: expected ${e}, got ${a}`);
      },
      includes: (text, needle, label) => {
        if (!String(text).includes(needle)) errors.push(`${label}: expected to find ${JSON.stringify(needle)}`);
      },
      excludes: (text, needle, label) => {
        if (String(text).includes(needle)) errors.push(`${label}: did not expect ${JSON.stringify(needle)}`);
      },
      warns: (diagnostics, needle, label) => {
        if (!diagnostics.some((d) => d.includes(needle))) {
          errors.push(`${label}: no diagnostic mentioning ${JSON.stringify(needle)} in ${JSON.stringify(diagnostics)}`);
        }
      },
      compiles: (monarch, label) => {
        if (!compileOracle) return;
        try {
          compileOracle(monarch.language, JSON.parse(JSON.stringify(monarch)));
        } catch (error) {
          errors.push(`${label}: monarch compiler rejected the output: ${error.message}`);
        }
      },
      invariant: (regexString, tokens, sample, label) => {
        const result = groupInvariant(regexString, tokens, sample);
        if (!result.ok) errors.push(`${label}: ${result.reason}`);
      },
    };
    try {
      fn(t);
    } catch (error) {
      errors.push(`threw: ${error instanceof Error ? error.stack.split("\n")[0] : String(error)}`);
    }
    if (errors.length) {
      failed += 1;
      console.log(`not ok - ${name}`);
      for (const error of errors) console.error(`  ${name}: ${error}`);
    } else {
      passed += 1;
      console.log(`ok - ${name}`);
    }
  };

  // 1. match-only grammar: structure, key order, token mapping, diagnostics.
  run("match-only grammar converts to root rules", (t) => {
    const grammar = {
      name: "Sample Lang",
      scopeName: "source.sample",
      patterns: [
        { match: "\\b(if|else)\\b", name: "keyword.control" },
        { match: "\\d+", name: "constant.numeric" },
        { match: "//.*", name: "comment.line" },
        { match: "zzz", name: "zzz.bogus" },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(Object.keys(monarch), ["language", "aliases", "defaultToken", "tokenizer"], "stable top-level key order");
    t.eq(monarch.language, "sample-lang", "name kebab-cased into language");
    t.eq(monarch.aliases, ["source.sample"], "scopeName recorded as alias");
    t.eq(monarch.defaultToken, "source", "defaultToken");
    t.eq(Object.keys(monarch.tokenizer), ["root"], "root-only tokenizer");
    t.eq(monarch.tokenizer.root, [
      ["\\b(if|else)\\b", "keyword"],
      ["\\d+", "number"],
      ["//.*", "comment"],
      ["zzz", "source"],
    ], "rules in source order with mapped tokens");
    t.warns(diagnostics, "zzz.bogus", "unmapped scope is reported");
    t.warns(diagnostics, "no token mapping", "warning explains the fallback");
    t.excludes(JSON.stringify(monarch), "diagnostics", "diagnostics never leak into the JSON");
    t.compiles(monarch, "match-only");
  });

  // 2. the whole documented scope table, including the entity.name.* wildcard.
  run("scope mapping table covers the documented scopes", (t) => {
    const cases = [
      ["keyword.control", "keyword"],
      ["keyword.operator.assignment", "operator"],
      ["storage.type.class", "keyword"],
      ["string.quoted.double", "string"],
      ["punctuation.definition.string.begin", "string"],
      ["comment.line.double-slash", "comment"],
      ["constant.numeric.integer.hex", "number"],
      ["constant.language.boolean", "keyword"],
      ["entity.name.function.main", "identifier.predeclared"],
      ["support.function.builtin", "identifier.predeclared"],
      ["meta.function-call.method", "identifier.predeclared"],
      ["entity.name.type.class", "type"],
      ["support.type.primitive", "type"],
      ["support.class.dom", "type"],
      ["entity.name.tag.html", "keyword"],
      ["entity.name.label", "type"],
      ["storage.modifier.async", "keyword"],
      ["variable.other.object", "variable"],
      ["variable.parameter.x", "variable.parameter"],
      ["invalid.illegal.bad", "invalid"],
      ["meta.separator.dot", "delimiter"],
      ["source.anything", "source"],
      ["totally.unknown.scope", "source"],
    ];
    const grammar = {
      name: "Scope Lang",
      patterns: cases.map(([scope], index) => ({ match: `m${index}`, name: scope })),
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.tokenizer.root.map((rule) => rule[1]), cases.map(([, token]) => token), "every scope maps as documented");
    t.warns(diagnostics, "totally.unknown.scope", "unknown scope warned");
    t.excludes(diagnostics.join("\n"), "entity.name.label", "wildcard-matched scope does not warn");
    t.excludes(diagnostics.join("\n"), "variable.other.object", "prefix-matched scope does not warn");
    t.compiles(monarch, "scope table");
  });

  // 3. --id wins over the grammar name but keeps it as an alias.
  run("--id overrides the language id and keeps the derived alias", (t) => {
    const { monarch } = convert({ name: "Sample Lang", patterns: [{ match: "x", name: "keyword" }] }, { id: "custom-id" });
    t.eq(monarch.language, "custom-id", "--id used verbatim");
    t.eq(monarch.aliases, ["sample-lang"], "name-derived id kept as alias");
    t.compiles(monarch, "--id");
  });

  // 4. begin/end becomes a state: kebab state name, @pop end rule first,
  //    begin/end captures mapped through capture 0.
  run("begin/end string rule becomes a monarch state", (t) => {
    const grammar = {
      name: "Test String Lang",
      scopeName: "source.test-string",
      patterns: [
        {
          begin: "\"",
          end: "\"",
          name: "string.quoted.double",
          beginCaptures: { "0": { name: "punctuation.definition.string.begin" } },
          endCaptures: { "0": { name: "punctuation.definition.string.end" } },
          patterns: [{ match: "\\S+", name: "string.escape" }],
        },
        { match: "\\bif\\b", name: "keyword.control" },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.language, "test-string-lang", "language id");
    t.eq(Object.keys(monarch.tokenizer), ["root", "string-quoted-double"], "state created with kebab name");
    t.eq(monarch.tokenizer.root[0], ["\"", { token: "string", next: "@string-quoted-double" }], "begin rule pushes the state");
    t.eq(monarch.tokenizer["string-quoted-double"], [
      ["\"", { token: "string", next: "@pop" }],
      ["\\S+", "string"],
    ], "end rule first, inner rules after it");
    t.eq(monarch.tokenizer.root[1], ["\\bif\\b", "keyword"], "sibling rule still in root");
    t.eq(diagnostics, [], "clean grammar produces no diagnostics");
    t.compiles(monarch, "begin/end");
  });

  // 5. captures that already tile the match become a token array directly.
  run("captures map to a token array when groups tile the match", (t) => {
    const grammar = {
      name: "Capture Lang",
      patterns: [
        { match: "\\b(if|else|while)\\b", name: "keyword.control", captures: { "1": { name: "keyword.control" } } },
      ],
    };
    const { monarch } = convert(grammar);
    t.eq(monarch.tokenizer.root[0], ["\\b(if|else|while)\\b", ["keyword"]], "one-element token array for one group");
    t.invariant(monarch.tokenizer.root[0][0], monarch.tokenizer.root[0][1], "if", "group invariant");
    t.compiles(monarch, "tiling-free captures");
  });

  // 6. gapped captures: the gap is wrapped into a synthesized group so the
  //    array covers the whole match, and original group numbers are re-mapped.
  run("gapped captures get synthesized groups", (t) => {
    const grammar = {
      name: "Gap Lang",
      patterns: [
        {
          match: "(\\d+)\\.(\\d+)",
          name: "constant.numeric",
          captures: { "1": { name: "keyword.control" }, "2": { name: "string" } },
        },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.tokenizer.root[0][0], "(\\d+)(\\.)(\\d+)", "gap wrapped into a capture group");
    t.eq(monarch.tokenizer.root[0][1], ["keyword", "number", "string"], "group, synthesized gap, group tokens");
    t.eq(diagnostics, [], "successful tiling needs no warning");
    t.invariant(monarch.tokenizer.root[0][0], monarch.tokenizer.root[0][1], "1.5", "group invariant");
    t.compiles(monarch, "tiled captures");
  });

  // 7. patterns Monarch cannot tile fall back to the whole-match token, loudly.
  run("uncapturable captures fall back with a warning", (t) => {
    const grammar = {
      name: "Fallback Lang",
      patterns: [
        { match: "(a)?b", name: "string", captures: { "1": { name: "keyword" } } },
        { match: "((x)y)", name: "string", captures: { "1": { name: "keyword" } } },
        { match: "x|y(z)", name: "string", captures: { "1": { name: "keyword" } } },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.tokenizer.root.map((rule) => [rule[0], rule[1]]), [
      ["(a)?b", "string"],
      ["((x)y)", "string"],
      ["x|y(z)", "string"],
    ], "original regex kept, whole-match token used");
    const drops = diagnostics.filter((d) => d.includes("cannot become a Monarch token group"));
    t.eq(drops.length, 3, "one fallback warning per uncapturable rule");
    t.includes(drops.join("\n"), "quantified or optional capture group", "optional-group reason reported");
    t.includes(drops.join("\n"), "nested capture group", "nesting reason reported");
    t.includes(drops.join("\n"), "top-level alternation", "alternation reason reported");
    t.compiles(monarch, "capture fallback");
  });

  // 8. repository chains inline completely, with no include keys left behind.
  run("repository include chain is inlined", (t) => {
    const grammar = {
      name: "Chain Lang",
      patterns: [{ include: "#outer" }],
      repository: {
        outer: { patterns: [{ include: "#middle" }] },
        middle: { patterns: [{ include: "#inner" }] },
        inner: { match: "VALUE", name: "entity.name.function" },
      },
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.tokenizer.root, [["VALUE", "identifier.predeclared"]], "chained include flattened into root");
    t.eq(diagnostics, [], "no warnings for a healthy chain");
    t.excludes(JSON.stringify(monarch), "\"include\"", "no include keys survive in the output");
    t.compiles(monarch, "include chain");
  });

    // 9. cycles: a→b→a and s→s stop at the stack, each rule emitted once.
  run("include cycles warn and emit the rule once", (t) => {
    const mutual = convert({
      name: "Cycle Lang",
      patterns: [{ include: "#a" }],
      repository: {
        a: { patterns: [{ include: "#b" }, { match: "AAA", name: "keyword.control" }] },
        b: { patterns: [{ include: "#a" }, { include: "#c" }] },
        c: { match: "CCC", name: "string" },
      },
    });
    t.warns(mutual.diagnostics, "include cycle at \"#a\"", "mutual cycle reported");
    t.warns(mutual.diagnostics, "already emitted", "cycle message explains the emit-once behaviour");
    t.eq(mutual.monarch.tokenizer.root, [["CCC", "string"], ["AAA", "keyword"]], "both branches expanded, cycle skipped");
    const serialized = JSON.stringify(mutual.monarch);
    t.eq(serialized.split("\"AAA\"").length - 1, 1, "AAA emitted exactly once");
    t.eq(serialized.split("\"CCC\"").length - 1, 1, "CCC emitted exactly once");

    const self = convert({
      name: "Self Lang",
      patterns: [{ include: "#s" }],
      repository: { s: { patterns: [{ include: "#s" }, { match: "SELF", name: "keyword" }] } },
    });
    t.warns(self.diagnostics, "include cycle at \"#s\"", "self cycle reported");
    t.eq(self.monarch.tokenizer.root, [["SELF", "keyword"]], "self-inclusion does not recurse");
    t.compiles(mutual.monarch, "mutual cycle");
    t.compiles(self.monarch, "self cycle");
  });

  // 10. `while`: dropped with a clear warning; a begin/while rule with no end
  //     (which would leave a state Monarch can never leave) is dropped too.
  run("unsupported while construct warns and does not crash", (t) => {
    const grammar = {
      name: "While Lang",
      patterns: [
        { begin: "<<<", end: ">>>", while: ".*", name: "string.heredoc" },
        { begin: "%%", while: "\\w+", name: "string.block" },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.warns(diagnostics, "the \"while\" construct is not supported by Monarch", "while warned clearly");
    t.warns(diagnostics, "begin\" without \"end\"", "endless state rule reported");
    t.eq(Object.keys(monarch.tokenizer), ["root", "string-heredoc"], "first rule kept, endless one dropped");
    t.eq(monarch.tokenizer.root, [["<<<", { token: "string", next: "@string-heredoc" }]], "begin rule survived");
    t.eq(monarch.tokenizer["string-heredoc"], [[">>>", { token: "string", next: "@pop" }]], "end rule intact");
    t.compiles(monarch, "while");
  });

  // 11. includes Monarch cannot express are skipped, each with a warning.
  run("$self, $base and external includes warn and skip", (t) => {
    const grammar = {
      name: "Incl Lang",
      patterns: [
        { include: "$self" },
        { include: "$base" },
        { include: "source.js" },
        { include: "#missing" },
        { match: "ok", name: "keyword" },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.tokenizer.root, [["ok", "keyword"]], "only the expressible rule survives");
    t.warns(diagnostics, "$self", "$self warned");
    t.warns(diagnostics, "$base", "$base warned");
    t.warns(diagnostics, "source.js", "external include warned");
    t.warns(diagnostics, "#missing", "missing repository entry warned");
    t.compiles(monarch, "skipped includes");
  });

  // 12. Oniguruma → JavaScript: anchors converted, fatal constructs dropped,
  //     semantic drift reported, `@` doubled for Monarch's attribute expansion.
  run("oniguruma anchors convert and unsupported patterns warn", (t) => {
    const grammar = {
      name: "Re Lang",
      patterns: [
        { match: "\\Afoo\\z", name: "keyword.control" },
        { match: "a*+", name: "keyword.control" },
        { match: "(?~x)y", name: "keyword.control" },
        { match: "\\G\\w+", name: "keyword.control" },
        { match: "[[:alpha:]]+", name: "keyword.control" },
        { match: "@media", name: "keyword.control" },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(monarch.tokenizer.root.length, 4, "two fatal patterns dropped, four kept");
    t.eq(monarch.tokenizer.root[0], ["^foo$", "keyword"], "\\A and \\z became ^ and $");
    t.eq(monarch.tokenizer.root[3], ["@@media", "keyword"], "@ doubled for monarch attribute expansion");
    t.warns(diagnostics, "possessive quantifier", "possessive quantifier reported");
    t.warns(diagnostics, "(?~", "absent-text construct reported");
    t.warns(diagnostics, "\\G", "semantic drift for \\G reported");
    t.warns(diagnostics, "[[:alpha:]]", "POSIX class reported");
    t.compiles(monarch, "regex conversions");
  });

  // 13. every ignored grammar/rule field lands on stderr, never in the JSON.
  run("ignored and unknown fields produce warnings only", (t) => {
    const grammar = {
      name: "Meta Lang",
      uuid: "abc",
      version: 3,
      fileTypes: ["lang"],
      firstLineMatch: "^#!",
      foldingStartMarker: "a",
      foldingStopMarker: "b",
      applyEndPatternLast: true,
      customField: 1,
      patterns: [
        { match: "x", name: "keyword", contentName: "inner", applyEndPatternLast: false, weirdField: 1 },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    const text = diagnostics.join("\n");
    for (const field of ["fileTypes", "firstLineMatch", "foldingStartMarker", "foldingStopMarker", "applyEndPatternLast", "contentName", "customField", "weirdField"]) {
      t.includes(text, field, `diagnostic mentions ${field}`);
    }
    t.excludes(text, "\"uuid\"", "metadata fields stay silent");
    t.excludes(text, "\"version\"", "metadata fields stay silent");
    const serialized = JSON.stringify(monarch);
    t.excludes(serialized, "fileTypes", "dropped fields are not in the JSON");
    t.excludes(serialized, "contentName", "dropped fields are not in the JSON");
    t.eq(monarch.tokenizer.root, [["x", "keyword"]], "rule still converts");
    t.compiles(monarch, "metadata");
  });

  // 14. never throws, always valid JSON — the worst input still converts.
  run("malformed grammars still emit valid json", (t) => {
    const nullish = convert(null, {});
    const scalar = convert("nope", {});
    const weirdName = convert({ name: 42, patterns: "nope" }, {});
    const badRules = convert({
      name: "Bad Lang",
      patterns: [null, 42, { match: 7 }, { name: "lone" }, { begin: "[", end: "]", name: "string" }, { patterns: "x" }],
    }, {});
    const arrayInput = convert([1, 2], {});

    for (const [label, result] of [["null", nullish], ["scalar", scalar], ["array", arrayInput]]) {
      t.eq(result.monarch.language, "monarch-grammar", `${label} input falls back to a generic id`);
      t.eq(Object.keys(result.monarch.tokenizer), ["root"], `${label} input yields an empty root`);
      t.ok(result.diagnostics.length > 0, `${label} input warns`);
      t.compiles(result.monarch, label);
    }
    t.warns(weirdName.diagnostics, "\"name\" is not a usable string", "bad name reported");
    t.warns(weirdName.diagnostics, "\"patterns\" is not an array", "bad patterns reported");
    t.compiles(weirdName.monarch, "weird name");
    t.warns(badRules.diagnostics, "pattern entry is not an object", "non-object rules reported");
    t.warns(badRules.diagnostics, "pattern is not a string", "non-string match reported");
    t.warns(badRules.diagnostics, "no \"match\", \"begin\", or \"patterns\"", "empty rule reported");
    t.warns(badRules.diagnostics, "\"patterns\" is not an array", "bad inner patterns reported");
    t.eq(Object.keys(badRules.monarch.tokenizer), ["root"], "uncolourable begin rule dropped, no state left behind");
    t.eq(badRules.monarch.tokenizer.root, [], "nothing emittable in root");
    t.compiles(badRules.monarch, "bad rules");
    const roundTrip = JSON.parse(JSON.stringify(badRules.monarch));
    t.eq(roundTrip, badRules.monarch, "output survives a JSON round-trip");
  });

  // 15. nameless begin/end rules get deterministic anonymous states.
  run("anonymous begin/end states are named deterministically", (t) => {
    const grammar = {
      name: "Anon Lang",
      patterns: [
        { begin: "\"", end: "\"", patterns: [] },
        { begin: "'", end: "'", patterns: [] },
        { begin: "/\\*", end: "\\*/" },
      ],
    };
    const { monarch, diagnostics } = convert(grammar);
    t.eq(Object.keys(monarch.tokenizer), ["root", "string-1", "string-2", "state-1"], "quote rules become string-N, others state-N");
    t.eq(monarch.tokenizer.root.map((rule) => rule[1].next), ["@string-1", "@string-2", "@state-1"], "each begin pushes its own state");
    for (const state of ["string-1", "string-2", "state-1"]) {
      t.eq(monarch.tokenizer[state][0][1].next, "@pop", `${state} pops on its end rule`);
    }
    t.eq(diagnostics, [], "anonymous states are not a diagnostic");
    t.compiles(monarch, "anonymous states");
  });

  // 16. identical input (even with reordered JSON keys) → identical bytes.
  run("output is deterministic", (t) => {
    const one = {
      name: "Det Lang",
      patterns: [{ begin: "\"", end: "\"", name: "string.quoted" }, { include: "#r" }],
      repository: { r: { match: "R", name: "keyword" } },
    };
    const reordered = {
      repository: { r: { name: "keyword", match: "R" } },
      patterns: [{ name: "string.quoted", end: "\"", begin: "\"" }, { include: "#r" }],
      name: "Det Lang",
    };
    const a = convert(one);
    const b = convert(one);
    const c = convert(reordered);
    const jsonA = JSON.stringify(a.monarch);
    t.eq(jsonA, JSON.stringify(b.monarch), "same input twice is byte-identical");
    t.eq(jsonA, JSON.stringify(c.monarch), "key order in the source grammar does not change the output");
    t.excludes(jsonA, "warning", "no diagnostic text in the JSON");
    t.compiles(a.monarch, "determinism");
  });

  console.log(`tm2monarch self-test: ${passed} passed, ${failed} failed`);
  return failed === 0;
}

// ------------------------------------------------------------------ cli

function parseArgs(argv) {
  const options = { input: null, out: null, id: null, selfTest: false, help: false };
  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "--self-test") {
      options.selfTest = true;
    } else if (arg === "-o" || arg === "--output") {
      i += 1;
      if (i >= argv.length || argv[i] === "") throw new Error(`${arg} needs a file path`);
      options.out = argv[i];
    } else if (arg === "--id") {
      i += 1;
      if (i >= argv.length || argv[i] === "") throw new Error("--id needs a language id");
      options.id = argv[i];
    } else if (arg === "-h" || arg === "--help") {
      options.help = true;
    } else if (arg.startsWith("-") && arg !== "-") {
      throw new Error(`unknown option ${arg}`);
    } else if (options.input !== null) {
      throw new Error(`unexpected extra argument ${arg}`);
    } else {
      options.input = arg;
    }
  }
  return options;
}

async function main(argv) {
  let options;
  try {
    options = parseArgs(argv);
  } catch (error) {
    console.error(`tm2monarch: ${error.message}`);
    console.error(USAGE);
    return 1;
  }
  if (options.help) {
    console.log(USAGE);
    return 0;
  }
  if (options.selfTest) return (await runSelfTest()) ? 0 : 1;
  if (options.input === null) {
    console.error(USAGE);
    return 1;
  }

  let text;
  try {
    text = options.input === "-" ? readFileSync(0, "utf8") : readFileSync(options.input, "utf8");
  } catch (error) {
    const origin = options.input === "-" ? "stdin" : options.input;
    console.error(`tm2monarch: cannot read ${origin}: ${error.message}`);
    return 1;
  }
  let grammar;
  try {
    grammar = JSON.parse(text);
  } catch (error) {
    console.error(`tm2monarch: input is not valid JSON: ${error.message}`);
    return 1;
  }

  const { monarch, diagnostics } = convert(grammar, { id: options.id });
  for (const diagnostic of diagnostics) console.error(`tm2monarch: warning: ${diagnostic}`);

  const json = `${JSON.stringify(monarch, null, 2)}\n`;
  if (options.out !== null && options.out !== "-") {
    try {
      writeFileSync(options.out, json, "utf8");
    } catch (error) {
      console.error(`tm2monarch: cannot write ${options.out}: ${error.message}`);
      return 1;
    }
    const states = Object.keys(monarch.tokenizer).length;
    console.error(`tm2monarch: wrote ${options.out} (${monarch.language}: ${states} tokenizer state${states === 1 ? "" : "s"})`);
  } else {
    process.stdout.write(json);
  }
  return 0;
}

process.exitCode = await main(process.argv.slice(2));
