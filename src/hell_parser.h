/*  hell_parser - v0.0.1 - MIT LICENSE
 *
 *  This is header only stb style inspired library ;
 *  It was created for my needs, use at your own risk.
 *
 *  ~~ How To compile code with this library ~~
 *
 *  Do this:
 *     #define HELL_PARSER_IMPLEMENTATION
 *  before you include this file in *one* C or C++ file to create the implementation.
 *
 *  #define HELL_PARSER_IMPLEMENTATION
 *  #include "hellparser.h"
 *
 *  You can #define HELL_MALLOC, STBI_REALLOC, and STBI_FREE to avoid using malloc,realloc,free
 *
 *
 *  Example code:
 *
 *    #include <stdlib.h>
 *    #include <stdio.h>
 *    
 *    #define HELL_PARSER_IMPLEMENTATION
 *    #include "../../hell_parser.h"
 *    
 *    int main()
 *    {
 *        char delim = '%';
 *        char *input_text = "background=%%color0.hex%%\n \
 *                            foreground=%%color15.hex%%";
 *    
 *        // Initialize parser with input
 *        hell_parser_t *parser = hell_parser_create(input_text);
 *        char *buffer = NULL;
 *    
 *        while (!hell_parser_eof(parser))
 *        {
 *            char ch;
 *    
 *            if (hell_parser_next(parser, &ch) == HELL_PARSER_OK)
 *            {
 *                if (ch == delim)
 *                {
 *                    parser->pos -= 1; // We have to do that so hell_parser_delim_buffer_between
 *                                      // keeps track of the position
 *
 *                    // Look out for delim, which is '%',
 *                    // count is set to 2, so it will trigger when it finds "%%"
 *                    if (hell_parser_delim_buffer_between(parser, delim, 2, &buffer) == HELL_PARSER_OK)
 *                    {
 *                        printf("Extracted content: '%s'\n", buffer);
 *                        free(buffer);
 *                    }
 *                }
 *            }
 *        }
 *    
 *        hell_parser_destroy(parser);
 *        return 0;
 *    }
*/

#ifndef HELL_PARSER_H
#define HELL_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version information */
#define HELL_PARSER_VERSION "0.0.1"

/* Configurable Options */
#ifndef HELL_DEF
#define HELL_DEF static inline
#else
#define HELL_DEF extern
#endif

/* Standard library dependencies */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/* Define your own malloc etc. */
#if defined(HELL_MALLOC) && defined(HELL_FREE) && (defined(HELL_REALLOC) || defined(HELL_REALLOC_SIZED))
// ok
#elif !defined(HELL_MALLOC) && !defined(HELL_FREE) && !defined(HELL_REALLOC) && !defined(HELL_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of HELL_MALLOC, HELL_FREE, and HELL_REALLOC (or HELL_REALLOC_SIZED)."
#endif

#ifndef HELL_MALLOC
#define HELL_MALLOC(sz)           malloc(sz)
#define HELL_REALLOC(p,newsz)     realloc(p,newsz)
#define HELL_FREE(p)              free(p)
#endif

#ifndef HELL_REALLOC_SIZED
#define HELL_REALLOC_SIZED(p,oldsz,newsz) HELL_REALLOC(p,newsz)
#endif

/* Library Types and Structures */
typedef enum
{
    HELL_PARSER_OK = 0,
    HELL_PARSER_ERROR = -1
} hell_parser_status_t;

typedef struct
{
    const char *input;   /* Input string to parse */
    size_t pos;          /* Current position in the input */
    size_t length;       /* Length of the input string */
} hell_parser_t;

/* Function Declarations */
HELL_DEF void hell_parser_destroy(hell_parser_t *parser);
HELL_DEF int hell_parser_eof(const hell_parser_t *parser);
HELL_DEF hell_parser_t *hell_parser_create(const char *input);
HELL_DEF hell_parser_status_t hell_parser_next(hell_parser_t *parser, char *out);
HELL_DEF hell_parser_status_t hell_parser_delim(hell_parser_t *parser, char delim, unsigned count);
HELL_DEF hell_parser_status_t hell_parser_delim_buffer_between(hell_parser_t *parser, char delim, unsigned count, char **buffer);

