
@class CSSContext, CSSStylesheet;

@interface CSSStyle : NSObject {
  css_computed_style *style_;
}

@property(readonly, nonatomic) css_computed_style *style;

/// Select style for an object
+ (CSSStyle*)selectStyleForObject:(void*)object
                        inContext:(CSSContext*)context
                    pseudoElement:(int)pseudoElement
                            media:(css_media_type)mediaTypes
                      inlineStyle:(CSSStylesheet*)inlineStyle
                     usingHandler:(css_select_handler*)handler;

/**
 * Merge this style (parent) with another style (child). |child| has
 * precedence. A new autoreleased CSSStyle object is returned.
 */
- (CSSStyle*)mergeWith:(CSSStyle*)child;

#pragma mark -
#pragma mark Style properties

// Color. A value of nil means "inherit".
@property(readonly, nonatomic) NSColor
    *color,
    *backgroundColor,
    *outlineColor,
    *borderTopColor,
    *borderRightColor,
    *borderBottomColor,
    *borderLeftColor;

// Border width
@property(readonly, nonatomic) CGFloat
    borderLeftWidth,
    borderTopWidth,
    borderRightWidth,
    borderBottomWidth,
    lineHeight;

// Block dimensions. Note: NaN is returned for e.g. "width:auto"
@property(readonly, nonatomic) CGFloat
    width,
    height;

// Font
@property(readonly, nonatomic) int
    fontWeight, // 100, 400, ...
    fontStyle, // CSS_FONT_STYLE_ITALIC, ...
    fontVariant; // CSS_FONT_VARIANT_SMALL_CAPS..
@property(readonly, nonatomic) CGFloat
    fontSize; // in points
@property(readonly, nonatomic) NSArray
    *fontFamilyNames; // nil means "inherit"
@property(readonly, nonatomic) NSFont
    *font; // nil means "inherit" or no font family could be found

// text
@property(readonly, nonatomic) int
    textDecoration; // CSS_TEXT_DECORATION_UNDERLINE, ...

// cursor
@property(readonly, nonatomic) NSCursor *cursor;
- (NSCursor*)cursorPassingURLs:(NSArray**)cursorURLs;

@end
