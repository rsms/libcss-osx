
@interface NSString (wapcaplet)

+ (NSString*)stringWithLWCString:(struct lwc_string_s*)str;

/// returns a new reference (not autoreleased) i.e. implies lwc_string_ref.
- (struct lwc_string_s*)LWCString;
@end
