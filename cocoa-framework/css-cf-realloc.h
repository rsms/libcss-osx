#ifndef CSS_CF_REALLOC_H_
#define CSS_CF_REALLOC_H_

/// cf-aware memory allocator which uses the cf memory pool.
void *css_cf_realloc(void *ptr, size_t size, void *pw);

#endif  // CSS_CF_REALLOC_H_
