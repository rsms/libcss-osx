@class HURLConnection;

typedef NSError* (^HURLOnResponseBlock)(NSURLResponse*);
typedef NSError* (^HURLOnDataBlock)(NSData*);
typedef void (^HURLOnCompleteBlock)(NSError*);

@interface NSURL (blocks)
- (HURLConnection*) fetchWithOnResponseBlock:(HURLOnResponseBlock)onResponse
                                 onDataBlock:(HURLOnDataBlock)onData
                             onCompleteBlock:(HURLOnCompleteBlock)onComplete;
@end

@interface HURLConnection : NSURLConnection {
 @public // struct access allowed
  HURLOnResponseBlock onResponse;
  HURLOnDataBlock onData;
  HURLOnCompleteBlock onComplete;
}
- (id)initWithRequest:(NSURLRequest *)request
      onResponseBlock:(HURLOnResponseBlock)onResponse
          onDataBlock:(HURLOnDataBlock)onData
      onCompleteBlock:(HURLOnCompleteBlock)onComplete
             delegate:(id)delegate
     startImmediately:(BOOL)startImmediately;
@end
