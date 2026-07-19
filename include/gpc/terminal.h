// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TERMINAL_INCLUDED
#define GP_TERMINAL_INCLUDED 1

#include <gpc/preprocessor.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup terminal Terminal
/// @code
/// #include <gpc/terminal.h>
/// @endcode
/// ANSI escape sequences. Most commonly used to control terminal output color,
/// but can also control things like terminal cursor movement. These are just
/// strings that change the behavior of the terminal printed to terminal output.
///
/// The majority of this module is just macro constants that expand to string
/// literals containing the ANSI escape sequence. Here is an example of simple
/// usage of these constants:
/// @code
///     printf(
///         GP_ANSI_RED                    "Printing in red! "
///         GP_ANSI_WHITE_BG GP_ANSI_BLACK "Printing in black with white background! "
///         GP_ANSI_RESET                  "Remember to reset to default color!\n");
/// @endcode
/// The example relies on [C string literal concatenation](https://learn.microsoft.com/en-us/cpp/c-language/string-literal-concatenation?view=msvc-170).
/// It is simple and works fine for applications that know that the standard
/// output always refers to a terminal that supports ANSI escape sequences.
/// However, applications rarely know that the standard output is always
/// referring to a terminal. Often standard output might be redirected to a file
/// or piped to another application. In such case, the escape sequences are
/// printed as is to the output, which results in garbled output. Since the
/// standard output type cannot be known at compile time, it is recommended to
/// use @ref gp_ansi_enable() and @ref GP_ANSI macro to only conditionally
/// print the escape sequences when they are supported like so:
/// @code
///    bool got_color = gp_ansi_enable(GP_STDOUT, true) && !getenv("NO_COLOR");
///    printf(
///        "%sPrinting in red! "
///        "%s%sPrinting in black with white background! "
///        "%sRemember to reset to default color!\n",
///        GP_ANSI(RED, got_color),
///        GP_ANSI(WHITE_BG, got_color), GP_ANSI(BLACK, got_color),
///        GP_ANSI(RESET, got_color));
/// @endcode
/// The check for `!getenv("NO_COLOR")` is optional, but highly recommended, see
/// [NO_COLOR](https://no-color.org/).
///
///
/// @{

#define GP_STDIN  0 ///< File descriptor of standard input.
#define GP_STDOUT 1 ///< File descriptor of standard output.
#define GP_STDERR 2 ///< File descriptor of standard error.

/** Enable or disable ANSI escape sequences for given file descriptor.
 *
 * Enable escape sequences for given file descriptor @a fd by setting
 * @a enable to `true`, disable for @a fd by setting @a enable to `false`.
 *
 * Enabling escape sequences does a few things:
 * - Check if terminal is dumb by checking `TERM` environment variable.
 * - Check if @a fd refers to a terminal (`isatty()`).
 * - Enable colors on terminals that by default do not support colors (Windows
 *   Console Host mainly).
 * - Sets a flag for the given file descriptor to indicate that ANSI escape
 *   sequences have been enabled for the given file descriptor. The flag will be
 *   returned and can be retrieved later with @ref gp_ansi_is_enabled().
 * This can fail for multiple reasons, most notably standard output or error
 * might be redirected to a file or other application. Also the check is somewhat
 * conservative for improved portability (there is no standard way of detecting
 * ANSI support), so false negatives may occur.
 *
 * It is somewhat common to only redirect standard output or standard error, so
 * you should call this function for both, @ref GP_STDOUT and @ref GP_STDERR.
 *
 * Since some terminals don't support colors by default and outputs can be
 * redirected, ANSI colors are disabled for each file descriptor by default.
 * This simply means that @ref gp_ansi_is_enabled() will return `false` for any
 * file descriptor that has not been enabled by this function even if the output
 * would support ANSI escape sequences.
 *
 * This function will not check for [NO_COLOR](https://no-color.org/). The
 * reason is that `NO_COLOR` is not supposed to disable all ANSI escape
 * sequences (like cursor movement), just colors. Therefore, a good practice
 * is to additionally check `!getenv("NO_COLOR")`.
 *
 * @return `true` if ANSI escape sequences have been successfully enabled for
 * @a fd, `false` if enabling failed, in which case `errno` will be set, and
 * `false` if escape sequences were disabled for @a fd by setting @a enable to
 * `false`.
 */
GP_API bool gp_ansi_enable(int fd, bool enable);

