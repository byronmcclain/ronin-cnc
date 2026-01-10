//! Asset loading system
//!
//! This module provides functionality for loading game assets from MIX archives
//! and extracting various asset types (palettes, shapes, templates, PCX images, etc.)

pub mod mix;
pub mod palette;
pub mod pcx;
pub mod shape;
pub mod template;

pub use mix::{MixFile, MixManager};
pub use palette::{Palette, PaletteEntry};
pub use pcx::PcxImage;
pub use shape::ShapeFile;
pub use template::TemplateFile;
