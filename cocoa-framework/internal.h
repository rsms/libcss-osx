
#define MAKE_EXC(_name, ... ) \
  [NSException exceptionWithName:(_name) \
   reason:[NSString stringWithFormat:__VA_ARGS__] userInfo:nil]

#define THROW_EXC(_name, ... ) \
  [NSException raise:(_name) format:__VA_ARGS__]

#define CSS_LOG_ERROR(status, label) \
  NSLog(@"[CSS.framework] %s => %s", label, css_error_to_string(status))

BOOL CSSCheck(css_error status);
NSException *CSSCheck2(css_error status);

#import "css-cf-realloc.h"
#import "h-objc.h"
#import "NSString-wapcaplet.h"
#import "NSError-css.h"
#import "NSColor-css.h"
