# Ralph Wiggum Strategy for Red Alert macOS Port

## Overview

This document describes how to decompose the Red Alert macOS port into **Ralph-compatible tasks** - atomic, testable units of work that can be completed through iterative AI agent loops.

## Core Principles

### 1. Automated Verification is Mandatory
Every Ralph task MUST have a verification step that can be executed without human judgment:
- `cargo build` / `cargo test`
- `cmake --build`
- File existence checks
- Output comparison against reference data
- Integration test scripts

### 2. Atomic Tasks with Clear Boundaries
Each task completes ONE logical unit:
- ❌ "Implement graphics system" (too broad)
- ✅ "Create SDL2 window and verify it opens/closes cleanly"

### 3. File-Based State Persistence
Ralph relies on file changes persisting between iterations:
- All code written to disk
- Git commits mark progress
- Verification scripts can check current state

### 4. Self-Correcting Prompts
Every prompt includes:
- What to build
- How to verify
- What to do if verification fails
- When to output the completion promise

## Task File Format

Create `ralph-tasks/` directory with task definitions:

```
ralph-tasks/
├── 01-build-system/
│   ├── 01a-rust-crate.md
│   ├── 01b-cbindgen.md
│   ├── 01c-cmake.md
│   └── 01d-link-test.md
├── 02-platform/
│   ├── 02a-error-types.md
│   └── ...
├── verify/
│   ├── check-cargo-build.sh
│   ├── check-cbindgen.sh
│   └── ...
└── orchestrator.md
```

### Task Definition Template

```markdown
# Task [ID]: [Title]

## Dependencies
- Task [X] must be complete
- File `platform/src/lib.rs` must exist

## Context
[Brief description of where this fits in the overall project]

## Objective
[Clear, specific goal]

## Deliverables
- [ ] `platform/src/[file].rs` - [description]
- [ ] `tests/[test].rs` - [description]
- [ ] Verification passes

## Verification Command
```bash
./ralph-tasks/verify/[script].sh
```

Expected output: `VERIFY_SUCCESS`

## Implementation Steps
1. [Step 1]
2. [Step 2]
3. Run verification
4. If verification fails, analyze error and fix
5. Repeat until verification passes

## Completion Promise
When verification passes, output:
<promise>TASK_[ID]_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/[ID].md`
- List attempted approaches
- Output: <promise>TASK_[ID]_BLOCKED</promise>

## Max Iterations
20
```

---

## Phase Decomposition

### Phase 01: Build System (4 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 01a | Rust Crate Skeleton | `cargo build --release` succeeds | 3-5 |
| 01b | cbindgen Setup | `platform.h` generated with expected symbols | 5-8 |
| 01c | CMake Integration | `cmake -B build` configures without error | 5-10 |
| 01d | Mixed Link Test | `cmake --build build` produces executable | 8-12 |

**Task 01a: Rust Crate Skeleton**
```
Create the platform/ directory structure:
- platform/Cargo.toml with dependencies (sdl2, rodio, enet, log, thiserror, once_cell)
- platform/src/lib.rs with module declarations
- platform/src/ffi/mod.rs with catch_panic and error handling
- Minimal Platform_Init() and Platform_Shutdown() exports

Verification: `cd platform && cargo build --release`
- Exit code 0 = success
- libredalert_platform.a exists in target/release/

Completion: <promise>TASK_01A_COMPLETE</promise>
```

**Task 01b: cbindgen Setup**
```
Configure cbindgen for automatic header generation:
- platform/cbindgen.toml with C language, include guard, cpp_compat
- platform/build.rs that generates headers on feature flag
- include/platform.h generated with Platform_Init, Platform_Shutdown symbols

Verification: `./ralph-tasks/verify/check-cbindgen.sh`
- platform.h exists
- Contains "Platform_Init" and "Platform_Shutdown"
- C compiler can parse: `clang -fsyntax-only -x c include/platform.h`

