# Fake Angular for ESP32 Design

## Goal

Build an Angular-shaped authoring model for the ESP32 web UI without shipping Angular, Node, or a browser-side framework runtime.

The source of truth will eventually be small JavaScript-ish component files that look Angular-inspired. A custom pre-build compiler will translate those files into C code and header data structures that the ESP32 firmware can compile directly.

For now, the existing hand-written site stays in place as the runtime target and reference implementation.

## Design Direction

The system should have three layers:

1. Authoring layer
   Files that look like minimal Angular:
   - component metadata
   - template string
   - styles string
   - state fields
   - methods / lifecycle hooks

2. Compiler layer
   A pre-build step that parses the authoring files and emits:
   - C structs for state and metadata
   - C functions for recompute/update logic
   - HTML/CSS string blobs
   - binding tables for interpolation and attributes

3. Runtime layer
   Small C helpers used by firmware code:
   - component definitions
   - template bindings
   - explicit render/update functions
   - sensor polling adapters

## Minimal Angular Surface Area

We are not reimplementing Angular. We only need the pieces required for the current UI:

- `@Component`-like metadata
- interpolation such as `{{reading}}`
- property binding such as `[cx]="ankleX"`
- event binding such as `(click)="handler()"`
- plain state fields
- explicit recompute hook
- `ngOnInit`-style startup hook
- optional `if` and `for` later

Out of scope:

- dependency injection
- RxJS
- modules
- router
- zones
- full template syntax compatibility

## Proposed Runtime Shape

The runtime is C-first. The compiler should target the structs in `include/faux_angular_runtime.h`.

Core ideas:

- each component gets a metadata struct
- each component gets an app-specific state struct
- template bindings are explicit tables, not reflection
- recompute is explicit, not change-detection magic
- rendering is string generation plus targeted value substitution

## Proposed Build Flow

PlatformIO should remain the top-level build entrypoint.

Planned flow:

1. Write fake Angular source files in a future `ui/` folder.
2. Add a pre-build script to `platformio.ini` with `extra_scripts`.
3. The script runs before compilation.
4. The script parses the UI source files.
5. The script emits generated C/H files into `src/generated/` and `include/generated/`.
6. The normal ESP32 C/C++ compile then picks those files up automatically.

The right place to attach this is PlatformIO's pre-build hook, not by patching the Espressif compiler itself. That keeps the toolchain boring and maintainable while still letting us feed generated C into the build.

## JS-to-C Compiler Plan

The compiler should be a small local script, probably Python first because PlatformIO already runs comfortably with Python.

Input:

- a restricted JS-like component file
- maybe one file per component

Output:

- generated header with state struct declarations
- generated C/C++ source with:
  - template strings
  - styles strings
  - binding descriptors
  - recompute functions
  - render helpers

Compiler stages:

1. Tokenize a tiny supported syntax.
2. Parse component metadata and fields.
3. Parse templates for interpolation and bindings.
4. Build a binding table.
5. Emit C identifiers and string constants.
6. Emit component state structs and lifecycle function skeletons.

## Constraints

- no package installs
- no dependency on the real Angular compiler
- generated code must be readable
- runtime memory use must stay predictable
- templates should compile to static strings where possible

## Current Recommendation

Start with a tiny fake Angular spec for just this app:

- `AppComponent`
- `KneeVisualizerComponent`
- `ReadingDisplayComponent`
- `SensorService`

Then write a pre-build generator that only supports:

- scalar fields
- interpolation
- attribute binding
- one init hook

That is enough to prove the concept without building a framework by accident.