/* Implementation */
#ifdef HELL_PARSER_IMPLEMENTATION

HELL_DEF hell_parser_t *hell_parser_create(const char *input)
{
    if (!input) return NULL;

    hell_parser_t *parser = (hell_parser_t *)malloc(sizeof(hell_parser_t));
    if (!parser) return NULL;

    parser->input = input;
    parser->pos = 0;
    parser->length = strlen(input);

    return parser;
}

HELL_DEF void hell_parser_destroy(hell_parser_t *parser)
{
    if (parser)
    {
        free(parser);
    }
}

/* Go next by one character, and pass it to *out,
 * if it's out of bounds return error
 */
HELL_DEF hell_parser_status_t hell_parser_next(hell_parser_t *parser, char *out)
{
    if (!parser || !out || parser->pos >= parser->length)
    {
        return HELL_PARSER_ERROR;
    }
    *out = parser->input[parser->pos++];
    return HELL_PARSER_OK;
}

HELL_DEF hell_parser_status_t hell_parser_delim(hell_parser_t *parser, char delim, unsigned count)
{
    if (!parser)
        return HELL_PARSER_ERROR;

    count = (count == 0) ? 1 : count;  /* Default count to 1 if 0                      */
    size_t matched = 0;                /* Tracks consecutive delimiter matches         */

    while (!hell_parser_eof(parser))
    {
        char current;
        if (hell_parser_next(parser, &current) != HELL_PARSER_OK)
            break;

        if (current == delim)
        {
            matched++;
            if (matched == count)
                return HELL_PARSER_OK;
        }
        else
            matched = 0;
    }
    return HELL_PARSER_ERROR;
}

/* Get content inside delim [count] times, if count is 0 it defaults to 1.
 * For example, for '%' with count 2, it triggers on "%%" in the text.
 * Dynamically allocates memory for the content between delimiters and stores it in `*buffer`.
 * CALLER MUST FREE THE ALLOCATED MEMORY.
 */
HELL_DEF hell_parser_status_t hell_parser_delim_buffer_between(hell_parser_t *parser, char delim, unsigned count, char **buffer)
{
    if (!parser || !buffer)
        return HELL_PARSER_ERROR;

    count = (count == 0) ? 1 : count;  /* Default count to 1 if 0                      */
    size_t matched = 0;                /* Tracks consecutive delimiter matches         */
    size_t capacity = 64;              /* Initial buffer capacity                      */
    size_t buffer_pos = 0;             /* Tracks position in the buffer                */
    int inside = 0;                    /* Flag to determine if we're inside delimiters */

    *buffer = (char *)malloc(capacity);
    if (!*buffer) {
        return HELL_PARSER_ERROR;
    }

    while (!hell_parser_eof(parser))
    {
        char current;
        if (hell_parser_next(parser, &current) != HELL_PARSER_OK)
            break;

        if (current == delim)
        {
            matched++;
            if (matched == count)
            {
                if (inside)
                {
                    (*buffer)[buffer_pos] = '\0';
                    return HELL_PARSER_OK;
                }
                else
                {
                    inside = 1;
                    buffer_pos = 0;
                    matched = 0;
                }
            }
        }
        else
        {
            if (inside)
            {
                int last_matched_unmatched = 0;
                if (matched > 0)
                    last_matched_unmatched = matched;

                if (buffer_pos + 1 + last_matched_unmatched >= capacity)
                {
                    capacity *= 2;
                    char *new_buffer = (char *)realloc(*buffer, capacity);
                    if (!new_buffer)
                    {
                        free(*buffer);
                        return HELL_PARSER_ERROR;
                    }
                    *buffer = new_buffer;
                }
                
                while (last_matched_unmatched > 0)
                {
                    last_matched_unmatched--;
                    (*buffer)[buffer_pos++] = delim;
                }
                (*buffer)[buffer_pos++] = current;
                
            }
            matched = 0;
        }
    }

    free(*buffer);
    *buffer = NULL;
    return HELL_PARSER_ERROR;
}

HELL_DEF int hell_parser_eof(const hell_parser_t *parser)
{
    return (parser && parser->pos >= parser->length);
}

#endif /* HELL_PARSER_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* HELL_PARSER_H */