/** Get cached result of @ref gp_ansi_enable(). */
GP_API bool gp_ansi_is_enabled(int fd);

/** Conditional ANSI macro string.
 *
 * Conditional ANSI string or an empty string depending on @a IS_ENABLED.
 * @a IS_ENABLED is supposed to be the boolean returned by @ref gp_ansi_enable()
 * or @a gp_ansi_is_enabled(), but of course can be any boolean value.
 *
 * @a ESCAPE_SEQUENCE should be the name of an `GP_ANSI_*` macro without the
 * `GP_ANSI_` prefix. For example, `GP_ANSI(RED, true)` returns @ref GP_ANSI_RED.
 *
 * @return ANSI escape sequence if @a IS_ENABLED is `true`, empty string
 * otherwise.
 */
#define/* const char* */GP_ANSI(ESCAPE_SEQUENCE,/* bool */IS_ENABLED) \
( \
    (IS_ENABLED) ? (GP_ANSI_##ESCAPE_SEQUENCE) : "" \
)

/** Conditional ANSI string.
 *
 * @ref GP_ANSI can only be used with the ANSI escape sequences that we have a
 * macro for. This function takes an arbitrary string instead of name of our
 * macros, which allows this to be used with escape sequences not defined by us.
 *
 * @a is_enabled is supposed to be the boolean returned by @ref gp_ansi_enable()
 * or @a gp_ansi_is_enabled(), but of course can be any boolean flag.
 *
 * @return @a escape_sequence if @a is_enabled is `true`, empty string otherwise.
 */
GP_INLINE const char* gp_ansi(const char* escape_sequence, bool is_enabled)
{
    return is_enabled ? escape_sequence : "";
}

/** Reset ANSI graphics to defaults.
 *
 * Printing graphic ANSI escape sequences changes the graphics (color usually)
 * for all subsequent printing. Therefore, one must print this after printing
 * the section with altered graphics to reset graphics to default.
 *
 * ### Example
 * @code
 *     if ((buf = malloc(BUF_SIZE)) == NULL) {
 *         printf("malloc() %sFAILED%s: %s\n,
 *             GP_ANSI_RED, GP_ANSI_RESET, strerror(errno));
 *     }
 * @endcode
 */
#define GP_ANSI_RESET               "\033[0m"

/** Swap foreground and background colors. */
#define GP_ANSI_INVERT_COLORS       "\033[7m"
/** Revert @ref GP_ANSI_INVERT_COLORS. */
#define GP_ANSI_NO_INVERTED_COLORS  "\033[27m"

// ----------------------------------------------------------------------------
/// @defgroup terminal_foreground_color Foreground Color
/// The exact RGB values of preset colors (all of these macros except @ref GP_ANSI_RGB)
/// are set by the terminal and can vary. The user often has control over the
/// preset colors in their terminal settings. Normal preset foreground colors
/// are always supported for any terminal that supports ANSI escape sequences.
/// The bright variants are practically always supported too.
/// @{

#define GP_ANSI_BLACK               "\033[30m" ///< ANSI escape sequence for black.
#define GP_ANSI_RED                 "\033[31m" ///< ANSI escape sequence for red.
#define GP_ANSI_GREEN               "\033[32m" ///< ANSI escape sequence for green.
#define GP_ANSI_YELLOW              "\033[33m" ///< ANSI escape sequence for yellow.
#define GP_ANSI_BLUE                "\033[34m" ///< ANSI escape sequence for blue.
#define GP_ANSI_MAGENTA             "\033[35m" ///< ANSI escape sequence for magenta.
#define GP_ANSI_CYAN                "\033[36m" ///< ANSI escape sequence for cyan.
#define GP_ANSI_WHITE               "\033[37m" ///< ANSI escape sequence for white.

#define GP_ANSI_BRIGHT_BLACK        "\033[90m" ///< ANSI escape sequence for dark gray.
#define GP_ANSI_BRIGHT_RED          "\033[91m" ///< ANSI escape sequence for bright red.
#define GP_ANSI_BRIGHT_GREEN        "\033[92m" ///< ANSI escape sequence for bright green.
#define GP_ANSI_BRIGHT_YELLOW       "\033[93m" ///< ANSI escape sequence for bright yellow.
#define GP_ANSI_BRIGHT_BLUE         "\033[94m" ///< ANSI escape sequence for bright blue.
#define GP_ANSI_BRIGHT_MAGENTA      "\033[95m" ///< ANSI escape sequence for bright magenta.
#define GP_ANSI_BRIGHT_CYAN         "\033[96m" ///< ANSI escape sequence for bright cyan.
#define GP_ANSI_BRIGHT_WHITE        "\033[97m" ///< ANSI escape sequence for bright white.

/** ANSI escape sequence for 24-bit RGB color (truecolor).
 *
 * A 24-bit color specified using 8-bit RGB color values. The @a R, @a G, and
 * @a B arguments must be decimal integer literals in range of 0 to 255 for all
 * normal usage. Example:
 * @code
 *     printf(GP_ANSI_RGB(red, green, blue)); // WRONG: runtime variables
 *     printf(GP_ANSI_RGB(0xF0, 0xC, 0xC)); // WRONG: hexadecimal
 *     printf(GP_ANSI_RGB(200, 12, 12)); // CORRECT: decimal integer literals
 * @endcode
 *
 * Runtime values can be created with `printf` family of functions like so:
 * @code
 *     printf(GP_ANSI_RGB(%i, %i, %i), red, green, blue);
 * @endcode
 *
 * Not all terminal support truecolor and some might require explicit
 * configuration, so maximum portability apps should use preset colors instead.
 * Some terminals set `COLORTERM` environment variable to some string that
 * includes "truecolor" or "24bit" to indicate truecolor. Example how you might
 * check for this:
 * @code
 *     char* colorterm = getenv(colorterm);
 *     bool has_truecolor = false;
 *     if (colorterm != NULL)
 *         has_truecolor = strstr(colorterm, "truecolor") != NULL
 *             || strstr(colorterm, "24bit") != NULL;
 * @endcode
 */
#define GP_ANSI_RGB(R, G, B)        "\033[38;2;" #R ";" #G ";" #B "m"

/// @}
// ----------------------------------------------------------------------------
/// @defgroup terminal_background_color Background Color
/// @{

#define GP_ANSI_BLACK_BG            "\033[40m" ///< ANSI escape sequence for black background.
#define GP_ANSI_RED_BG              "\033[41m" ///< ANSI escape sequence for red background.
#define GP_ANSI_GREEN_BG            "\033[42m" ///< ANSI escape sequence for green background.
#define GP_ANSI_YELLOW_BG           "\033[43m" ///< ANSI escape sequence for yellow background.
#define GP_ANSI_BLUE_BG             "\033[44m" ///< ANSI escape sequence for blue background.
#define GP_ANSI_MAGENTA_BG          "\033[45m" ///< ANSI escape sequence for magenta background.
#define GP_ANSI_CYAN_BG             "\033[46m" ///< ANSI escape sequence for cyan background.
#define GP_ANSI_WHITE_BG            "\033[47m" ///< ANSI escape sequence for white background.

#define GP_ANSI_BRIGHT_BLACK_BG     "\033[100m" ///< ANSI escape sequence for dark gray background.
#define GP_ANSI_BRIGHT_RED_BG       "\033[101m" ///< ANSI escape sequence for bright red background.
#define GP_ANSI_BRIGHT_GREEN_BG     "\033[102m" ///< ANSI escape sequence for bright green background.
#define GP_ANSI_BRIGHT_YELLOW_BG    "\033[103m" ///< ANSI escape sequence for bright yellow background.
#define GP_ANSI_BRIGHT_BLUE_BG      "\033[104m" ///< ANSI escape sequence for bright blue background.
#define GP_ANSI_BRIGHT_MAGENTA_BG   "\033[105m" ///< ANSI escape sequence for bright magenta background.
#define GP_ANSI_BRIGHT_CYAN_BG      "\033[106m" ///< ANSI escape sequence for bright cyan background.
#define GP_ANSI_BRIGHT_WHITE_BG     "\033[107m" ///< ANSI escape sequence for bright white background.

/** ANSI escape sequence for 24-bit RGB color (truecolor) background.
 *
 * Like @ref GP_ANSI_RGB but for background.
 */
#define GP_ANSI_RGB_BG(R, G, B)     "\033[48;2;" #R ";" #G ";" #B "m"

/// @}
// ----------------------------------------------------------------------------
/// @defgroup terminal_font Font
/// @{

/** Reset ANSI font to default. */
#define GP_ANSI_RESET_FONT          "\033[10m"

#define GP_ANSI_BOLD                "\033[1m"  ///< ANSI escape sequence for boldface.
#define GP_ANSI_FAINT               "\033[2m"  ///< ANSI escape sequence for faint.
#define GP_ANSI_NORMAL_INTENSITY    "\033[22m" ///< ANSI escape sequence for neither bold nor faint.
#define GP_ANSI_ITALIC              "\033[3m"  ///< ANSI escape sequence for italic. Rarely supported.
#define GP_ANSI_GOTHIC              "\033[20m" ///< ANSI escape sequence for Gothic. Rarely supported
#define GP_ANSI_NO_ITALIC           "\033[23m" ///< ANSI escape sequence for no italic. Also disables Gothic.
#define GP_ANSI_UNDERLINE           "\033[4m"  ///< ANSI escape sequence for underline.
#define GP_ANSI_DOUBLE_UNDERLINE    "\033[21m" ///< ANSI escape sequence for double underline. May disable bold instead.
#define GP_ANSI_NO_UNDERLINE        "\033[24m" ///< ANSI escape sequence for no underline. Also disables double underline
#define GP_ANSI_SLOW_BLINK          "\033[5m"  ///< ANSI escape sequence for slow blink.
#define GP_ANSI_FAST_BLINK          "\033[6m"  ///< ANSI escape sequence for fast blink. Rarely supported
#define GP_ANSI_HIDE                "\033[8m"  ///< ANSI escape sequence for hide. Rarely supported
#define GP_ANSI_REVEAL              "\033[28m" ///< ANSI escape sequence for unhide.
#define GP_ANSI_CROSSED_OUT         "\033[9m"  ///< ANSI escape sequence for crossed out.

/** Alternative font.
 *
 * Select alternative font. @a N must be a decimal integer literal in range of
 * 0 to 9 where 0 is default font. Runtime values can be created with `printf`
 * family of functions like so:
 * @code
 *     printf(GP_ANSI_FONT(%i), font_index);
 * @endcode
 */
#define GP_ANSI_FONT(N)             "\033[1" #N "m"

/// @}
// ----------------------------------------------------------------------------
/// @defgroup terminal_cursor_movement Cursor Movement
/// Like @ref GP_ANSI_RGB, the arguments to these must be an decimal integer
/// literals for most normal use.
/// @code
///     printf(GP_ANSI_CURSOR_UP(var)); // WRONG: variable
///     printf(GP_ANSI_CURSOR_UP(0xF)); // WRONG: hexadecimal
///     printf(GP_ANSI_CURSOR_UP(15));  // CORRECT: decimal
/// @endcode
///
/// Runtime values can be created with `printf` family of functions like so:
/// @code
///     printf(GP_ANSI_CURSOR_UP(%i), var);
/// @endcode
/// @{

/** ANSI escape sequence to move cursor @a N lines up.
 *
 * Cursor column stays fixed.
 */
#define GP_ANSI_CURSOR_UP(N)            "\033[" #N "A"

/** ANSI escape sequence to move cursor @a N lines down.
 *
 * Cursor column stays fixed.
 */
#define GP_ANSI_CURSOR_DOWN(N)          "\033[" #N "B"

/** ANSI escape sequence to move cursor @a N characters forward. */
#define GP_ANSI_CURSOR_FORWARD(N)       "\033[" #N "C"

/** ANSI escape sequence to move cursor @a N characters backward. */
#define GP_ANSI_CURSOR_BACK(N)          "\033[" #N "D"

/** ANSI escape sequence to move cursor @a N lines down.
 *
 * Cursor moves to the beginning of line.
 */
#define GP_ANSI_CURSOR_NEXT_LINE(N)     "\033[" #N "E"

/** ANSI escape sequence to move cursor @a N lines up.
 *
 * Cursor moves to the beginning of line.
 */
#define GP_ANSI_CURSOR_PREVIOUS_LINE(N) "\033[" #N "F"

/** ANSI escape sequence to move cursor to row @a N. */
#define GP_ANSI_CURSOR_ROW(N)           "\033[" #N "G"

/** ANSI escape sequence to move cursor to row @a N column @a M. */
#define GP_ANSI_CURSOR_POSITION(N, M)   "\033[" #N ";" #M "H"

/// @}
/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

///@endcond
#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_TERMINAL_INCLUDED