Completion: <promise>TASK_01B_COMPLETE</promise>
```

**Task 01c: CMake Integration**
```
Create root CMakeLists.txt:
- CMake 3.20+ required
- Fetch corrosion for Cargo integration
- corrosion_import_crate for platform crate
- Platform detection (APPLE, universal binary support)
- Generate headers target

Verification: `cmake -B build -G Ninja && echo SUCCESS`
- No CMake errors
- build/ directory created
- Corrosion fetched

Completion: <promise>TASK_01C_COMPLETE</promise>
```

**Task 01d: Mixed Link Test**
```
Complete the build system:
- src/main.cpp that calls Platform_Init/Platform_Shutdown
- Link against redalert_platform static library
- Verify executable runs and exits cleanly

Verification:
```bash
cmake --build build && ./build/RedAlert && echo "LINK_SUCCESS"
```

Completion: <promise>TASK_01D_COMPLETE</promise>
```

---

### Phase 02: Platform Abstraction (3 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 02a | Error Types & FFI Safety | `cargo test` passes | 5-8 |
| 02b | Core FFI Exports | nm shows all Platform_* symbols | 5-8 |
| 02c | Header Compatibility | C++ code compiles against platform.h | 3-5 |

---

### Phase 03: Graphics System (5 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 03a | SDL2 Window | Window opens, stays 2s, closes cleanly | 5-10 |
| 03b | Surface Management | Create/destroy surfaces without leaks | 5-8 |
| 03c | Texture Streaming | Lock/unlock texture, blit test pattern | 8-12 |
| 03d | Palette Conversion | Converted pixels match expected RGBA | 5-10 |
| 03e | Full Render Pipeline | Render 8-bit buffer to window | 10-15 |

**Verification Strategy:**
```bash
# Create a graphics test harness
./build/graphics_test --test window_lifecycle
./build/graphics_test --test surface_create
./build/graphics_test --test palette_convert --compare reference/palette_test.png
```

---

### Phase 04: Input System (3 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 04a | Event Pump | Events received, logged to file | 5-8 |
| 04b | Keyboard Mapping | VK codes match expected values | 5-8 |
| 04c | Mouse Handling | Coords, buttons, wheel correct | 5-8 |

---

### Phase 05: Timing System (3 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 05a | High-Precision Timer | 1000 samples within 1ms accuracy | 3-5 |
| 05b | Periodic Callbacks | 100 callbacks at 10ms interval, <5% variance | 5-10 |
| 05c | Frame Rate Limiter | 60 FPS target, actual 58-62 FPS | 5-8 |

**Verification:**
```rust
#[test]
fn test_timer_accuracy() {
    let start = Platform_GetTicks();
    std::thread::sleep(Duration::from_millis(100));
    let elapsed = Platform_GetTicks() - start;
    assert!(elapsed >= 99 && elapsed <= 105); // 5% tolerance
}
```

---

### Phase 06: Memory Management (3 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 06a | Allocator | Alloc/free 1000 blocks, no crash | 3-5 |
| 06b | Debug Tracking | Leak detector catches intentional leak | 5-8 |
| 06c | C Compatibility | VirtualAlloc/HeapAlloc stubs work | 3-5 |

---

### Phase 07: File System (4 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 07a | Path Normalization | Backslash → forward slash, tests pass | 3-5 |
| 07b | File I/O | Read/write roundtrip matches | 3-5 |
| 07c | MIX File Parsing | Extract known file from test MIX | 8-12 |
| 07d | Case-Insensitive | Find "CONQUER.MIX" as "conquer.mix" | 5-8 |

**Task 07c Verification:**
```bash
# Create test MIX file with known content
./build/mix_test --extract test.mix TESTFILE.DAT
diff TESTFILE.DAT reference/TESTFILE.DAT && echo "MIX_PARSE_SUCCESS"
```

---

### Phase 08: Audio System (4 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 08a | Device Setup | Audio device opens/closes | 5-8 |
| 08b | WAV Playback | Play test.wav, no errors | 5-10 |
| 08c | ADPCM Decoder | Decoded matches reference PCM | 8-12 |
| 08d | Mixing | 3 sounds play simultaneously | 5-10 |

**Task 08c Verification:**
```bash
./build/audio_test --decode test.adpcm --output decoded.raw
cmp decoded.raw reference/decoded.raw && echo "ADPCM_SUCCESS"
```

---

### Phase 09: Assembly Rewrites (5 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 09a | Blitting | Output matches reference bitmap | 8-12 |
| 09b | Scaling | Scaled output matches reference | 8-12 |
| 09c | LCW Compression | Compress/decompress roundtrip | 5-10 |
| 09d | CRC & Random | Values match original implementation | 3-5 |
| 09e | SIMD Optimization | Same output, >2x faster | 10-15 |

**Task 09a Verification:**
```rust
#[test]
fn test_blit_transparent() {
    let src = load_test_sprite();
    let mut dest = create_test_buffer();

    buffer_to_buffer_trans(&mut dest, ...);

    let expected = load_reference("blit_trans_expected.bin");
    assert_eq!(dest, expected);
}
```

---

### Phase 10: Networking (5 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 10a | enet Init | Initialize/shutdown cleanly | 3-5 |
| 10b | Host Creation | Bind to port, accept connection | 8-12 |
| 10c | Client Connection | Connect to localhost host | 5-10 |
| 10d | Packet Send/Recv | Echo test passes | 8-12 |
| 10e | Game Protocol | Serialize/deserialize game packets | 5-10 |

**Task 10d Verification:**
```bash
# Terminal 1: Start host
./build/net_test --host 12345 &
HOST_PID=$!

