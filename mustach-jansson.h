/*
 Author: José Bollo <jobol@nonadev.net>

 https://gitlab.com/jobol/mustach

 SPDX-License-Identifier: ISC
*/

#ifndef _mustach_jansson_h_included_
#define _mustach_jansson_h_included_

/*
 * mustach-jansson is intended to make integration of jansson
 * library by providing integrated functions.
 */

#include <jansson.h>
#include "mustach-wrap.h"

/**
 * mustach_jansson_file - Renders the mustache 'template' in 'file' for 'root'.
 *
 * @template: the template string to instanciate
 * @root:     the root json object to render
 * @file:     the file where to write the result
 *
 * Returns 0 in case of success, -1 with errno set in case of system error
 * a other negative value in case of error.
 */
extern int mustach_jansson_file(const char *template, json_t *root, int flags, FILE *file);

/**
 * mustach_jansson_fd - Renders the mustache 'template' in 'fd' for 'root'.
 *
 * @template: the template string to instanciate
 * @root:     the root json object to render
 * @fd:       the file descriptor number where to write the result
 *
 * Returns 0 in case of success, -1 with errno set in case of system error
 * a other negative value in case of error.
 */
extern int mustach_jansson_fd(const char *template, json_t *root, int flags, int fd);


/**
 * mustach_jansson_mem - Renders the mustache 'template' in 'result' for 'root'.
 *
 * @template: the template string to instanciate
 * @root:     the root json object to render
 * @result:   the pointer receiving the result when 0 is returned
 * @size:     the size of the returned result
 *
 * Returns 0 in case of success, -1 with errno set in case of system error
 * a other negative value in case of error.
 */
extern int mustach_jansson_mem(const char *template, json_t *root, int flags, char **result, size_t *size);

/**
 * mustach_jansson_write - Renders the mustache 'template' for 'root' to custom writer 'writecb' with 'closure'.
 *
 * @template: the template string to instanciate
 * @root:     the root json object to render
 * @writecb:  the function that write values
 * @closure:  the closure for the write function
 *
 * Returns 0 in case of success, -1 with errno set in case of system error
 * a other negative value in case of error.
 */
extern int mustach_jansson_write(const char *template, json_t *root, int flags, mustach_write_cb_t *writecb, void *closure);

/**
 * mustach_jansson_emit - Renders the mustache 'template' for 'root' to custom emiter 'emitcb' with 'closure'.
 *
 * @template: the template string to instanciate
 * @root:     the root json object to render
 * @emitcb:   the function that emit values
 * @closure:  the closure for the write function
 *
 * Returns 0 in case of success, -1 with errno set in case of system error
 * a other negative value in case of error.
 */
extern int mustach_jansson_emit(const char *template, json_t *root, int flags, mustach_emit_cb_t *emitcb, void *closure);

#endif

