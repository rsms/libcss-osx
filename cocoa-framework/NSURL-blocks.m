#import "NSURL-blocks.h"

@implementation CSSURLConnection

- (id)initWithRequest:(NSURLRequest*)request
      onResponseBlock:(CSSURLOnResponseBlock)_onResponse
          onDataBlock:(CSSURLOnDataBlock)_onData
      onCompleteBlock:(CSSURLOnCompleteBlock)_onComplete
             delegate:(id)delegate
     startImmediately:(BOOL)startImmediately {
  self = [super initWithRequest:request
                      delegate:delegate
              startImmediately:NO];
  if (self) {
    if (_onResponse) onResponse = [_onResponse copy];
    if (_onData) onData = [_onData copy];
    if (_onComplete) onComplete = [_onComplete copy];
    if (startImmediately) [self start];
  }
  return self;
}


- (void)start {
  // schedule in NSDefaultRunLoopMode to support multithreading
  [self scheduleInRunLoop:[NSRunLoop currentRunLoop]
                  forMode:NSDefaultRunLoopMode];
  [super start];
}


- (void) dealloc {
  if (onResponse) { [onResponse release]; onResponse = nil; }
  if (onData) { [onData release]; onData = nil; }
  if (onComplete) { [onComplete release]; onComplete = nil; }
  [super dealloc];
}

@end


@interface CSSURLConnectionDelegate : NSObject {}
@end
@implementation CSSURLConnectionDelegate

- (void)_onComplete:(CSSURLConnection*)c error:(NSError*)err cancel:(BOOL)cancel {
  if (cancel)
    [c cancel];
  if (c->onComplete)
    c->onComplete(err);
  [c release];
  [self release];
}

- (void)connection:(CSSURLConnection*)c didReceiveResponse:(NSURLResponse *)re {
  assert([c isKindOfClass:[CSSURLConnection class]]);
  if (c->onResponse) {
    NSError *error = c->onResponse(re);
    if (error) [self _onComplete:c error:error cancel:YES];
  }
}

- (void)connection:(CSSURLConnection *)c didReceiveData:(NSData *)data {
  assert([c isKindOfClass:[CSSURLConnection class]]);
  if (c->onData) {
    NSError *error = c->onData(data);
    if (error) [self _onComplete:c error:error cancel:YES];
  }
}

- (void)connection:(CSSURLConnection *)c didFailWithError:(NSError *)error {
  assert([c isKindOfClass:[CSSURLConnection class]]);
  [self _onComplete:c error:error cancel:NO];
}

- (void)connectionDidFinishLoading:(CSSURLConnection *)c {
  assert([c isKindOfClass:[CSSURLConnection class]]);
  [self _onComplete:c error:nil cancel:NO];
}

@end



@implementation NSURL (blocks_CSS)

- (CSSURLConnection*)fetchCSSWithOnResponseBlock:(CSSURLOnResponseBlock)onResponse
                                     onDataBlock:(CSSURLOnDataBlock)onData
                                 onCompleteBlock:(CSSURLOnCompleteBlock)onComplete {
  NSURLRequest *req = 
      [NSURLRequest requestWithURL:self
                       cachePolicy:NSURLRequestUseProtocolCachePolicy
                   timeoutInterval:60.0];
  CSSURLConnection *conn =
      [[CSSURLConnection alloc] initWithRequest:req
                              onResponseBlock:onResponse
                                  onDataBlock:onData
                              onCompleteBlock:onComplete
                                     delegate:[CSSURLConnectionDelegate new]
                             startImmediately:YES];
  return conn;
}

@end