# Terminal 2: Client sends, receives echo
./build/net_test --client localhost:12345 --echo "HELLO"
# Expected output: "ECHO: HELLO"

kill $HOST_PID
```

---

### Phase 11: Cleanup & Polish (4 tasks)

| Task | Name | Verification | Est. Iterations |
|------|------|--------------|-----------------|
| 11a | Performance Baseline | All benchmarks recorded | 5-8 |
| 11b | Critical Bugs | Zero crash bugs in test suite | 10-20 |
| 11c | App Bundle | Bundle structure valid, runs | 8-12 |
| 11d | Universal Binary | Runs on x86_64 and arm64 | 5-10 |

---

## Orchestration Strategy

### Option A: Sequential with Dependencies

Run tasks in order, each depends on previous:

```bash
for task in ralph-tasks/**/*.md; do
  /ralph-loop "$(cat $task)" --max-iterations 20 --completion-promise "TASK_.*_COMPLETE"

  if [ $? -ne 0 ]; then
    echo "Task failed: $task"
    exit 1
  fi
done
```

### Option B: Parallel Where Possible

Some tasks can run in parallel (different modules):

```
Phase 1: Sequential (foundation)
  01a → 01b → 01c → 01d

Phase 2-8: Parallel Tracks (after build works)
  Track A: 03a → 03b → 03c → 03d → 03e (Graphics)
  Track B: 04a → 04b → 04c (Input)
  Track C: 05a → 05b → 05c (Timing)
  Track D: 08a → 08b → 08c → 08d (Audio)

Phase 9-10: Sequential (integration)
  09a → 09b → ... → 10a → 10b → ...

Phase 11: Final (polish)
  11a → 11b → 11c → 11d
```

### Option C: Ralph Orchestrator

Use [ralph-orchestrator](https://github.com/mikeyobrien/ralph-orchestrator) for DAG-based execution:

```yaml
# ralph-tasks/dag.yaml
tasks:
  - id: 01a
    prompt_file: 01-build-system/01a-rust-crate.md
    verify: ./verify/check-cargo-build.sh

  - id: 01b
    depends_on: [01a]
    prompt_file: 01-build-system/01b-cbindgen.md
    verify: ./verify/check-cbindgen.sh

  - id: 03a
    depends_on: [01d]  # Needs complete build
    prompt_file: 03-graphics/03a-sdl-window.md
    verify: ./verify/check-window.sh
```

---

## Verification Scripts

Create reusable verification scripts:

```bash
#!/bin/bash
# ralph-tasks/verify/check-cargo-build.sh

