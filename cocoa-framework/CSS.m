#import "CSS.h"
#import "internal.h"

@interface CSS : NSObject {
}
@end
@implementation CSS


+ (void)load {
  NSAutoreleasePool *pool = [NSAutoreleasePool new];
  
	// initialise libwapcaplet (required by libcss)
	int c = lwc_initialise(&css_cf_realloc, NULL, 0);
	if (c != lwc_error_ok) {
		NSLog(@"[CSS.framework] FATAL lwc_initialise: %i", c);
		exit(1);
	}
  
  // find path to charset aliases file
  NSBundle *fwBundle = [NSBundle bundleForClass:[self class]];
  NSString *aliasesFile = nil;
  if (fwBundle)
    aliasesFile = [fwBundle pathForResource:@"Aliases" ofType:nil];
  
  // initialise libcss */
	c = css_initialise(aliasesFile ? [aliasesFile UTF8String] : NULL,
                     &css_cf_realloc, 0);
	if (c != CSS_OK)
		NSLog(@"[CSS.framework] FATAL css_initialise: %d", c);
  
  [pool drain];
}

@end
