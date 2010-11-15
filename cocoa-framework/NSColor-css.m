#import "NSColor-css.h"

@implementation NSColor (css)

+ (NSColor*)colorWithCSSColor:(css_color)rgba {
  const CGFloat rgbaptr[4] = {
    ((rgba >> 24) & 0xFF) / 255.0,
    ((rgba >> 16) & 0xFF) / 255.0,
    ((rgba >> 8) & 0xFF) / 255.0,
    (rgba & 0xFF) / 255.0
  };
  return [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace]
                           components:rgbaptr
                                count:4];
}

@end
