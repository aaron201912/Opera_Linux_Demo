/* Generated by wayland-scanner 1.17.0 */

#ifndef TEXT_CLIENT_PROTOCOL_H
#define TEXT_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_text The text protocol
 * @section page_ifaces_text Interfaces
 * - @subpage page_iface_wl_text_input - text input
 * - @subpage page_iface_wl_text_input_manager - text input manager
 * @section page_copyright_text Copyright
 * <pre>
 *
 * Copyright © 2012, 2013 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 * </pre>
 */
struct wl_seat;
struct wl_surface;
struct wl_text_input;
struct wl_text_input_manager;

/**
 * @page page_iface_wl_text_input wl_text_input
 * @section page_iface_wl_text_input_desc Description
 *
 * An object used for text input. Adds support for text input and input 
 * methods to applications. A text-input object is created from a
 * wl_text_input_manager and corresponds typically to a text entry in an
 * application.
 * Requests are used to activate/deactivate the text-input object and set
 * state information like surrounding and selected text or the content type.
 * The information about entered text is sent to the text-input object via
 * the pre-edit and commit events. Using this interface removes the need
 * for applications to directly process hardware key events and compose text
 * out of them.
 *
 * Text is generally UTF-8 encoded, indices and lengths are in bytes.
 *
 * Serials are used to synchronize the state between the text input and
 * an input method. New serials are sent by the text input in the
 * commit_state request and are used by the input method to indicate
 * the known text input state in events like preedit_string, commit_string,
 * and keysym. The text input can then ignore events from the input method
 * which are based on an outdated state (for example after a reset).
 * @section page_iface_wl_text_input_api API
 * See @ref iface_wl_text_input.
 */
/**
 * @defgroup iface_wl_text_input The wl_text_input interface
 *
 * An object used for text input. Adds support for text input and input 
 * methods to applications. A text-input object is created from a
 * wl_text_input_manager and corresponds typically to a text entry in an
 * application.
 * Requests are used to activate/deactivate the text-input object and set
 * state information like surrounding and selected text or the content type.
 * The information about entered text is sent to the text-input object via
 * the pre-edit and commit events. Using this interface removes the need
 * for applications to directly process hardware key events and compose text
 * out of them.
 *
 * Text is generally UTF-8 encoded, indices and lengths are in bytes.
 *
 * Serials are used to synchronize the state between the text input and
 * an input method. New serials are sent by the text input in the
 * commit_state request and are used by the input method to indicate
 * the known text input state in events like preedit_string, commit_string,
 * and keysym. The text input can then ignore events from the input method
 * which are based on an outdated state (for example after a reset).
 */
extern const struct wl_interface wl_text_input_interface;
/**
 * @page page_iface_wl_text_input_manager wl_text_input_manager
 * @section page_iface_wl_text_input_manager_desc Description
 *
 * A factory for text-input objects. This object is a global singleton.
 * @section page_iface_wl_text_input_manager_api API
 * See @ref iface_wl_text_input_manager.
 */
/**
 * @defgroup iface_wl_text_input_manager The wl_text_input_manager interface
 *
 * A factory for text-input objects. This object is a global singleton.
 */
extern const struct wl_interface wl_text_input_manager_interface;

#ifndef WL_TEXT_INPUT_CONTENT_HINT_ENUM
#define WL_TEXT_INPUT_CONTENT_HINT_ENUM
/**
 * @ingroup iface_wl_text_input
 * content hint
 *
 * Content hint is a bitmask to allow to modify the behavior of the text
 * input.
 */
