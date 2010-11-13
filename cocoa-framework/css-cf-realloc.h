// cf-aware memory allocator which uses the cf memory pool
//   alloc:   MemAlloc(NULL, long size)
//   realloc: MemAlloc(void *ptr, long size)
//   free:    MemAlloc(void *ptr, 0)
void *css_cf_realloc(void *ptr, size_t size, void *pw);
