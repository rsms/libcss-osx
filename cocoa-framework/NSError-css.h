
extern NSString *CSSErrorDomain;

@interface NSError (CSS)
+ (NSError*)libcssErrorFromStatus:(int)status;
+ (NSError*)libcssHTTPErrorWithStatusCode:(int)status;
@end
