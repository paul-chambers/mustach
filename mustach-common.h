/*
 Author: José Bollo <jobol@nonadev.net>

 https://gitlab.com/jobol/mustach

 SPDX-License-Identifier: ISC
*/

#ifndef _mustach_common_h_included_
#define _mustach_common_h_included_

/**
 * Current version of mustach
 *
 * This number is Mmmrr where:
 *    - M is the major number,
 *    - mm is the minor number,
 *    - rr is the revision number
 */
#define MUSTACH_VERSION    10300  /* version 1.3.0 */

/**
 * Maximum nested imbrications supported
 */
#define MUSTACH_MAX_DEPTH  256

/**
 * Maximum length of tags in mustaches {{...}}
 */
#define MUSTACH_MAX_LENGTH 4096

/**
 * Maximum length of delimitors (2 normally but extended here)
 */
#define MUSTACH_MAX_DELIM_LENGTH 8

/*
 * Definition of error codes returned by mustach
 */
#define MUSTACH_OK                       0
#define MUSTACH_ERROR_SYSTEM            -1
#define MUSTACH_ERROR_UNEXPECTED_END    -2
#define MUSTACH_ERROR_EMPTY_TAG         -3
#define MUSTACH_ERROR_TAG_TOO_LONG      -4
#define MUSTACH_ERROR_BAD_SEPARATORS    -5
#define MUSTACH_ERROR_TOO_DEEP          -6
#define MUSTACH_ERROR_CLOSING           -7
#define MUSTACH_ERROR_BAD_UNESCAPE_TAG  -8
#define MUSTACH_ERROR_INVALID_ITF       -9
#define MUSTACH_ERROR_ITEM_NOT_FOUND    -10
#define MUSTACH_ERROR_PARTIAL_NOT_FOUND -11
#define MUSTACH_ERROR_UNDEFINED_TAG     -12

/*
 * You can use definition below for user specific error
 *
 * The macro MUSTACH_ERROR_USER is involutive so for any value
 *   value = MUSTACH_ERROR_USER(MUSTACH_ERROR_USER(value))
 */
#define MUSTACH_ERROR_USER_BASE         -100
#define MUSTACH_ERROR_USER(x)           (MUSTACH_ERROR_USER_BASE-(x))
#define MUSTACH_IS_ERROR_USER(x)        (MUSTACH_ERROR_USER(x) >= 0)

/**
 * Extension flags
 */
#define Mustach_With_Colon                1     /* core */
#define Mustach_With_EmptyTag             2     /* core */
#define Mustach_With_SingleDot            4     /* obsolete, always set */
#define Mustach_With_Equal                8     /* wrap */
#define Mustach_With_Compare             16     /* wrap */
#define Mustach_With_JsonPointer         32     /* wrap */
#define Mustach_With_ObjectIter          64     /* wrap */
#define Mustach_With_IncPartial         128     /* obsolete, always set */
#define Mustach_With_EscFirstCmp        256     /* wrap */
#define Mustach_With_PartialDataFirst   512     /* wrap */
#define Mustach_With_ErrorUndefined    1024     /* wrap */

#define Mustach_With_NoExtensions         0
#define Mustach_With_AllExtensions     1023     /* don't include ErrorUndefined */

/**
 * mustach_sbuf - Interface for handling zero terminated strings
 *
 * That structure is used for returning zero terminated strings -in 'value'-
 * to mustach. The callee can provide a function for releasing the returned
 * 'value'. Three methods for releasing the string are possible.
 *
 *  1. no release: set either 'freecb' or 'releasecb' with NULL (done by default)
 *  2. release without closure: set 'freecb' to its expected value
 *  3. release with closure: set 'releasecb' and 'closure' to their expected values
 *
 * @value: The value of the string. That value is not changed by mustach -const-.
 *
 * @freecb: The function to call for freeing the value without closure.
 *          For convenience, signature of that callback is compatible with 'free'.
 *          Can be NULL.
 *
 * @releasecb: The function to release with closure.
 *             Can be NULL.
 *
 * @closure: The closure to use for 'releasecb'.
 *
 * @length: Length of the value or zero if unknown and value null terminated.
 *          To return the empty string, let it to zero and let value to NULL.
 */
struct mustach_sbuf {
	const char *value;
	union {
		void (*freecb)(void*);
		void (*releasecb)(const char *value, void *closure);
	};
	void *closure;
	size_t length;
};

/*
 * Definition of the writing callbacks for mustach functions
 * producing output to callbacks.
 *
 * Two callback types are defined:
 *
 * @mustach_write_cb_t:
 *
 *    callback receiving the escaped data to be written as 3 parameters:
 *
 *    1. the 'closure', the same given to the wmustach_... function
 *    2. a pointer to a 'buffer' containing the characters to be written
 *    3. the size in bytes of the data pointed by 'buffer'
 *
 * @mustach_emit_cb_t:
 *
 *    callback receiving the data to be written and a flag indicating
 *    if escaping should be done or not as 4 parameters:
 *
 *    1. the 'closure', the same given to the emustach_... function
 *    2. a pointer to a 'buffer' containing the characters to be written
 *    3. the size in bytes of the data pointed by 'buffer'
 *    4. a boolean indicating if 'escape' should be done
 */
typedef int mustach_write_cb_t(void *closure, const char *buffer, size_t size);
typedef int mustach_emit_cb_t(void *closure, const char *buffer, size_t size, int escape);

/***************************************************************************
* compatibility with version before 1.0
*/
#if defined(WITHOUT_DEPRECATED_MUSTACH)
#   define DEPRECATED_MUSTACH(func)
#   define DEPRECATED_MUSTACH_TYPE(type)
#else
#   define DEPRECATED_MUSTACH_TYPE(type) type
#   if defined(__GNUC__)
#      define DEPRECATED_MUSTACH(func) func __attribute__ ((deprecated))
#   elif defined(_MSC_VER)
#      define DEPRECATED_MUSTACH(func) __declspec(deprecated) func
#   elif !defined(DEPRECATED_MUSTACH)
#      pragma message("WARNING: You need to implement DEPRECATED_MUSTACH for this compiler")
#      define DEPRECATED_MUSTACH(func) func
#   endif
#endif

#endif

