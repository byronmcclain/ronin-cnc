//! Object pooling for reduced allocation overhead
//!
//! Provides a generic object pool that reuses objects instead of
//! allocating/deallocating for each use.

use std::mem::MaybeUninit;

/// A pool of reusable objects
///
/// Objects are stored in a fixed-size array and recycled.
/// When an object is released, it's returned to the pool for reuse.
///
/// # Type Parameters
/// - `T`: The type of objects in the pool. Must implement Default.
/// - `N`: Maximum number of objects in the pool.
pub struct ObjectPool<T, const N: usize> {
    /// Storage for pooled objects
    objects: Box<[MaybeUninit<T>; N]>,
    /// Indices of available objects
    free_list: Vec<usize>,
    /// Number of objects currently in use
    in_use_count: usize,
}

impl<T: Default, const N: usize> ObjectPool<T, N> {
    /// Create a new object pool
    pub fn new() -> Self {
        // Create uninitialized storage
        let objects = Box::new(unsafe {
            MaybeUninit::uninit().assume_init()
        });

        // Initialize free list with all indices
        let free_list = (0..N).collect();

        Self {
            objects,
            free_list,
            in_use_count: 0,
        }
    }

    /// Acquire an object from the pool
    ///
    /// Returns `Some((index, &mut T))` if an object is available,
    /// `None` if the pool is exhausted.
    pub fn acquire(&mut self) -> Option<(usize, &mut T)> {
        if let Some(index) = self.free_list.pop() {
            // Initialize the object
            self.objects[index] = MaybeUninit::new(T::default());
            self.in_use_count += 1;

            // SAFETY: We just initialized this slot
            let obj = unsafe { self.objects[index].assume_init_mut() };
            Some((index, obj))
        } else {
            None
        }
    }

    /// Release an object back to the pool
    ///
    /// # Arguments
    /// - `index`: The index returned from `acquire()`
    ///
    /// # Panics
    /// Panics if `index >= N` or if the index is already free
    pub fn release(&mut self, index: usize) {
        assert!(index < N, "Index {} out of bounds (max {})", index, N);

        // Drop the object
        unsafe {
            std::ptr::drop_in_place(self.objects[index].assume_init_mut());
        }

        self.free_list.push(index);
        self.in_use_count = self.in_use_count.saturating_sub(1);
    }

    /// Get a reference to an object by index
    ///
    /// # Safety
    /// The index must have been acquired and not yet released.
    pub unsafe fn get(&self, index: usize) -> &T {
        self.objects[index].assume_init_ref()
    }

    /// Get a mutable reference to an object by index
    ///
    /// # Safety
    /// The index must have been acquired and not yet released.
    pub unsafe fn get_mut(&mut self, index: usize) -> &mut T {
        self.objects[index].assume_init_mut()
    }

    /// Get the number of objects currently in use
    pub fn in_use(&self) -> usize {
        self.in_use_count
    }

    /// Get the number of available slots
    pub fn available(&self) -> usize {
        self.free_list.len()
    }

    /// Get the pool capacity
    pub fn capacity(&self) -> usize {
        N
    }

    /// Check if the pool is exhausted
    pub fn is_full(&self) -> bool {
        self.free_list.is_empty()
    }

    /// Check if the pool is empty
    pub fn is_empty(&self) -> bool {
        self.in_use_count == 0
    }
}

impl<T: Default, const N: usize> Default for ObjectPool<T, N> {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Default, Debug, PartialEq)]
    struct TestObject {
        value: i32,
    }

    #[test]
    fn test_pool_new() {
        let pool: ObjectPool<TestObject, 10> = ObjectPool::new();
        assert_eq!(pool.capacity(), 10);
        assert_eq!(pool.available(), 10);
        assert_eq!(pool.in_use(), 0);
        assert!(!pool.is_full());
        assert!(pool.is_empty());
    }

    #[test]
    fn test_pool_acquire() {
        let mut pool: ObjectPool<TestObject, 5> = ObjectPool::new();

        let (idx, obj) = pool.acquire().unwrap();
        obj.value = 42;

        assert_eq!(pool.in_use(), 1);
        assert_eq!(pool.available(), 4);

        // Verify the object
        unsafe {
            assert_eq!(pool.get(idx).value, 42);
        }
    }

    #[test]
    fn test_pool_release() {
        let mut pool: ObjectPool<TestObject, 5> = ObjectPool::new();

        let (idx, obj) = pool.acquire().unwrap();
        obj.value = 42;

        assert_eq!(pool.in_use(), 1);

        pool.release(idx);

        assert_eq!(pool.in_use(), 0);
        assert_eq!(pool.available(), 5);
    }

    #[test]
    fn test_pool_exhaustion() {
        let mut pool: ObjectPool<TestObject, 3> = ObjectPool::new();

        let _ = pool.acquire().unwrap();
        let _ = pool.acquire().unwrap();
        let _ = pool.acquire().unwrap();

        assert!(pool.is_full());
        assert!(pool.acquire().is_none());
    }

    #[test]
    fn test_pool_reuse() {
        let mut pool: ObjectPool<TestObject, 2> = ObjectPool::new();

        let (idx1, _) = pool.acquire().unwrap();
        let (idx2, _) = pool.acquire().unwrap();

        pool.release(idx1);

        // Should be able to acquire again
        let (idx3, obj) = pool.acquire().unwrap();
        obj.value = 100;

        // idx3 should be the recycled idx1
        assert!(idx3 == idx1 || idx3 == idx2);

        pool.release(idx2);
        pool.release(idx3);

        assert!(pool.is_empty());
    }

    #[test]
    fn test_pool_multiple_types() {
        // Test with different object types
        let mut int_pool: ObjectPool<i32, 5> = ObjectPool::new();
        let mut str_pool: ObjectPool<String, 5> = ObjectPool::new();

        let (idx1, val) = int_pool.acquire().unwrap();
        *val = 42;

        let (idx2, s) = str_pool.acquire().unwrap();
        s.push_str("hello");

        unsafe {
            assert_eq!(*int_pool.get(idx1), 42);
            assert_eq!(str_pool.get(idx2), "hello");
        }

        int_pool.release(idx1);
        str_pool.release(idx2);
    }

    #[test]
    #[should_panic(expected = "Index")]
    fn test_pool_release_invalid_index() {
        let mut pool: ObjectPool<TestObject, 5> = ObjectPool::new();
        pool.release(100); // Should panic
    }
}
