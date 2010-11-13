#import "NSError-css.h"

NSString *CSSErrorDomain = @"CSS";

@implementation NSError (CSS)

+ (NSError*)CSSErrorFromStatus:(int)status {
  NSString *msg = [NSString stringWithUTF8String:css_error_to_string(status)];
  NSDictionary *info =
      [NSDictionary dictionaryWithObject:msg
                                  forKey:NSLocalizedDescriptionKey];
  return [NSError errorWithDomain:CSSErrorDomain code:status userInfo:info];
}

+ (NSError*)HTTPErrorWithStatusCode:(int)status {
  NSString *msg = [NSHTTPURLResponse localizedStringForStatusCode:status];
  NSDictionary *info =
      [NSDictionary dictionaryWithObject:msg forKey:NSLocalizedDescriptionKey];
  return [NSError errorWithDomain:NSURLErrorDomain code:status userInfo:info];
}

@end
