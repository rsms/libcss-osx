#import "css-cf-realloc.h"

void *css_cf_realloc(void *ptr, size_t size, void *pw) { pw=pw;
  if (size)
    return CFAllocatorReallocate(kCFAllocatorDefault, ptr, (CFIndex)size, 0);
  CFAllocatorDeallocate(kCFAllocatorDefault, ptr);
  return NULL;
}
