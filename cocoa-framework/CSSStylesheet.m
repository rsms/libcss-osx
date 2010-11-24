#import "CSSStylesheet.h"
#import "CSSContext.h"
#import "NSURL-blocks.h"
#import "NSError-css.h"
#import "NSString-wapcaplet.h"

#import "internal.h"


static css_error resolve_url(void *pw, const char *base, lwc_string *rel,
                             lwc_string **abs) {
  CSSStylesheet *self = (CSSStylesheet*)pw;
  NSString *rels = [NSString stringWithLWCString:rel];
  NSURL *url = [NSURL URLWithString:rels relativeToURL:self.url];
  *abs = url.absoluteString.LWCString;
  return CSS_OK;
}


static css_error dummy_url_resolver(void *pw, const char *base, lwc_string *rel,
                                    lwc_string **abs) {
  //pw = pw; base = base;
  *abs = lwc_string_ref(rel);
  return CSS_OK;
}


@implementation CSSStylesheet

@synthesize url = url_,
            sheet = sheet_;


- (id)initWithURL:(NSURL*)url {
  self = [super init];

  url_ = [url retain];
  const char *urlpch = url_ ? [[url_ absoluteString] UTF8String] : "";
  bool allow_quirks = false;
  bool inline_style = false;
  css_error status =
      css_stylesheet_create(CSS_LEVEL_DEFAULT, "UTF-8", urlpch, NULL,
                            allow_quirks, inline_style, &css_cf_realloc, NULL,
                            &dummy_url_resolver, self,
                            &sheet_);
	if (status != CSS_OK) {
    CSS_LOG_ERROR(status, "css_stylesheet_create");
    [self release];
    return nil;
  }

  return self;
}


- (void)dealloc {
  css_stylesheet_destroy(sheet_);
  [super dealloc];
}


#pragma mark -
#pragma mark Parsing data


- (BOOL)appendData:(NSData*)data
             error:(NSError**)outError
       expectsMore:(BOOL*)expectsMore {
  assert(data != nil);
  css_error status = css_stylesheet_append_data(sheet_,
                                                (const uint8_t *)data.bytes,
                                                data.length);
  if (status != CSS_OK && status != CSS_NEEDDATA) {
    *outError = [NSError CSSErrorFromStatus:status];
    if (expectsMore != nil) *expectsMore = NO;
    return NO;
  } else if (expectsMore != nil) {
    *expectsMore = (status == CSS_NEEDDATA);
  }
  return YES;
}

// -------------


- (void)_importNext:(void(^)(NSError*))callback {
  callback = [callback copy];

  lwc_string *relurl;
  uint64_t media;
  css_error status =
      css_stylesheet_next_pending_import(sheet_, &relurl, &media);

  if (status == CSS_INVALID) {
    //NSLog(@"end of import chain -- invoking callback");
    callback(nil);
    [callback release];
    return;
  }
  assert(status == CSS_OK);
  
  NSString *relurls = [NSString stringWithLWCString:relurl];
  NSURL *url = [NSURL URLWithString:relurls relativeToURL:url_];
  CSSStylesheet *sheet = [[CSSStylesheet alloc] initWithURL:url];
  //NSLog(@"@import(%@)", url);
  
  [sheet loadFromRepresentedURLWithCallback:^(NSError *error) {
    // Note: even if there's an error we need to register the import
    css_stylesheet_register_import(sheet_, sheet->sheet_);
    [sheet release]; // no longer used since sheet->sheet_ was "eaten"
    // This isn't very nice since multiple errors will only result in one
    if (error) {
      //NSLog(@"end of import chain -- error: %@", error);
      callback(error);
    } else {
      [self _importNext:callback];
    }
    //callback(nil);
    [callback release];
  }];
}


- (void)finalizeWithCallback:(void(^)(NSError*))callback {
  assert(callback);
  //callback = [callback copy];
  //int32_t startedAlready = OSAtomicAnd32Orig(1, &hasStartedLoading_);
  //startedAlready = startedAlready; // STFU, mr compiler
  css_error status = css_stylesheet_data_done(sheet_);
  if (status == CSS_OK) {
    callback(nil);
  } else if (status != CSS_IMPORTS_PENDING) {
    callback([NSError CSSErrorFromStatus:status]);
  } else {
    // handle @imports
    [self _importNext:callback];
  }
  //[callback release];
}


#pragma mark -
#pragma mark Loading external data


- (void)loadData:(NSData*)data withCallback:(void(^)(NSError*))callback {
  NSError *err = nil;
  //callback = [callback copy];
  if (![self appendData:data error:&err expectsMore:nil]) {
    assert(err != nil);
    callback(err);
  } else {
    [self finalizeWithCallback:callback];
  }
  //[callback release];
}


- (BOOL)loadFromRepresentedURLWithCallback:(void(^)(NSError*))callback {
  assert(url_ != nil);
  assert(callback != nil);
  callback = [callback copy];

  return !![url_ fetchCSSWithOnResponseBlock:^(NSURLResponse *response) {
    // check response
    if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
      NSInteger status = [(NSHTTPURLResponse*)response statusCode];
      if (status < 200 || status > 299) {
        return [NSError HTTPErrorWithStatusCode:status];
      }
    }
    return (NSError*)0;
  } onDataBlock:^(NSData *data) {
    // append received data
    css_error status = css_stylesheet_append_data(sheet_,
                                                  (const uint8_t *)data.bytes,
                                                  data.length);
    if (status != CSS_OK && status != CSS_NEEDDATA)
      return [NSError CSSErrorFromStatus:status];
    return (NSError*)0;
  } onCompleteBlock:^(NSError *error) {
    // finalize creation
    if (!error) {
      [self finalizeWithCallback:callback];
    } else {
      callback(error);
    }
    [callback release];
  }];
}


- (NSString*)description {
  return [NSString stringWithFormat:@"<%@@%p url=%@>",
      NSStringFromClass([self class]), self, url_];
}


@end
