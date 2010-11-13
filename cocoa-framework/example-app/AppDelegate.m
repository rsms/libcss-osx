#import "AppDelegate.h"

#import <CSS/CSS.h>

@implementation AppDelegate


- (void)awakeFromNib {
  [inputTextView_ setTextContainerInset:NSMakeSize(2.0, 4.0)];
  [inputTextView_ setFont:[NSFont userFixedPitchFontOfSize:13.0]];
}


- (IBAction)parse:(id)sender {
  NSString *text = inputTextView_.textStorage.string;
  NSData *data = [text dataUsingEncoding:NSUTF8StringEncoding];
  NSLog(@"parsing string '%@' (%u bytes)", text, data.length);
  
  NSURL *url = [NSURL fileURLWithPath:@"/foo/bar/test.css"];
  CSSStylesheet *stylesheet = [[CSSStylesheet alloc] initWithURL:url];
  [stylesheet loadData:data withCallback:^(NSError *err) {
    if (err) {
      NSLog(@"parse: error: %@", err);
      [NSApp presentError:err];
    } else {
      NSLog(@"parse: success");
    }
    [stylesheet release];
  }];
}

@end
