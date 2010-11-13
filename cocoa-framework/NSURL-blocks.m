#import "NSURL-blocks.h"

@implementation HURLConnection

- (id)initWithRequest:(NSURLRequest*)request
      onResponseBlock:(HURLOnResponseBlock)_onResponse
          onDataBlock:(HURLOnDataBlock)_onData
      onCompleteBlock:(HURLOnCompleteBlock)_onComplete
             delegate:(id)delegate
     startImmediately:(BOOL)startImmediately {
  self = [self initWithRequest:request
                      delegate:delegate
              startImmediately:startImmediately];
  if (self) {
    if (_onResponse) onResponse = [_onResponse copy];
    if (_onData) onData = [_onData copy];
    if (_onComplete) onComplete = [_onComplete copy];
  }
  return self;
}


- (void) dealloc {
  if (onResponse) { [onResponse release]; onResponse = nil; }
  if (onData) { [onData release]; onData = nil; }
  if (onComplete) { [onComplete release]; onComplete = nil; }
  [super dealloc];
}

@end



@implementation NSURL (fetch)


- (HURLConnection*)fetchWithOnResponseBlock:(HURLOnResponseBlock)onResponse
                                onDataBlock:(HURLOnDataBlock)onData
                            onCompleteBlock:(HURLOnCompleteBlock)onComplete {
  NSURLRequest *req = 
      [NSURLRequest requestWithURL:self
                       cachePolicy:NSURLRequestUseProtocolCachePolicy
                   timeoutInterval:60.0];
  HURLConnection *conn =
      [[HURLConnection alloc] initWithRequest:req
                              onResponseBlock:onResponse
                                  onDataBlock:onData
                              onCompleteBlock:onComplete
                                     delegate:self
                             startImmediately:YES];
  return conn;
}


- (void)connection:(HURLConnection *)c didReceiveResponse:(NSURLResponse *)re {
  assert([c isKindOfClass:[HURLConnection class]]);
  if (c->onResponse) {
    NSError *error = c->onResponse(re);
    if (error) {
      [c cancel];
      if (c->onComplete) c->onComplete(error);
      [c release];
    }
  }
}


- (void)connection:(HURLConnection *)c didReceiveData:(NSData *)data {
  assert([c isKindOfClass:[HURLConnection class]]);
  if (c->onData) {
    NSError *error = c->onData(data);
    if (error) {
      [c cancel];
      if (c->onComplete) c->onComplete(error);
      [c release];
    }
  }
}


- (void)connection:(HURLConnection *)c didFailWithError:(NSError *)error {
  assert([c isKindOfClass:[HURLConnection class]]);
  if (c->onComplete) c->onComplete(error);
  [c release];
  //NSLog(@"Connection failed! Error - %@ %@",
  //      [error localizedDescription],
  //      [[error userInfo] objectForKey:NSErrorFailingURLStringErrorKey]);
}


- (void)connectionDidFinishLoading:(HURLConnection *)c {
  assert([c isKindOfClass:[HURLConnection class]]);
  if (c->onComplete) c->onComplete(nil);
  [c release];
}


@end
