#import "AppDelegate.h"

#import <CSS/CSS.h>

// CSS selection handlers
css_error node_name(void *pw, void *n, lwc_string **name) {
	lwc_string *node = n;
	*name = lwc_string_ref(node);
	return CSS_OK;
}
css_error node_has_name(void *pw, void *n, lwc_string *name, bool *match) {
	lwc_string *node = n;
	assert(lwc_string_caseless_isequal(node, name, match) == lwc_error_ok);
	return CSS_OK;
}

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
    
    // context
    CSSContext* context = [[CSSContext alloc] initWithStylesheet:stylesheet];
    
    // setup select handler
    css_select_handler handler;
    CSSSelectHandlerInitToBase(&handler);
    handler.node_name = &node_name;
    handler.node_has_name = &node_has_name;
    
    // select "body" element
    lwc_string *elementName = [@"body" LWCString];
    CSSStyle *style = [CSSStyle selectStyleForObject:elementName
                                           inContext:context
                                       pseudoElement:0
                                               media:CSS_MEDIA_SCREEN
                                         inlineStyle:nil
                                        usingHandler:&handler];
    lwc_string_unref(elementName); elementName = NULL;
    
    // log color
    NSLog(@"body font(s): %@", style.fontNames);
    NSLog(@"body font size: %f", style.fontSize);
    NSLog(@"body font weight: %d", style.fontWeight);
    NSLog(@"body color: %@", style.color);
    
    [stylesheet release];
  }];
}

@end
