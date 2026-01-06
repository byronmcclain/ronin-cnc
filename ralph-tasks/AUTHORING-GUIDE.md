# Ralph Task Authoring Guide

**Purpose**: This file defines the conventions for creating Ralph task files. Claude should read this file when asked to create new tasks.

## How to Request New Tasks

Tell Claude:
- `"Create Ralph task 03a for SDL window creation"` - specific task
- `"Create all tasks for Phase 03 Graphics"` - entire phase
- `"Create the next task in sequence"` - continue from last completed

Claude will read this guide and produce consistent task files.

---

## Task ID Convention

Format: `[Phase][Letter]` → `03a`, `03b`, `07c`

| Phase | Description |
|-------|-------------|
| 01 | Build System |
| 02 | Platform Abstraction |
| 03 | Graphics System |
| 04 | Input System |
| 05 | Timing System |
| 06 | Memory Management |
| 07 | File System |
| 08 | Audio System |
| 09 | Assembly Rewrites |
| 10 | Networking |
| 11 | Cleanup & Polish |

Letters start at `a` and increment: `03a`, `03b`, `03c`, etc.

---

## File Location

```
ralph-tasks/
├── [NN]-[phase-name]/
│   └── [NN][x]-[short-name].md
└── verify/
    └── check-[short-name].sh
```

Examples:
- `ralph-tasks/03-graphics/03a-sdl-window.md`
- `ralph-tasks/verify/check-sdl-window.sh`

---

## Task File Template

```markdown
# Task [ID]: [Title]

## Dependencies
- Task [X] must be complete
- [Other prerequisites]

## Context
[1-2 sentences about where this fits in the project]

## Objective
[Clear, specific, testable goal]

## Deliverables
- [ ] `path/to/file.rs` - [description]
- [ ] `path/to/test.rs` - [description]
- [ ] Verification passes

## Files to Create

### path/to/file.rs
```rust
[Actual code to write]
```

### path/to/other_file.rs
```rust
[More code]
```

## Verification Command
```bash
[Single command or script that returns 0 on success]
```

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
If stuck after [N] iterations:
- Document what's blocking in `ralph-tasks/blocked/[ID].md`
- List attempted approaches
- Output: <promise>TASK_[ID]_BLOCKED</promise>

## Max Iterations
[10-20 depending on complexity]
```

---

## Verification Script Template

```bash
#!/bin/bash
# Verification script for [description]

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking [Feature] ==="

cd "$ROOT_DIR"

# Step 1: [Description]
if ! [command]; then
    echo "VERIFY_FAILED: [Reason]"
    exit 1
fi

# Step 2: [Description]
if [ ! -f "expected/file" ]; then
    echo "VERIFY_FAILED: [Reason]"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
```

---

## Verification Patterns by Type

### Build Verification
```bash
cargo build --release && test -f target/release/lib*.a
```

### Test Verification
```bash
cargo test --release -- --nocapture
```

### Output Comparison
```bash
./build/test_program --output actual.bin
cmp actual.bin reference/expected.bin
```

### File Existence
```bash
test -f path/to/expected/file.rs
```

### Symbol Check
```bash
nm library.a | grep -q "Expected_Symbol"
```

### Syntax Check
```bash
clang -fsyntax-only -x c header.h
```

---

## Task Complexity Guidelines

| Complexity | Max Iterations | Example |
|------------|----------------|---------|
| Simple | 5-8 | Add single function, fix syntax |
| Medium | 10-15 | New module, integrate library |
| Complex | 15-20 | Multi-file feature, debugging |
| Very Complex | 20-30 | Performance optimization, protocol |

---

## Phase Task Lists

### Phase 01: Build System
- [x] 01a: Rust crate skeleton
- [x] 01b: cbindgen setup
- [ ] 01c: CMake integration with corrosion
- [ ] 01d: Mixed C++/Rust link test

### Phase 02: Platform Abstraction
- [ ] 02a: Error types and FFI safety
- [ ] 02b: Core FFI exports
- [ ] 02c: Header compatibility test

### Phase 03: Graphics System
- [ ] 03a: SDL2 window creation
- [ ] 03b: Surface management
- [ ] 03c: Texture streaming
- [ ] 03d: Palette conversion
- [ ] 03e: Full render pipeline

### Phase 04: Input System
- [ ] 04a: Event pump setup
- [ ] 04b: Keyboard mapping
- [ ] 04c: Mouse handling

### Phase 05: Timing System
- [ ] 05a: High-precision timer
- [ ] 05b: Periodic callbacks
- [ ] 05c: Frame rate limiter

### Phase 06: Memory Management
- [ ] 06a: Allocator implementation
- [ ] 06b: Debug tracking
- [ ] 06c: C compatibility layer

### Phase 07: File System
- [ ] 07a: Path normalization
- [ ] 07b: File I/O operations
- [ ] 07c: MIX file parsing
- [ ] 07d: Case-insensitive lookup

### Phase 08: Audio System
- [ ] 08a: Audio device setup
- [ ] 08b: WAV playback
- [ ] 08c: ADPCM decoder
- [ ] 08d: Sound mixing

### Phase 09: Assembly Rewrites
- [ ] 09a: Blitting routines
- [ ] 09b: Scaling routines
- [ ] 09c: LCW compression
- [ ] 09d: CRC and random
- [ ] 09e: SIMD optimization

### Phase 10: Networking
- [ ] 10a: enet initialization
- [ ] 10b: Host creation
- [ ] 10c: Client connection
- [ ] 10d: Packet transmission
- [ ] 10e: Game protocol adapter

### Phase 11: Cleanup & Polish
- [ ] 11a: Performance baseline
- [ ] 11b: Critical bug fixes
- [ ] 11c: App bundle creation
- [ ] 11d: Universal binary

---

## Reference Files

When creating tasks, refer to:
- `project-plan/[NN]-*.md` - Detailed implementation plans with code
- `ralph-tasks/01-build-system/01a-rust-crate.md` - Example task file
- `ralph-tasks/verify/check-cargo-build.sh` - Example verification script

---

## Quick Commands for Claude

```
"Create Ralph task 03a"
→ Creates ralph-tasks/03-graphics/03a-sdl-window.md

"Create verification script for 03a"
→ Creates ralph-tasks/verify/check-sdl-window.sh

"Create all Phase 03 tasks"
→ Creates 03a through 03e with their verification scripts

"What's the next task to create?"
→ Reads this file, finds first unchecked task

"Mark task 01a complete"
→ Updates this file, checks off 01a
```