enum wl_text_input_content_hint {
	/**
	 * no special behaviour
	 */
	WL_TEXT_INPUT_CONTENT_HINT_NONE = 0x0,
	/**
	 * auto completion, correction and capitalization
	 */
	WL_TEXT_INPUT_CONTENT_HINT_DEFAULT = 0x7,
	/**
	 * hidden and sensitive text
	 */
	WL_TEXT_INPUT_CONTENT_HINT_PASSWORD = 0xc0,
	/**
	 * suggest word completions
	 */
	WL_TEXT_INPUT_CONTENT_HINT_AUTO_COMPLETION = 0x1,
	/**
	 * suggest word corrections
	 */
	WL_TEXT_INPUT_CONTENT_HINT_AUTO_CORRECTION = 0x2,
	/**
	 * switch to uppercase letters at the start of a sentence
	 */
	WL_TEXT_INPUT_CONTENT_HINT_AUTO_CAPITALIZATION = 0x4,
	/**
	 * prefer lowercase letters
	 */
	WL_TEXT_INPUT_CONTENT_HINT_LOWERCASE = 0x8,
	/**
	 * prefer uppercase letters
	 */
	WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE = 0x10,
	/**
	 * prefer casing for titles and headings (can be language dependent)
	 */
	WL_TEXT_INPUT_CONTENT_HINT_TITLECASE = 0x20,
	/**
	 * characters should be hidden
	 */
	WL_TEXT_INPUT_CONTENT_HINT_HIDDEN_TEXT = 0x40,
	/**
	 * typed text should not be stored
	 */
	WL_TEXT_INPUT_CONTENT_HINT_SENSITIVE_DATA = 0x80,
	/**
	 * just latin characters should be entered
	 */
	WL_TEXT_INPUT_CONTENT_HINT_LATIN = 0x100,
	/**
	 * the text input is multiline
	 */
	WL_TEXT_INPUT_CONTENT_HINT_MULTILINE = 0x200,
};
#endif /* WL_TEXT_INPUT_CONTENT_HINT_ENUM */

#ifndef WL_TEXT_INPUT_CONTENT_PURPOSE_ENUM
#define WL_TEXT_INPUT_CONTENT_PURPOSE_ENUM
/**
 * @ingroup iface_wl_text_input
 * content purpose
 *
 * The content purpose allows to specify the primary purpose of a text
 * input.
 *
 * This allows an input method to show special purpose input panels with
 * extra characters or to disallow some characters.
 */
enum wl_text_input_content_purpose {
	/**
	 * default input, allowing all characters
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL = 0,
	/**
	 * allow only alphabetic characters
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_ALPHA = 1,
	/**
	 * allow only digits
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS = 2,
	/**
	 * input a number (including decimal separator and sign)
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_NUMBER = 3,
	/**
	 * input a phone number
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_PHONE = 4,
	/**
	 * input an URL
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_URL = 5,
	/**
	 * input an email address
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_EMAIL = 6,
	/**
	 * input a name of a person
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_NAME = 7,
	/**
	 * input a password (combine with password or sensitive_data hint)
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD = 8,
	/**
	 * input a date
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_DATE = 9,
	/**
	 * input a time
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_TIME = 10,
	/**
	 * input a date and time
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_DATETIME = 11,
	/**
	 * input for a terminal
	 */
	WL_TEXT_INPUT_CONTENT_PURPOSE_TERMINAL = 12,
};
#endif /* WL_TEXT_INPUT_CONTENT_PURPOSE_ENUM */

#ifndef WL_TEXT_INPUT_PREEDIT_STYLE_ENUM
#define WL_TEXT_INPUT_PREEDIT_STYLE_ENUM
enum wl_text_input_preedit_style {
	/**
	 * default style for composing text
	 */
	WL_TEXT_INPUT_PREEDIT_STYLE_DEFAULT = 0,
	/**
	 * style should be the same as in non-composing text
	 */
	WL_TEXT_INPUT_PREEDIT_STYLE_NONE = 1,
	WL_TEXT_INPUT_PREEDIT_STYLE_ACTIVE = 2,
	WL_TEXT_INPUT_PREEDIT_STYLE_INACTIVE = 3,
	WL_TEXT_INPUT_PREEDIT_STYLE_HIGHLIGHT = 4,
	WL_TEXT_INPUT_PREEDIT_STYLE_UNDERLINE = 5,
	WL_TEXT_INPUT_PREEDIT_STYLE_SELECTION = 6,
	WL_TEXT_INPUT_PREEDIT_STYLE_INCORRECT = 7,
};
#endif /* WL_TEXT_INPUT_PREEDIT_STYLE_ENUM */

#ifndef WL_TEXT_INPUT_TEXT_DIRECTION_ENUM
#define WL_TEXT_INPUT_TEXT_DIRECTION_ENUM
enum wl_text_input_text_direction {
	/**
	 * automatic text direction based on text and language
	 */
	WL_TEXT_INPUT_TEXT_DIRECTION_AUTO = 0,
	/**
	 * left-to-right
	 */
	WL_TEXT_INPUT_TEXT_DIRECTION_LTR = 1,
	/**
	 * right-to-left
	 */
	WL_TEXT_INPUT_TEXT_DIRECTION_RTL = 2,
};
#endif /* WL_TEXT_INPUT_TEXT_DIRECTION_ENUM */

/**
 * @ingroup iface_wl_text_input
 * @struct wl_text_input_listener
 */
