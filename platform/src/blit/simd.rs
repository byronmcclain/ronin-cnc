//! SIMD-optimized buffer operations
//!
//! This module provides vectorized implementations of performance-critical
//! functions. All functions have the same signature as their scalar equivalents
//! and automatically fall back to scalar on unsupported platforms.

#[cfg(all(target_arch = "x86_64", feature = "simd"))]
mod x86_64_impl {
    use std::arch::x86_64::*;

    /// Clear buffer using SSE2 (16 bytes per iteration)
    ///
    /// # Safety
    /// - Buffer must be valid for writes
    /// - For maximum performance, buffer should be 16-byte aligned
    #[inline]
    pub unsafe fn buffer_clear_sse2(buffer: &mut [u8], value: u8) {
        let len = buffer.len();
        if len == 0 {
            return;
        }

        let ptr = buffer.as_mut_ptr();

        // Create 16-byte fill value
        let fill = _mm_set1_epi8(value as i8);

        // Process 16 bytes at a time
        let chunks = len / 16;
        let remainder = len % 16;

        for i in 0..chunks {
            let dest = ptr.add(i * 16) as *mut __m128i;
            _mm_storeu_si128(dest, fill);
        }

        // Handle remainder with scalar
        let tail_start = chunks * 16;
        for i in 0..remainder {
            *ptr.add(tail_start + i) = value;
        }
    }

    /// Copy buffer using SSE2 (16 bytes per iteration)
    ///
    /// # Safety
    /// - Source and destination must not overlap
    /// - Both must be valid for their respective operations
    #[inline]
    pub unsafe fn buffer_copy_sse2(src: &[u8], dst: &mut [u8]) {
        let len = src.len().min(dst.len());
        if len == 0 {
            return;
        }

        let src_ptr = src.as_ptr();
        let dst_ptr = dst.as_mut_ptr();

        let chunks = len / 16;
        let remainder = len % 16;

        for i in 0..chunks {
            let s = src_ptr.add(i * 16) as *const __m128i;
            let d = dst_ptr.add(i * 16) as *mut __m128i;
            let data = _mm_loadu_si128(s);
            _mm_storeu_si128(d, data);
        }

        // Handle remainder
        let tail_start = chunks * 16;
        for i in 0..remainder {
            *dst_ptr.add(tail_start + i) = *src_ptr.add(tail_start + i);
        }
    }

    /// Apply remap table using SSE2
    ///
    /// Note: True SIMD gather isn't available until AVX2, so this uses
    /// scalar lookups but vectorized stores. Still provides some benefit
    /// from reduced loop overhead.
    #[inline]
    pub unsafe fn buffer_remap_sse2(
        buffer: &mut [u8],
        remap: &[u8; 256],
    ) {
        let len = buffer.len();
        if len == 0 {
            return;
        }

        let ptr = buffer.as_mut_ptr();

        // Process 16 bytes at a time with scalar lookups
        let chunks = len / 16;
        let remainder = len % 16;

        for i in 0..chunks {
            let base = i * 16;
            // Load 16 source bytes, remap each, store back
            let mut temp = [0u8; 16];
            for j in 0..16 {
                temp[j] = remap[*ptr.add(base + j) as usize];
            }
            let remapped = _mm_loadu_si128(temp.as_ptr() as *const __m128i);
            _mm_storeu_si128(ptr.add(base) as *mut __m128i, remapped);
        }

        // Handle remainder
        let tail_start = chunks * 16;
        for i in 0..remainder {
            let idx = *ptr.add(tail_start + i) as usize;
            *ptr.add(tail_start + i) = remap[idx];
        }
    }
}

#[cfg(all(target_arch = "aarch64", feature = "simd"))]
mod aarch64_impl {
    use std::arch::aarch64::*;

    /// Clear buffer using NEON (16 bytes per iteration)
    #[inline]
    pub unsafe fn buffer_clear_neon(buffer: &mut [u8], value: u8) {
        let len = buffer.len();
        if len == 0 {
            return;
        }

        let ptr = buffer.as_mut_ptr();

        // Create 16-byte fill value
        let fill = vdupq_n_u8(value);

        let chunks = len / 16;
        let remainder = len % 16;

        for i in 0..chunks {
            vst1q_u8(ptr.add(i * 16), fill);
        }

        // Handle remainder
        let tail_start = chunks * 16;
        for i in 0..remainder {
            *ptr.add(tail_start + i) = value;
        }
    }

