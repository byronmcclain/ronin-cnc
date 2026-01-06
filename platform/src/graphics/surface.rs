//! Software surface for off-screen rendering

use crate::error::PlatformError;

/// Software surface (8-bit indexed)
pub struct Surface {
    pub pixels: Vec<u8>,
    pub width: i32,
    pub height: i32,
    pub pitch: i32,
    pub bpp: i32,
    locked: bool,
}

impl Surface {
    /// Create new surface
    pub fn new(width: i32, height: i32, bpp: i32) -> Self {
        let pitch = width * (bpp / 8);
        let size = (pitch * height) as usize;

        Self {
            pixels: vec![0; size],
            width,
            height,
            pitch,
            bpp,
            locked: false,
        }
    }

    /// Get surface dimensions
    pub fn get_size(&self) -> (i32, i32) {
        (self.width, self.height)
    }

    /// Lock surface for direct pixel access
    pub fn lock(&mut self) -> Result<(*mut u8, i32), PlatformError> {
        if self.locked {
            return Err(PlatformError::Graphics("Surface already locked".into()));
        }
        self.locked = true;
        Ok((self.pixels.as_mut_ptr(), self.pitch))
    }

    /// Unlock surface
    pub fn unlock(&mut self) {
        self.locked = false;
    }

    /// Check if surface is locked
    pub fn is_locked(&self) -> bool {
        self.locked
    }

    /// Blit from another surface with clipping
    pub fn blit(
        &mut self,
        dx: i32,
        dy: i32,
        src: &Surface,
        sx: i32,
        sy: i32,
        sw: i32,
        sh: i32,
    ) -> Result<(), PlatformError> {
        // Clip to destination bounds
        let (dx, dy, sx, sy, sw, sh) = clip_blit(
            dx, dy, sx, sy, sw, sh,
            self.width, self.height,
            src.width, src.height,
        );

        if sw <= 0 || sh <= 0 {
            return Ok(());
        }

        // Validate source coordinates
        if sx < 0 || sy < 0 || sx + sw > src.width || sy + sh > src.height {
            return Err(PlatformError::Graphics("Source coordinates out of bounds".into()));
        }

        // Copy pixels row by row
        for row in 0..sh {
            let src_offset = ((sy + row) * src.pitch + sx) as usize;
            let dst_offset = ((dy + row) * self.pitch + dx) as usize;
            let width = sw as usize;

            // Bounds check
            if src_offset + width > src.pixels.len() || dst_offset + width > self.pixels.len() {
                continue;
            }

            self.pixels[dst_offset..dst_offset + width]
                .copy_from_slice(&src.pixels[src_offset..src_offset + width]);
        }

        Ok(())
    }

    /// Blit with transparency (skip color 0)
    pub fn blit_transparent(
        &mut self,
        dx: i32,
        dy: i32,
        src: &Surface,
        sx: i32,
        sy: i32,
        sw: i32,
        sh: i32,
    ) -> Result<(), PlatformError> {
        let (dx, dy, sx, sy, sw, sh) = clip_blit(
            dx, dy, sx, sy, sw, sh,
            self.width, self.height,
            src.width, src.height,
        );

        if sw <= 0 || sh <= 0 {
            return Ok(());
        }

        for row in 0..sh {
            let src_row_start = ((sy + row) * src.pitch + sx) as usize;
            let dst_row_start = ((dy + row) * self.pitch + dx) as usize;

            for col in 0..sw as usize {
                let src_pixel = src.pixels.get(src_row_start + col).copied().unwrap_or(0);
                if src_pixel != 0 {
                    if let Some(dst_pixel) = self.pixels.get_mut(dst_row_start + col) {
                        *dst_pixel = src_pixel;
                    }
                }
            }
        }

        Ok(())
    }

    /// Fill rectangle with color
    pub fn fill(&mut self, x: i32, y: i32, w: i32, h: i32, color: u8) {
        let x = x.max(0);
        let y = y.max(0);
        let w = w.min(self.width - x);
        let h = h.min(self.height - y);

        if w <= 0 || h <= 0 {
            return;
        }

        for row in 0..h {
            let offset = ((y + row) * self.pitch + x) as usize;
            let width = w as usize;
            if offset + width <= self.pixels.len() {
                self.pixels[offset..offset + width].fill(color);
            }
        }
    }

    /// Clear entire surface
    pub fn clear(&mut self, color: u8) {
        self.pixels.fill(color);
    }

    /// Get pixel at coordinates (bounds checked)
    pub fn get_pixel(&self, x: i32, y: i32) -> Option<u8> {
        if x < 0 || y < 0 || x >= self.width || y >= self.height {
            return None;
        }
        let offset = (y * self.pitch + x) as usize;
        self.pixels.get(offset).copied()
    }

    /// Set pixel at coordinates (bounds checked)
    pub fn set_pixel(&mut self, x: i32, y: i32, color: u8) {
        if x < 0 || y < 0 || x >= self.width || y >= self.height {
            return;
        }
        let offset = (y * self.pitch + x) as usize;
        if let Some(pixel) = self.pixels.get_mut(offset) {
            *pixel = color;
        }
    }
}

/// Clip blit coordinates to destination bounds
fn clip_blit(
    mut dx: i32, mut dy: i32,
    mut sx: i32, mut sy: i32,
    mut sw: i32, mut sh: i32,
    dw: i32, dh: i32,
    src_w: i32, src_h: i32,
) -> (i32, i32, i32, i32, i32, i32) {
    // Clip to source bounds first
    if sx < 0 {
        dx -= sx;
        sw += sx;
        sx = 0;
    }
    if sy < 0 {
        dy -= sy;
        sh += sy;
        sy = 0;
    }
    if sx + sw > src_w {
        sw = src_w - sx;
    }
    if sy + sh > src_h {
        sh = src_h - sy;
    }

    // Clip to destination bounds
    if dx < 0 {
        sx -= dx;
        sw += dx;
        dx = 0;
    }
    if dy < 0 {
        sy -= dy;
        sh += dy;
        dy = 0;
    }
    if dx + sw > dw {
        sw = dw - dx;
    }
    if dy + sh > dh {
        sh = dh - dy;
    }

    (dx, dy, sx, sy, sw, sh)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_surface_creation() {
        let surface = Surface::new(100, 100, 8);
        assert_eq!(surface.width, 100);
        assert_eq!(surface.height, 100);
        assert_eq!(surface.pitch, 100);
        assert_eq!(surface.pixels.len(), 10000);
    }

    #[test]
    fn test_lock_unlock() {
        let mut surface = Surface::new(100, 100, 8);
        assert!(!surface.is_locked());

        let result = surface.lock();
        assert!(result.is_ok());
        assert!(surface.is_locked());

        // Double lock should fail
        let result2 = surface.lock();
        assert!(result2.is_err());

        surface.unlock();
        assert!(!surface.is_locked());
    }

    #[test]
    fn test_fill() {
        let mut surface = Surface::new(100, 100, 8);
        surface.fill(10, 10, 20, 20, 42);

        assert_eq!(surface.get_pixel(15, 15), Some(42));
        assert_eq!(surface.get_pixel(5, 5), Some(0));
    }

    #[test]
    fn test_blit() {
        let mut src = Surface::new(50, 50, 8);
        src.fill(0, 0, 50, 50, 100);

        let mut dst = Surface::new(100, 100, 8);
        let result = dst.blit(25, 25, &src, 0, 0, 50, 50);
        assert!(result.is_ok());

        assert_eq!(dst.get_pixel(30, 30), Some(100));
        assert_eq!(dst.get_pixel(10, 10), Some(0));
    }
}