struct wl_text_input_listener {
	/**
	 * enter event
	 *
	 * Notify the text-input object when it received focus. Typically
	 * in response to an activate request.
	 */
	void (*enter)(void *data,
		      struct wl_text_input *wl_text_input,
		      struct wl_surface *surface);
	/**
	 * leave event
	 *
	 * Notify the text-input object when it lost focus. Either in
	 * response to a deactivate request or when the assigned surface
	 * lost focus or was destroyed.
	 */
	void (*leave)(void *data,
		      struct wl_text_input *wl_text_input);
	/**
	 * modifiers map
	 *
	 * Transfer an array of 0-terminated modifiers names. The
	 * position in the array is the index of the modifier as used in
	 * the modifiers bitmask in the keysym event.
	 */
	void (*modifiers_map)(void *data,
			      struct wl_text_input *wl_text_input,
			      struct wl_array *map);
	/**
	 * state of the input panel
	 *
	 * Notify when the visibility state of the input panel changed.
	 */
	void (*input_panel_state)(void *data,
				  struct wl_text_input *wl_text_input,
				  uint32_t state);
	/**
	 * pre-edit
	 *
	 * Notify when a new composing text (pre-edit) should be set
	 * around the current cursor position. Any previously set composing
	 * text should be removed.
	 *
	 * The commit text can be used to replace the preedit text on reset
	 * (for example on unfocus).
	 *
	 * The text input should also handle all preedit_style and
	 * preedit_cursor events occuring directly before preedit_string.
	 * @param serial serial of the latest known text input state
	 */
	void (*preedit_string)(void *data,
			       struct wl_text_input *wl_text_input,
			       uint32_t serial,
			       const char *text,
			       const char *commit);
	/**
	 * pre-edit styling
	 *
	 * Sets styling information on composing text. The style is
	 * applied for length bytes from index relative to the beginning of
	 * the composing text (as byte offset). Multiple styles can be
	 * applied to a composing text by sending multiple preedit_styling
	 * events.
	 *
	 * This event is handled as part of a following preedit_string
	 * event.
	 */
	void (*preedit_styling)(void *data,
				struct wl_text_input *wl_text_input,
				uint32_t index,
				uint32_t length,
				uint32_t style);
	/**
	 * pre-edit cursor
	 *
	 * Sets the cursor position inside the composing text (as byte
	 * offset) relative to the start of the composing text. When index
	 * is a negative number no cursor is shown.
	 *
	 * This event is handled as part of a following preedit_string
	 * event.
	 */
	void (*preedit_cursor)(void *data,
			       struct wl_text_input *wl_text_input,
			       int32_t index);
	/**
	 * commit
	 *
	 * Notify when text should be inserted into the editor widget.
	 * The text to commit could be either just a single character after
	 * a key press or the result of some composing (pre-edit). It could
	 * be also an empty text when some text should be removed (see
	 * delete_surrounding_text) or when the input cursor should be
	 * moved (see cursor_position).
	 *
	 * Any previously set composing text should be removed.
	 * @param serial serial of the latest known text input state
	 */
	void (*commit_string)(void *data,
			      struct wl_text_input *wl_text_input,
			      uint32_t serial,
			      const char *text);
	/**
	 * set cursor to new position
	 *
	 * Notify when the cursor or anchor position should be modified.
	 *
	 * This event should be handled as part of a following
	 * commit_string event.
	 */
	void (*cursor_position)(void *data,
				struct wl_text_input *wl_text_input,
				int32_t index,
				int32_t anchor);
	/**
	 * delete surrounding text
	 *
	 * Notify when the text around the current cursor position should
	 * be deleted.
	 *
	 * Index is relative to the current cursor (in bytes). Length is
	 * the length of deleted text (in bytes).
	 *
	 * This event should be handled as part of a following
	 * commit_string event.
	 */
	void (*delete_surrounding_text)(void *data,
					struct wl_text_input *wl_text_input,
					int32_t index,
					uint32_t length);
	/**
	 * keysym
	 *
	 * Notify when a key event was sent. Key events should not be
	 * used for normal text input operations, which should be done with
	 * commit_string, delete_surrounding_text, etc. The key event
	 * follows the wl_keyboard key event convention. Sym is a XKB
	 * keysym, state a wl_keyboard key_state. Modifiers are a mask for
	 * effective modifiers (where the modifier indices are set by the
	 * modifiers_map event)
	 * @param serial serial of the latest known text input state
	 */
	void (*keysym)(void *data,
		       struct wl_text_input *wl_text_input,
		       uint32_t serial,
		       uint32_t time,
		       uint32_t sym,
		       uint32_t state,
		       uint32_t modifiers);
	/**
	 * language
	 *
	 * Sets the language of the input text. The "language" argument
	 * is a RFC-3066 format language tag.
	 * @param serial serial of the latest known text input state
	 */
	void (*language)(void *data,
			 struct wl_text_input *wl_text_input,
			 uint32_t serial,
			 const char *language);
	/**
	 * text direction
	 *
	 * Sets the text direction of input text.
	 *
	 * It is mainly needed for showing input cursor on correct side of
	 * the editor when there is no input yet done and making sure
	 * neutral direction text is laid out properly.
	 * @param serial serial of the latest known text input state
	 */
	void (*text_direction)(void *data,
			       struct wl_text_input *wl_text_input,
			       uint32_t serial,
			       uint32_t direction);
};

