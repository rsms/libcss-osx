
extern NSString *CSSErrorDomain;

@interface NSError (CSS)
+ (NSError*)CSSErrorFromStatus:(int)status;
+ (NSError*)HTTPErrorWithStatusCode:(int)status;
@end
