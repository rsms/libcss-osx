@class CSSStylesheet;

@interface CSSContext : NSObject <NSFastEnumeration> {
  css_select_ctx *ctx_;
}

@property(readonly, nonatomic) css_select_ctx *ctx;

- (id)init;
- (id)initWithStylesheet:(CSSStylesheet*)stylesheet;

#pragma mark -
#pragma mark Adding, retrieving and removing stylesheets

- (void)addStylesheet:(CSSStylesheet*)stylesheet;
- (void)insertStylesheet:(CSSStylesheet*)stylesheet atIndex:(NSUInteger)index;
- (CSSStylesheet*)stylesheetAtIndex:(NSUInteger)index;
- (void)removeStylesheet:(CSSStylesheet*)stylesheet;
- (void)removeStylesheetAtIndex:(NSUInteger)index;
- (NSUInteger)count;

/// Fast enumeration support
- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(id *)stackbuf
                                    count:(NSUInteger)len;

#pragma mark -
#pragma mark Selecting



@end
