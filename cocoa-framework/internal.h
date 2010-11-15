
#define MAKE_EXC(_name, _reason) \
  [NSException exceptionWithName:(_name) reason:(_reason) userInfo:nil]

#define THROW_EXC(_name, _reason) [MAKE_EXC(_name, _reason) raise]

#define CSS_LOG_ERROR(status, label) \
  NSLog(@"[CSS.framework] %s => %s", label, css_error_to_string(status))

BOOL CSSCheck(css_error status);
NSException *CSSCheck2(css_error status);

#import "css-cf-realloc.h"
#import "h-objc.h"
#import "NSString-wapcaplet.h"
#import "NSError-css.h"
#import "NSColor-css.h"
