/*
 * Default "no-op" handler which can be used as a base for creating user
 * handlers.
 */
#ifndef CSS_BASE_SELECT_HANDLER_H_
#define CSS_BASE_SELECT_HANDLER_H_

/// A handler which does not match anything
extern const css_select_handler CSSSelectHandlerBase;

/// Initialize |css_select_handler *handler| to a copy of |CSSSelectHandlerBase|
#define CSSSelectHandlerInitToBase(handler) \
  memcpy((handler), &CSSSelectHandlerBase, sizeof(CSSSelectHandlerBase))


#endif  // CSS_BASE_SELECT_HANDLER_H_