/**
 * @ingroup iface_wl_text_input
 */
static inline int
wl_text_input_add_listener(struct wl_text_input *wl_text_input,
			   const struct wl_text_input_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_text_input,
				     (void (**)(void)) listener, data);
}

#define WL_TEXT_INPUT_ACTIVATE 0
#define WL_TEXT_INPUT_DEACTIVATE 1
#define WL_TEXT_INPUT_SHOW_INPUT_PANEL 2
#define WL_TEXT_INPUT_HIDE_INPUT_PANEL 3
#define WL_TEXT_INPUT_RESET 4
#define WL_TEXT_INPUT_SET_SURROUNDING_TEXT 5
#define WL_TEXT_INPUT_SET_CONTENT_TYPE 6
#define WL_TEXT_INPUT_SET_CURSOR_RECTANGLE 7
#define WL_TEXT_INPUT_SET_PREFERRED_LANGUAGE 8
#define WL_TEXT_INPUT_COMMIT_STATE 9
#define WL_TEXT_INPUT_INVOKE_ACTION 10

/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_ENTER_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_LEAVE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_MODIFIERS_MAP_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_INPUT_PANEL_STATE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_PREEDIT_STRING_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_PREEDIT_STYLING_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_PREEDIT_CURSOR_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_COMMIT_STRING_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_CURSOR_POSITION_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_DELETE_SURROUNDING_TEXT_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_KEYSYM_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_LANGUAGE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_TEXT_DIRECTION_SINCE_VERSION 1

/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_ACTIVATE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_DEACTIVATE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_SHOW_INPUT_PANEL_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_HIDE_INPUT_PANEL_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_RESET_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_SET_SURROUNDING_TEXT_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_SET_CONTENT_TYPE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_SET_CURSOR_RECTANGLE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_SET_PREFERRED_LANGUAGE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_COMMIT_STATE_SINCE_VERSION 1
/**
 * @ingroup iface_wl_text_input
 */
#define WL_TEXT_INPUT_INVOKE_ACTION_SINCE_VERSION 1

/** @ingroup iface_wl_text_input */
static inline void
wl_text_input_set_user_data(struct wl_text_input *wl_text_input, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_text_input, user_data);
}

/** @ingroup iface_wl_text_input */
static inline void *
wl_text_input_get_user_data(struct wl_text_input *wl_text_input)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_text_input);
}

static inline uint32_t
wl_text_input_get_version(struct wl_text_input *wl_text_input)
{
	return wl_proxy_get_version((struct wl_proxy *) wl_text_input);
}

/** @ingroup iface_wl_text_input */
static inline void
wl_text_input_destroy(struct wl_text_input *wl_text_input)
{
	wl_proxy_destroy((struct wl_proxy *) wl_text_input);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Requests the text-input object to be activated (typically when the 
 * text entry gets focus).
 * The seat argument is a wl_seat which maintains the focus for this
 * activation. The surface argument is a wl_surface assigned to the
 * text-input object and tracked for focus lost. The enter event
 * is emitted on successful activation.
 */
static inline void
wl_text_input_activate(struct wl_text_input *wl_text_input, struct wl_seat *seat, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_ACTIVATE, seat, surface);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Requests the text-input object to be deactivated (typically when the
 * text entry lost focus). The seat argument is a wl_seat which was used
 * for activation.
 */
static inline void
wl_text_input_deactivate(struct wl_text_input *wl_text_input, struct wl_seat *seat)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_DEACTIVATE, seat);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Requests input panels (virtual keyboard) to show.
 */
