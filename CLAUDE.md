# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the official open-source release of **Command & Conquer Red Alert** (1996) by Westwood Studios / Electronic Arts. The code is licensed under GPL v3 and was released to support Steam Workshop integration. This is a **historical preservation** codebase - it does not fully compile in its current state.

## Build System

The original build environment requires vintage tools that are difficult to obtain:
- **Watcom C/C++ v10.6** - for C/C++ source files
- **Borland Turbo Assembler (TASM) v4.0** - for assembly files
- **DirectX 5 SDK** and **DirectX Media 5.1 SDK**
- **Greenleaf Communications Library (GCL)** and **HMI SOS** audio library

Build files use Watcom `wmake` syntax (not GNU make). Key makefiles:
- `WIN32LIB/MAKEFILE` - builds the Westwood Win32 library
- `CODE/ADAMTEMP.MAK` - main game makefile
- `CODE/RULES.MAK` - path definitions and build rules for asset processing

Environment variables expected: `WIN32LIB`, `WIN32VCS`, `COMPILER` (Watcom path)

## Repository Structure

```
CODE/           - Main game source (~277 CPP files, ~300K lines)
WIN32LIB/       - Westwood's proprietary Win32 library (modular)
WWFLAT32/       - Alternative flat memory model library
IPX/            - IPX networking (16/32-bit thunking layer)
VQ/             - VQA video codec library
WINVQ/          - Windows VQA player
LAUNCHER/       - Game launcher (Visual Studio .dsp project)
LAUNCH/         - DOS launcher assembly
TOOLS/          - Asset processing utilities (EXE binaries)
```

## Code Architecture

The game uses a deep inheritance hierarchy documented in `CODE/FUNCTION.H`:

**Screen/Map Classes** (top to bottom):
```
MapeditClass → MouseClass → ScrollClass → HelpClass → TabClass →
SidebarClass → PowerClass → RadarClass → DisplayClass → MapClass → GScreenClass
```

**Game Object Classes** (AbstractClass is root):
```
AbstractClass → ObjectClass → MissionClass → RadioClass → TechnoClass
                                                              ↓
                                            ┌─────────────────┴─────────────────┐
                                       FootClass                         BuildingClass
                                            ↓
                          ┌────────────────┬────────────────┐
                    DriveClass      InfantryClass      AircraftClass
                          ↓
                    ┌─────┴─────┐
                UnitClass    VesselClass
```

**Type Classes** (parallel hierarchy for unit/building definitions):
```
AbstractTypeClass → ObjectTypeClass → TechnoTypeClass → UnitTypeClass, etc.
```

Key source files:
- `FUNCTION.H` - Master include with class hierarchy documentation
- `DEFINES.H` - Game constants and enumerations
- `EXTERNS.H` - Global variable declarations
- `HOUSE.CPP/H` - Player/faction logic
- `CELL.CPP/H` - Map tile handling
- `INFANTRY.CPP`, `UNIT.CPP`, `AIRCRAFT.CPP`, `BUILDING.CPP` - Unit implementations

## WIN32LIB Components

The library is built from modular subdirectories:
- `AUDIO/` - Sound system
- `DRAWBUFF/` - Graphics buffer operations
- `KEYBOARD/` - Input handling
- `SHAPE/` - Sprite/shape rendering
- `TILE/` - Tile graphics
- `TIMER/` - Game timing
- `PALETTE/` - Color palette management
- `WSA/` - Westwood Studios Animation format

## Important Notes

- This repository is archived and does not accept contributions
- Missing dependencies (GCL, HMI SOS) must be replaced or stubbed to compile
- Assembly files (`.ASM`) use 16-bit and 32-bit x86 with TASM syntax
- Asset files are processed by tools in `TOOLS/` directory (MIXFILE, KEYFRAME, AUDIOMAK, etc.)
- Localization exists for English, French, and German
