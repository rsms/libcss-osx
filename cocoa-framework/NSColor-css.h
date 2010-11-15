#import <libcss/libcss.h>

@interface NSColor (css)

+ (NSColor*)colorWithCSSColor:(css_color)rgba;

@end
