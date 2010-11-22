@class CSSURLConnection;

typedef NSError* (^CSSURLOnResponseBlock)(NSURLResponse*);
typedef NSError* (^CSSURLOnDataBlock)(NSData*);
typedef void (^CSSURLOnCompleteBlock)(NSError*);

@interface NSURL (blocks_CSS)
- (CSSURLConnection*) fetchCSSWithOnResponseBlock:(CSSURLOnResponseBlock)onResponse
                                      onDataBlock:(CSSURLOnDataBlock)onData
                                  onCompleteBlock:(CSSURLOnCompleteBlock)onComplete;
@end

@interface CSSURLConnection : NSURLConnection {
 @public // struct access allowed
  CSSURLOnResponseBlock onResponse;
  CSSURLOnDataBlock onData;
  CSSURLOnCompleteBlock onComplete;
}
- (id)initWithRequest:(NSURLRequest *)request
      onResponseBlock:(CSSURLOnResponseBlock)onResponse
          onDataBlock:(CSSURLOnDataBlock)onData
      onCompleteBlock:(CSSURLOnCompleteBlock)onComplete
             delegate:(id)delegate
     startImmediately:(BOOL)startImmediately;
@end
