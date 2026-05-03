# AU Validation — Tester Instructions

This project ships two Audio Unit instruments. Before each release we need
someone with macOS to confirm that Logic Pro can load them. The Apple tool
`auval` performs the same validation Logic does at startup, so a clean `auval`
run is the gate.

## Plugins to validate

| Plugin              | AU type     | `auval` command                |
|---------------------|-------------|--------------------------------|
| Black Widow Drums   | Instrument  | `auval -v aumu BWDR FXME`      |
| Century Drums       | Instrument  | `auval -v aumu CTDR FXME`      |

Both are JUCE-built `aumu` (music device / instrument) components with a
4-char manufacturer code `FXME`.

## How to run

1. Install the AU components (the `.component` bundles produced by the build)
   into either:
   - `~/Library/Audio/Plug-Ins/Components/` (per-user, no sudo needed), or
   - `/Library/Audio/Plug-Ins/Components/` (system-wide).
2. Open Terminal and run **both** commands above, one at a time.
3. macOS caches AU validation results. If you reinstall a plugin and `auval`
   seems to re-use a stale result, kill the cache:
   ```
   killall -9 AudioComponentRegistrar
   ```
   then re-run `auval`.

## What a passing run looks like

The output ends with:

```
AU VALIDATION SUCCEEDED.
```

and the shell exit code is `0` (`echo $?` prints `0`). Above that you should
see sections labelled `OPENING COMPONENT`, parameter / preset enumeration,
and render tests, with no lines beginning with `error` or `FATAL`.

## What to send back on failure

If either command fails, please reply with **all four** of the following so we
can reproduce the issue without going back and forth:

1. **Full terminal output** of the failing `auval` command — top to bottom.
   The warnings that appear before the failure line are the diagnostic
   information; the failure message alone is rarely enough.
2. **Crash report**, if one was generated. Look in
   `~/Library/Logs/DiagnosticReports/` for files whose name begins with
   `auvaltool` or `auval` and whose timestamp matches the run. Console.app
   → Crash Reports shows the same files with a friendlier UI.
3. **macOS version**: output of `sw_vers -productVersion`.
4. **CPU type**: Apple Silicon (`arm64`) or Intel (`x86_64`). `uname -m`
   prints it.

If `auval` hangs instead of failing, wait at least 60 seconds, then `Ctrl+C`
and send the partial output plus a sample of the `auval` process taken with
Activity Monitor (Spindump → Sample Process).