static inline void
wl_text_input_show_input_panel(struct wl_text_input *wl_text_input)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_SHOW_INPUT_PANEL);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Requests input panels (virtual keyboard) to hide.
 */
static inline void
wl_text_input_hide_input_panel(struct wl_text_input *wl_text_input)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_HIDE_INPUT_PANEL);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Should be called by an editor widget when the input state should be
 * reset, for example after the text was changed outside of the normal
 * input method flow.
 */
static inline void
wl_text_input_reset(struct wl_text_input *wl_text_input)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_RESET);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Sets the plain surrounding text around the input position. Text is
 * UTF-8 encoded. Cursor is the byte offset within the
 * surrounding text. Anchor is the byte offset of the
 * selection anchor within the surrounding text. If there is no selected
 * text anchor is the same as cursor.
 */
static inline void
wl_text_input_set_surrounding_text(struct wl_text_input *wl_text_input, const char *text, uint32_t cursor, uint32_t anchor)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_SET_SURROUNDING_TEXT, text, cursor, anchor);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Sets the content purpose and content hint. While the purpose is the
 * basic purpose of an input field, the hint flags allow to modify some
 * of the behavior.
 *
 * When no content type is explicitly set, a normal content purpose with
 * default hints (auto completion, auto correction, auto capitalization)
 * should be assumed.
 */
static inline void
wl_text_input_set_content_type(struct wl_text_input *wl_text_input, uint32_t hint, uint32_t purpose)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_SET_CONTENT_TYPE, hint, purpose);
}

/**
 * @ingroup iface_wl_text_input
 */
static inline void
wl_text_input_set_cursor_rectangle(struct wl_text_input *wl_text_input, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_SET_CURSOR_RECTANGLE, x, y, width, height);
}

/**
 * @ingroup iface_wl_text_input
 *
 * Sets a specific language. This allows for example a virtual keyboard to
 * show a language specific layout. The "language" argument is a RFC-3066 
 * format language tag.
 *
 * It could be used for example in a word processor to indicate language of
 * currently edited document or in an instant message application which tracks
 * languages of contacts.
 */
static inline void
wl_text_input_set_preferred_language(struct wl_text_input *wl_text_input, const char *language)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_SET_PREFERRED_LANGUAGE, language);
}

/**
 * @ingroup iface_wl_text_input
 */
static inline void
wl_text_input_commit_state(struct wl_text_input *wl_text_input, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_COMMIT_STATE, serial);
}

/**
 * @ingroup iface_wl_text_input
 */
static inline void
wl_text_input_invoke_action(struct wl_text_input *wl_text_input, uint32_t button, uint32_t index)
{
	wl_proxy_marshal((struct wl_proxy *) wl_text_input,
			 WL_TEXT_INPUT_INVOKE_ACTION, button, index);
}

#define WL_TEXT_INPUT_MANAGER_CREATE_TEXT_INPUT 0


/**
 * @ingroup iface_wl_text_input_manager
 */
#define WL_TEXT_INPUT_MANAGER_CREATE_TEXT_INPUT_SINCE_VERSION 1

/** @ingroup iface_wl_text_input_manager */
static inline void
wl_text_input_manager_set_user_data(struct wl_text_input_manager *wl_text_input_manager, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_text_input_manager, user_data);
}

/** @ingroup iface_wl_text_input_manager */
static inline void *
wl_text_input_manager_get_user_data(struct wl_text_input_manager *wl_text_input_manager)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_text_input_manager);
}

static inline uint32_t
wl_text_input_manager_get_version(struct wl_text_input_manager *wl_text_input_manager)
{
	return wl_proxy_get_version((struct wl_proxy *) wl_text_input_manager);
}

/** @ingroup iface_wl_text_input_manager */
static inline void
wl_text_input_manager_destroy(struct wl_text_input_manager *wl_text_input_manager)
{
	wl_proxy_destroy((struct wl_proxy *) wl_text_input_manager);
}

/**
 * @ingroup iface_wl_text_input_manager
 *
 * Creates a new text-input object.
 */
static inline struct wl_text_input *
wl_text_input_manager_create_text_input(struct wl_text_input_manager *wl_text_input_manager)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_text_input_manager,
			 WL_TEXT_INPUT_MANAGER_CREATE_TEXT_INPUT, &wl_text_input_interface, NULL);

	return (struct wl_text_input *) id;
}

#ifdef  __cplusplus
}
#endif

#endif
