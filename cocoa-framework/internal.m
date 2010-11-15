#import "internal.h"

NSException *CSSCheck2(css_error status) {
  if (status != CSS_OK) {
    if (status == CSS_BADPARM) {
      return MAKE_EXC(NSInvalidArgumentException, @"CSS.framework");
    } else if (status == CSS_INVALID) {
      return MAKE_EXC(NSRangeException, @"CSS.framework");
    } else if (status == CSS_NOMEM) {
      NSLog(@"[CSS.framework] FATAL: out of memory -- softly terminating");
      exit(1);
    }
  }
  return nil;
}

BOOL CSSCheck(css_error status) {
  NSException *e = CSSCheck2(status);
  if (e) {
    [e raise];
    return NO;
  }
  return YES;
}
