
@interface CSSStylesheet : NSObject {
  struct css_stylesheet *sheet_;
  NSURL* url_;
  volatile uint32_t hasStartedLoading_;
}

@property(readonly, nonatomic) NSURL* url;

- (id)initWithURL:(NSURL*)url;

#pragma mark -
#pragma mark Parsing data

- (BOOL)appendData:(NSData*)data
             error:(NSError**)outError
       expectsMore:(BOOL*)expectsMore;

/// load from buffered data and invoke |callback| when loaded.
- (void)finalizeWithCallback:(void(^)(NSError *error))callback;

#pragma mark -
#pragma mark Loading external data

/// load |data| and invoke |callback| when loaded.
- (void)loadData:(NSData*)data withCallback:(void(^)(NSError *error))callback;

/// load |url_| asynchronously and invoke |callback| when loaded.
- (BOOL)loadFromRepresentedURLWithCallback:(void(^)(NSError *error))callback;

@end
