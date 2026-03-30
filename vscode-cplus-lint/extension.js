'use strict';

const vscode = require('vscode');
const cp     = require('child_process');
const fs     = require('fs');
const os     = require('os');
const path   = require('path');

/** @type {vscode.DiagnosticCollection} */
let diagnosticCollection;

/** @type {Map<string, ReturnType<typeof setTimeout>>} */
const debounceTimers = new Map();

// ---------------------------------------------------------------------------
// Activation / deactivation
// ---------------------------------------------------------------------------

function activate(/** @type {vscode.ExtensionContext} */ context) {
    diagnosticCollection = vscode.languages.createDiagnosticCollection('cplus');
    context.subscriptions.push(diagnosticCollection);

    context.subscriptions.push(
        vscode.workspace.onDidChangeConfiguration(event => {
            if (event.affectsConfiguration('cplusLint.enableDiagnostics')) {
                const config = getConfig();
                if (!config.enableDiagnostics) {
                    diagnosticCollection.clear();
                    debounceTimers.forEach(t => clearTimeout(t));
                    debounceTimers.clear();
                } else {
                    vscode.workspace.textDocuments.forEach(doc => lintDocument(doc));
                }
            }
        }),
        vscode.workspace.onDidOpenTextDocument(doc => lintDocument(doc)),
        vscode.workspace.onDidSaveTextDocument(doc => lintDocument(doc)),
        vscode.workspace.onDidChangeTextDocument(event => onDocumentChange(event.document)),
        vscode.workspace.onDidCloseTextDocument(doc => {
            diagnosticCollection.delete(doc.uri);
            clearTimer(doc.uri.toString());
        })
    );

    // Lint files already open when the extension activates.
    vscode.workspace.textDocuments.forEach(doc => lintDocument(doc));
}

function deactivate() {
    debounceTimers.forEach(t => clearTimeout(t));
    debounceTimers.clear();
    if (diagnosticCollection) {
        diagnosticCollection.dispose();
    }
}

// ---------------------------------------------------------------------------
// Debounced change handler
// ---------------------------------------------------------------------------

function onDocumentChange(/** @type {vscode.TextDocument} */ document) {
    if (document.languageId !== 'cplus' && document.languageId !== 'hplus') { return; }

    const config = getConfig();
    if (!config.enableDiagnostics) { return; }
    if (!config.lintOnChange) { return; }

    const key = document.uri.toString();
    clearTimer(key);
    debounceTimers.set(key, setTimeout(() => {
        debounceTimers.delete(key);
        lintDocumentContent(document, document.getText());
    }, config.lintDelay));
}

function clearTimer(key) {
    const t = debounceTimers.get(key);
    if (t !== undefined) {
        clearTimeout(t);
        debounceTimers.delete(key);
    }
}

// ---------------------------------------------------------------------------
// Lint entry points
// ---------------------------------------------------------------------------

function lintDocument(/** @type {vscode.TextDocument} */ document) {
    if (document.languageId !== 'cplus' && document.languageId !== 'hplus') { return; }
    if (!getConfig().enableDiagnostics) {
        diagnosticCollection.delete(document.uri);
        return;
    }
    lintDocumentContent(document, document.getText());
}

function lintDocumentContent(
    /** @type {vscode.TextDocument} */ document,
    /** @type {string} */ content
) {
    const tmpExt  = document.languageId === 'hplus' ? '.h' : '.c';
    const tmpFile = path.join(os.tmpdir(), `cplus_lint_${process.pid}_${Date.now()}${tmpExt}`);

    try {
        fs.writeFileSync(tmpFile, content, 'utf8');
    } catch (_) {
        return;
    }

    const config  = getConfig();
    const compiler = config.compiler;
    const std      = config.std;
    const includeArgs = buildIncludeArgs(document);

    resolveStdFlag(compiler, std, effectiveStd => {
        const args = ['-x', 'c', `-std=${effectiveStd}`, ...includeArgs, '-fsyntax-only', tmpFile];
        cp.execFile(compiler, args, { timeout: 10000 }, (_err, _stdout, stderr) => {
            try { fs.unlinkSync(tmpFile); } catch (_) {}

            const raw = stderr || '';
            // Remap temp-file path back to the real document path for display.
            const remapped = raw.split('\n').map(line =>
                line.replace(tmpFile, document.fileName)
            ).join('\n');

            const diagnostics = parseDiagnostics(remapped);
            diagnosticCollection.set(document.uri, diagnostics);
        });
    });
}

// ---------------------------------------------------------------------------
// Diagnostic parser
// ---------------------------------------------------------------------------

/**
 * Parses GCC/Clang output lines with the format:
 *   <file>:<line>:<col>: (error|warning|note): <message>
 */
function parseDiagnostics(/** @type {string} */ output) {
    /** @type {vscode.Diagnostic[]} */
    const diagnostics = [];
    const re = /^[^:]+:(\d+):(\d+):\s+(error|warning|note):\s+(.+)$/;

    for (const line of output.split('\n')) {
        const m = line.match(re);
        if (!m) { continue; }

        const lineNum = Math.max(parseInt(m[1], 10) - 1, 0);
        const col     = Math.max(parseInt(m[2], 10) - 1, 0);
        const kind    = m[3];
        const message = m[4];

        const range = new vscode.Range(lineNum, col, lineNum, Number.MAX_SAFE_INTEGER);

        const severity =
            kind === 'error'   ? vscode.DiagnosticSeverity.Error   :
            kind === 'warning' ? vscode.DiagnosticSeverity.Warning :
                                 vscode.DiagnosticSeverity.Information;

        const diag = new vscode.Diagnostic(range, message, severity);
        diag.source = 'cplus-lint';
        diagnostics.push(diag);
    }

    return diagnostics;
}

// ---------------------------------------------------------------------------
// Compiler helpers
// ---------------------------------------------------------------------------

function getConfig() {
    const cfg = vscode.workspace.getConfiguration('cplusLint');
    return {
        enableDiagnostics: cfg.get('enableDiagnostics', false),
        compiler:     cfg.get('compiler', 'gcc'),
        std:          cfg.get('std', 'c23'),
        lintOnChange: cfg.get('lintOnChange', true),
        lintDelay:    cfg.get('lintDelay', 600),
    };
}

function buildIncludeArgs(/** @type {vscode.TextDocument} */ document) {
    const includeDirs = new Set();
    const documentDir = path.dirname(document.fileName);

    includeDirs.add(documentDir);

    for (const folder of vscode.workspace.workspaceFolders || []) {
        includeDirs.add(folder.uri.fsPath);
    }

    const args = [];
    for (const includeDir of includeDirs) {
        args.push('-I', includeDir);
    }

    return args;
}

/**
 * GCC < 14 does not recognise -std=c23; it uses -std=c2x instead.
 * Queries the compiler major version at runtime and remaps accordingly.
 */
function resolveStdFlag(compiler, std, callback) {
    if (std !== 'c23' || compiler !== 'gcc') {
        callback(std);
        return;
    }

    cp.execFile(compiler, ['-dumpversion'], { timeout: 5000 }, (_err, stdout) => {
        const major = parseInt((stdout || '').trim(), 10);
        callback((!isNaN(major) && major > 0 && major < 14) ? 'c2x' : 'c23');
    });
}

// ---------------------------------------------------------------------------

module.exports = { activate, deactivate };
