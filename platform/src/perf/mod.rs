//! Performance optimization utilities
//!
//! Provides:
//! - SIMD-optimized routines
//! - Object pooling
//! - Frame timing

pub mod pool;
pub mod simd;
pub mod timing;

pub use pool::ObjectPool;
pub use simd::{convert_palette_fast, PaletteConverter};
pub use timing::{FrameTimer, get_fps, get_frame_time_ms, frame_start, frame_end};

// Re-export FFI functions
pub use timing::{
    Platform_GetFPS,
    Platform_GetFrameTime,
    Platform_FrameStart,
    Platform_FrameEnd,
    Platform_GetFrameCount,
};
