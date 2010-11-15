#import "CSSContext.h"
#import "CSSStylesheet.h"

#import "internal.h"


inline static CSSStylesheet *_getCSSStylesheet(const css_stylesheet *sheet) {
  CSSStylesheet *stylesheet = nil;
  CSSCheck(css_stylesheet_get_resolve_pw((css_stylesheet *)sheet,
                                         (void **)&stylesheet));
  return stylesheet;
}


@implementation CSSContext

@synthesize ctx = ctx_;


- (id)init {
  if (!(self = [super init])) return nil;

  NSException *e = CSSCheck2(css_select_ctx_create(&css_cf_realloc, 0, &ctx_));
	if (e) {
    [self release];
    [e raise];
    return nil;
  }

  return self;
}


- (id)initWithStylesheet:(CSSStylesheet*)stylesheet {
  if (!(self = [self init])) return nil;
  [self addStylesheet:stylesheet];
  return self;
}


- (void)dealloc {
  for (CSSStylesheet *stylesheet in self) {
    [stylesheet release];
  }
  css_select_ctx_destroy(ctx_);
  [super dealloc];
}



#pragma mark -
#pragma mark Adding, retrieving and removing stylesheets


- (void)addStylesheet:(CSSStylesheet*)stylesheet {
  if (CSSCheck(css_select_ctx_append_sheet(ctx_, stylesheet.sheet,
                                           CSS_ORIGIN_AUTHOR, CSS_MEDIA_ALL))) {
    [stylesheet retain];
  }
}


- (void)insertStylesheet:(CSSStylesheet*)stylesheet atIndex:(NSUInteger)index {
  css_stylesheet *oldSheet = NULL;
  css_select_ctx_get_sheet(ctx_, (uint32_t)index,
                           (const css_stylesheet **)&oldSheet);
  if (CSSCheck(css_select_ctx_insert_sheet(ctx_, stylesheet.sheet, index,
                                           CSS_ORIGIN_AUTHOR, CSS_MEDIA_ALL))) {
    [stylesheet retain];
    if (oldSheet) {
      [_getCSSStylesheet(oldSheet) release];
    }
  } 
}


- (CSSStylesheet*)stylesheetAtIndex:(NSUInteger)index {
  css_stylesheet *sheet;
  if (CSSCheck(css_select_ctx_get_sheet(ctx_, (uint32_t)index,
                                        (const css_stylesheet **)&sheet))) {
    return _getCSSStylesheet(sheet);
  }
  return nil;
}


- (void)removeStylesheet:(CSSStylesheet*)stylesheet {
  if (CSSCheck(css_select_ctx_remove_sheet(ctx_, stylesheet.sheet))) {
    [stylesheet release];
  }
}


- (void)removeStylesheetAtIndex:(NSUInteger)index {
  const css_stylesheet *sheet = NULL;
  if (CSSCheck(css_select_ctx_get_sheet(ctx_, index, &sheet))) {
    if (CSSCheck(css_select_ctx_remove_sheet(ctx_, sheet))) {
      [_getCSSStylesheet(sheet) release];
    }
  }
}


- (NSUInteger)count {
  uint32_t count = 0;
  CSSCheck(css_select_ctx_count_sheets(ctx_, &count));
  return (NSUInteger)count;
}


// NSFastEnumeration implementation

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(id *)stackbuf
                                    count:(NSUInteger)fetchCount {
  uint32_t totalCount = 0;
  CSSCheck(css_select_ctx_count_sheets(ctx_, &totalCount));
  
  NSUInteger fetchIndex = 0;
  while (state->state < totalCount && fetchIndex < fetchCount) {
    stackbuf[fetchIndex++] = [self stylesheetAtIndex:state->state + fetchIndex];
  }
  
  state->state += fetchIndex;
  state->itemsPtr = stackbuf;
  state->mutationsPtr = (unsigned long *)self;  // TODO
  
  return fetchIndex;
}


#pragma mark -
#pragma mark Selecting




@end