cd platform
cargo build --release 2>&1

if [ $? -eq 0 ] && [ -f target/release/libredalert_platform.a ]; then
    echo "VERIFY_SUCCESS"
    exit 0
else
    echo "VERIFY_FAILED: Cargo build failed or library missing"
    exit 1
fi
```

```bash
#!/bin/bash
# ralph-tasks/verify/check-cbindgen.sh

if [ ! -f include/platform.h ]; then
    echo "VERIFY_FAILED: platform.h not found"
    exit 1
fi

# Check for expected symbols
if grep -q "Platform_Init" include/platform.h && \
   grep -q "Platform_Shutdown" include/platform.h; then
    # Try to compile
    clang -fsyntax-only -x c include/platform.h 2>&1
    if [ $? -eq 0 ]; then
        echo "VERIFY_SUCCESS"
        exit 0
    fi
fi

echo "VERIFY_FAILED: Header incomplete or invalid"
exit 1
```

---

## Estimated Total Iterations

| Phase | Tasks | Est. Iterations | Est. API Cost* |
|-------|-------|-----------------|----------------|
| 01 Build | 4 | 20-35 | $5-10 |
| 02 Platform | 3 | 15-20 | $3-5 |
| 03 Graphics | 5 | 35-55 | $10-15 |
| 04 Input | 3 | 15-25 | $3-5 |
| 05 Timing | 3 | 15-25 | $3-5 |
| 06 Memory | 3 | 10-20 | $2-4 |
| 07 File System | 4 | 20-30 | $5-8 |
| 08 Audio | 4 | 25-40 | $6-10 |
| 09 Assembly | 5 | 35-55 | $10-15 |
| 10 Networking | 5 | 30-50 | $8-12 |
| 11 Polish | 4 | 30-50 | $8-12 |
| **Total** | **43** | **250-400** | **$60-100** |

*Assuming ~$0.25 per iteration with Opus 4

---

## Success Metrics

### Per-Task
- Verification script returns `VERIFY_SUCCESS`
- No uncommitted changes (everything saved)
- Git commit created for completed work

### Per-Phase
- All phase tasks complete
- Integration test passes
- No regressions in previous phases

### Project-Wide
- Game compiles and links
- Game runs on Intel Mac
- Game runs on Apple Silicon Mac
- Single-player mission loads
- Graphics render correctly

---

## Failure Recovery

### When a Task Gets Stuck

1. Check `ralph-tasks/blocked/` for documented blockers
2. Analyze the specific failure
3. Refine the task prompt with more guidance
4. Consider breaking task into smaller sub-tasks
5. Re-run with fresh context

### When Verification Flakes

1. Make verification deterministic
2. Add retries for timing-sensitive tests
3. Use reference data for comparisons
4. Log verbose output for debugging

---

## Getting Started

1. Create the directory structure:
```bash
mkdir -p ralph-tasks/{01-build-system,02-platform,...,verify,blocked}
```

2. Write first task file:
```bash
cat > ralph-tasks/01-build-system/01a-rust-crate.md << 'EOF'
[Task definition here]
EOF
```

3. Create verification script:
```bash
cat > ralph-tasks/verify/check-cargo-build.sh << 'EOF'
[Script here]
EOF
chmod +x ralph-tasks/verify/check-cargo-build.sh
```

4. Run first Ralph loop:
```bash
/ralph-loop "$(cat ralph-tasks/01-build-system/01a-rust-crate.md)" \
    --max-iterations 10 \
    --completion-promise "TASK_01A_COMPLETE"
```

5. Verify and continue:
```bash
./ralph-tasks/verify/check-cargo-build.sh
# If VERIFY_SUCCESS, move to next task
```

---

## References

- [Ralph Methodology](https://ghuntley.com/ralph/)
- [Ralph Orchestrator](https://github.com/mikeyobrien/ralph-orchestrator)
- [Claude Code Ralph Plugin](https://github.com/anthropics/claude-code/tree/main/plugins/ralph-wiggum)