    /// Copy buffer using NEON (16 bytes per iteration)
    #[inline]
    pub unsafe fn buffer_copy_neon(src: &[u8], dst: &mut [u8]) {
        let len = src.len().min(dst.len());
        if len == 0 {
            return;
        }

        let src_ptr = src.as_ptr();
        let dst_ptr = dst.as_mut_ptr();

        let chunks = len / 16;
        let remainder = len % 16;

        for i in 0..chunks {
            let data = vld1q_u8(src_ptr.add(i * 16));
            vst1q_u8(dst_ptr.add(i * 16), data);
        }

        // Handle remainder
        let tail_start = chunks * 16;
        for i in 0..remainder {
            *dst_ptr.add(tail_start + i) = *src_ptr.add(tail_start + i);
        }
    }

    /// Apply remap table using NEON
    #[inline]
    pub unsafe fn buffer_remap_neon(
        buffer: &mut [u8],
        remap: &[u8; 256],
    ) {
        let len = buffer.len();
        if len == 0 {
            return;
        }

        let ptr = buffer.as_mut_ptr();

        let chunks = len / 16;
        let remainder = len % 16;

        for i in 0..chunks {
            let base = i * 16;
            let mut temp = [0u8; 16];
            for j in 0..16 {
                temp[j] = remap[*ptr.add(base + j) as usize];
            }
            let remapped = vld1q_u8(temp.as_ptr());
            vst1q_u8(ptr.add(base), remapped);
        }

        let tail_start = chunks * 16;
        for i in 0..remainder {
            let idx = *ptr.add(tail_start + i) as usize;
            *ptr.add(tail_start + i) = remap[idx];
        }
    }
}

// =============================================================================
// Public API with automatic platform selection
// =============================================================================

/// Clear buffer with SIMD optimization when available
///
/// Automatically selects:
/// - SSE2 on x86_64 with `simd` feature
/// - NEON on aarch64 with `simd` feature
/// - Scalar fallback otherwise
pub fn buffer_clear_fast(buffer: &mut [u8], value: u8) {
    #[cfg(all(target_arch = "x86_64", feature = "simd"))]
    {
        // Safety: x86_64 always has SSE2
        unsafe { x86_64_impl::buffer_clear_sse2(buffer, value); }
        return;
    }

    #[cfg(all(target_arch = "aarch64", feature = "simd"))]
    {
        // Safety: aarch64 always has NEON
        unsafe { aarch64_impl::buffer_clear_neon(buffer, value); }
        return;
    }

    // Scalar fallback
    #[allow(unreachable_code)]
    {
        buffer.fill(value);
    }
}

/// Copy buffer with SIMD optimization when available
pub fn buffer_copy_fast(src: &[u8], dst: &mut [u8]) {
    #[cfg(all(target_arch = "x86_64", feature = "simd"))]
    {
        unsafe { x86_64_impl::buffer_copy_sse2(src, dst); }
        return;
    }

    #[cfg(all(target_arch = "aarch64", feature = "simd"))]
    {
        unsafe { aarch64_impl::buffer_copy_neon(src, dst); }
        return;
    }

    #[allow(unreachable_code)]
    {
        let len = src.len().min(dst.len());
        dst[..len].copy_from_slice(&src[..len]);
    }
}

