# Ralph Tasks for Red Alert macOS Port

This directory contains task definitions for the Ralph Wiggum iterative development methodology.

## Quick Start

```bash
# Run the first task
/ralph-loop "$(cat ralph-tasks/01-build-system/01a-rust-crate.md)" \
    --max-iterations 15 \
    --completion-promise "TASK_01A_COMPLETE"

# Verify it worked
./ralph-tasks/verify/check-cargo-build.sh

# Continue with next task
/ralph-loop "$(cat ralph-tasks/01-build-system/01b-cbindgen.md)" \
    --max-iterations 15 \
    --completion-promise "TASK_01B_COMPLETE"
```

## Directory Structure

```
ralph-tasks/
├── README.md              # This file
├── 01-build-system/       # Phase 1 tasks
│   ├── 01a-rust-crate.md
│   ├── 01b-cbindgen.md
│   ├── 01c-cmake.md
│   └── 01d-link-test.md
├── 02-platform/           # Phase 2 tasks
├── 03-graphics/           # Phase 3 tasks
├── ...
├── verify/                # Verification scripts
│   ├── check-cargo-build.sh
│   ├── check-cbindgen.sh
│   ├── check-cmake.sh
│   └── check-link.sh
└── blocked/               # Blocked task documentation
```

## Task File Format

Each task file contains:
- **Dependencies**: What must be done first
- **Context**: Where this fits in the project
- **Objective**: Clear, specific goal
- **Deliverables**: Files to create
- **Verification Command**: How to verify success
- **Implementation Steps**: Step-by-step guide
- **Completion Promise**: What to output when done
- **Escape Hatch**: What to do if stuck
- **Max Iterations**: Safety limit

## Verification Scripts

All verification scripts:
- Exit with code 0 on success
- Print `VERIFY_SUCCESS` on success
- Print `VERIFY_FAILED: <reason>` on failure
- Are idempotent (can run multiple times)

## Task Dependencies

```
01a ─→ 01b ─→ 01c ─→ 01d
                      │
       ┌──────────────┼──────────────┐
       ↓              ↓              ↓
      02a            03a            05a
       ↓              ↓              ↓
      02b            03b            05b
       ↓              ↓              ↓
      ...            ...            ...
```

## Best Practices

1. **Always verify before moving on**
   ```bash
   ./ralph-tasks/verify/check-*.sh
   ```

2. **Use max-iterations as safety net**
   ```bash
   /ralph-loop "..." --max-iterations 20
   ```

3. **Document blockers**
   If stuck, create `ralph-tasks/blocked/<task-id>.md`

4. **Commit after each task**
   ```bash
   git add -A && git commit -m "Complete task 01a"
   ```

## Estimated Effort

| Phase | Tasks | ~Iterations | ~API Cost |
|-------|-------|-------------|-----------|
| 01 Build | 4 | 25 | $6 |
| 02 Platform | 3 | 18 | $4 |
| 03 Graphics | 5 | 45 | $11 |
| 04 Input | 3 | 20 | $5 |
| 05 Timing | 3 | 18 | $4 |
| 06 Memory | 3 | 15 | $4 |
| 07 Filesystem | 4 | 25 | $6 |
| 08 Audio | 4 | 35 | $9 |
| 09 Assembly | 5 | 45 | $11 |
| 10 Networking | 5 | 40 | $10 |
| 11 Polish | 4 | 40 | $10 |
| **Total** | **43** | **~325** | **~$80** |

## See Also

- [RALPH-STRATEGY.md](../project-plan/RALPH-STRATEGY.md) - Full strategy document
- [Ralph Methodology](https://ghuntley.com/ralph/)
- [Ralph Plugin](https://github.com/anthropics/claude-code/tree/main/plugins/ralph-wiggum)
