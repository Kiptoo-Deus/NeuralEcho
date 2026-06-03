use crossbeam::queue::ArrayQueue;
use std::sync::Arc;

/// Single-producer single-consumer lock-free ring buffer.
/// Wraps crossbeam's ArrayQueue with a shared Arc for easy cloning.
pub struct SpscRing<T> {
    inner: Arc<ArrayQueue<T>>,
}

impl<T> SpscRing<T> {
    pub fn new(capacity: usize) -> Self {
        Self {
            inner: Arc::new(ArrayQueue::new(capacity)),
        }
    }

    /// Non-blocking push. Returns Err(item) if the queue is full.
    pub fn push(&self, item: T) -> Result<(), T> {
        self.inner.push(item)
    }

    /// Non-blocking pop. Returns None if the queue is empty.
    pub fn pop(&self) -> Option<T> {
        self.inner.pop()
    }

    pub fn len(&self) -> usize  { self.inner.len() }
    pub fn is_empty(&self) -> bool { self.inner.is_empty() }
    pub fn capacity(&self) -> usize { self.inner.capacity() }
}

impl<T> Clone for SpscRing<T> {
    fn clone(&self) -> Self {
        Self { inner: Arc::clone(&self.inner) }
    }
}
