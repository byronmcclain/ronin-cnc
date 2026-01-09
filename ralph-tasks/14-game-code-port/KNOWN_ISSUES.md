# Phase 14 Known Issues

## Rendering

### No Real Graphics Yet
- All graphics are placeholder colored rectangles
- Actual sprite/shape loading is Phase 15
- Palette loading is stubbed

### No Font Rendering
- Text appears as placeholder dots
- Font system needs asset loading

## Input

### Edge Scrolling Not Implemented
- Only keyboard scrolling works
- Mouse edge scroll needs DisplayClass update

## Objects

### No Object Instantiation
- ObjectClass exists but no units created yet
- Factory system not implemented

## Type Classes

### Incomplete Type Data
- Only sample unit/building types defined
- Full data needs RULES.INI parsing

## Performance

### No Dirty Rect Tracking
- Full screen redrawn every frame
- Optimization needed for large maps

## Audio

### No Sound
- Audio system stubbed
- SFX and music need Phase 16