/// Apply remap table with SIMD optimization when available
pub fn buffer_remap_fast(buffer: &mut [u8], remap: &[u8; 256]) {
    #[cfg(all(target_arch = "x86_64", feature = "simd"))]
    {
        unsafe { x86_64_impl::buffer_remap_sse2(buffer, remap); }
        return;
    }

    #[cfg(all(target_arch = "aarch64", feature = "simd"))]
    {
        unsafe { aarch64_impl::buffer_remap_neon(buffer, remap); }
        return;
    }

    #[allow(unreachable_code)]
    {
        for pixel in buffer.iter_mut() {
            *pixel = remap[*pixel as usize];
        }
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_buffer_clear_fast() {
        let mut buffer = vec![0xFFu8; 1024];
        buffer_clear_fast(&mut buffer, 0x42);
        assert!(buffer.iter().all(|&b| b == 0x42));
    }

    #[test]
    fn test_buffer_clear_fast_unaligned() {
        // Test with various sizes to ensure remainder handling works
        for size in [1, 7, 15, 16, 17, 31, 32, 33, 127, 128, 129] {
            let mut buffer = vec![0xFFu8; size];
            buffer_clear_fast(&mut buffer, 0x42);
            assert!(buffer.iter().all(|&b| b == 0x42), "failed at size {}", size);
        }
    }

    #[test]
    fn test_buffer_copy_fast() {
        let src = vec![0x42u8; 1024];
        let mut dst = vec![0u8; 1024];
        buffer_copy_fast(&src, &mut dst);
        assert_eq!(src, dst);
    }

    #[test]
    fn test_buffer_copy_fast_unaligned() {
        for size in [1, 7, 15, 16, 17, 31, 32, 33, 127, 128, 129] {
            let src: Vec<u8> = (0..size).map(|i| (i & 0xFF) as u8).collect();
            let mut dst = vec![0u8; size];
            buffer_copy_fast(&src, &mut dst);
            assert_eq!(src, dst, "failed at size {}", size);
        }
    }

    #[test]
    fn test_buffer_remap_fast() {
        // Create identity remap but with +1 offset
        let mut remap = [0u8; 256];
        for i in 0..256 {
            remap[i] = ((i + 1) & 0xFF) as u8;
        }

        let mut buffer: Vec<u8> = (0..256).map(|i| i as u8).collect();
        buffer_remap_fast(&mut buffer, &remap);

        for i in 0..256 {
            assert_eq!(buffer[i], ((i + 1) & 0xFF) as u8, "failed at index {}", i);
        }
    }

    #[test]
    fn test_buffer_remap_fast_unaligned() {
        let remap: [u8; 256] = core::array::from_fn(|i| (255 - i) as u8);

        for size in [1, 7, 15, 16, 17, 31, 32, 33] {
            let mut buffer: Vec<u8> = (0..size).map(|i| i as u8).collect();
            let expected: Vec<u8> = (0..size).map(|i| (255 - i) as u8).collect();
            buffer_remap_fast(&mut buffer, &remap);
            assert_eq!(buffer, expected, "failed at size {}", size);
        }
    }

    #[test]
    fn test_empty_buffers() {
        let mut empty: Vec<u8> = vec![];
        buffer_clear_fast(&mut empty, 0);
        buffer_copy_fast(&[], &mut empty);
        buffer_remap_fast(&mut empty, &[0u8; 256]);
    }

    #[test]
    fn test_simd_matches_scalar() {
        // Verify SIMD results match scalar for various sizes
        let remap: [u8; 256] = core::array::from_fn(|i| (i.wrapping_mul(7).wrapping_add(13) & 0xFF) as u8);

        for size in [1, 15, 16, 17, 100, 127, 128, 129, 255, 256, 1000] {
            // Test clear
            let mut buffer_simd = vec![0xFFu8; size];
            let mut buffer_scalar = vec![0xFFu8; size];

            buffer_clear_fast(&mut buffer_simd, 0x42);
            buffer_scalar.fill(0x42);
            assert_eq!(buffer_simd, buffer_scalar, "clear mismatch at size {}", size);

            // Test copy
            let src: Vec<u8> = (0..size).map(|i| (i & 0xFF) as u8).collect();
            let mut dst_simd = vec![0u8; size];
            let mut dst_scalar = vec![0u8; size];

            buffer_copy_fast(&src, &mut dst_simd);
            dst_scalar.copy_from_slice(&src);
            assert_eq!(dst_simd, dst_scalar, "copy mismatch at size {}", size);

            // Test remap
            let mut remap_simd: Vec<u8> = (0..size).map(|i| (i & 0xFF) as u8).collect();
            let mut remap_scalar: Vec<u8> = remap_simd.clone();

            buffer_remap_fast(&mut remap_simd, &remap);
            for pixel in remap_scalar.iter_mut() {
                *pixel = remap[*pixel as usize];
            }
            assert_eq!(remap_simd, remap_scalar, "remap mismatch at size {}", size);
        }
    }
}
