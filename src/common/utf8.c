/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* utf8.c: UTF-8 string functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#if defined(__OpenBSD__)
#include <utf8/wchar.h>
#else
#include <wchar.h>
#endif

#include "weechat.h"
#include "utf8.h"
#include "weeconfig.h"


int local_utf8 = 0;


/*
 * utf8_init: initializes UTF-8 in WeeChat
 */

void
utf8_init ()
{
    local_utf8 = 0;
    
    if (cfg_look_charset_internal && cfg_look_charset_internal[0])
    {
        if (strstr (cfg_look_charset_internal, "UTF-8")
            || strstr (cfg_look_charset_internal, "utf-8"))
            local_utf8 = 1;
    }
    else if ((local_charset)
             && ((strstr (local_charset, "UTF-8")
                  || strstr (local_charset, "utf-8"))))
        local_utf8 = 1;
}

/*
 * utf8_is_valid: return 1 if UTF-8 string is valid, 0 otherwise
 */

int
utf8_is_valid (char *string)
{
    while (string && string[0])
    {
        /* UTF-8, 2 bytes, should be: 110vvvvv 10vvvvvv */
        if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
        {
            if (!string[1] || (((unsigned char)(string[1]) & 0xC0) != 0x80))
                return 0;
            string += 2;
        }
        /* UTF-8, 3 bytes, should be: 1110vvvv 10vvvvvv 10vvvvvv */
        else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
        {
            if (!string[1] || !string[2]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80))
                return 0;
            string += 3;
        }
        /* UTF-8, 4 bytes, should be: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
        else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
        {
            if (!string[1] || !string[2] || !string[3]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80)
                || (((unsigned char)(string[3]) & 0xC0) != 0x80))
                return 0;
            string += 4;
        }
        /* UTF-8, 1 byte, should be: 0vvvvvvv */
        else if ((unsigned char)(string[0]) >= 0x80)
            return 0;
        else
            string++;
    }
    return 1;
}

/*
 * utf8_prev_char: return previous UTF-8 char in a string
 */

char *
utf8_prev_char (char *string_start, char *string)
{
    if (!string || (string <= string_start))
        return NULL;
    
    string--;
    
    if (!local_utf8)
        return string;
    
    if (((unsigned char)(string[0]) & 0xC0) == 0x80)
    {
        /* UTF-8, at least 2 bytes */
        string--;
        if (string < string_start)
            return string + 1;
        if (((unsigned char)(string[0]) & 0xC0) == 0x80)
        {
            /* UTF-8, at least 3 bytes */
            string--;
            if (string < string_start)
                return string + 1;
            if (((unsigned char)(string[0]) & 0xC0) == 0x80)
            {
                /* UTF-8, 4 bytes */
                string--;
                if (string < string_start)
                    return string + 1;
                return string;
            }
            else
                return string;
        }
        else
            return string;
    }
    return string;
}

/*
 * utf8_next_char: return next UTF-8 char in a string
 */

char *
utf8_next_char (char *string)
{
    if (!string)
        return NULL;
    
    if (!local_utf8)
        return string + 1;
    
    /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
    if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
    {
        if (!string[1])
            return string + 1;
        return string + 2;
    }
    /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
    {
        if (!string[1])
            return string + 1;
        if (!string[2])
            return string + 2;
        return string + 3;
    }
    /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
    {
        if (!string[1])
            return string + 1;
        if (!string[2])
            return string + 2;
        if (!string[3])
            return string + 3;
        return string + 4;
    }
    /* UTF-8, 1 byte: 0vvvvvvv */
    return string + 1;
}

/*
 * utf8_char_size: return UTF-8 char size
 */

int
utf8_char_size (char *string)
{
    if (!string)
        return 0;
    
    return utf8_next_char (string) - string;
}

/*
 * utf8_strlen: return length of an UTF-8 string (<= strlen(string))
 */

int
utf8_strlen (char *string)
{
    int length;
    
    if (!string)
        return 0;
    
    if (!local_utf8)
        return strlen (string);
    
    length = 0;
    while (string && string[0])
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * utf8_strnlen: return length of an UTF-8 string, for N bytes
 */

int
utf8_strnlen (char *string, int bytes)
{
    char *start;
    int length;
    
    if (!string)
        return 0;
    
    if (!local_utf8)
    {
        length = strlen (string);
        if (bytes > length)
            return length;
        return bytes;
    }
    
    start = string;
    length = 0;
    while (string && string[0] && (string - start < bytes))
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * utf8_width_screen: return number of chars needed on screen to display UTF-8 string
 */

int
utf8_width_screen (char *string)
{
    int length, num_char;
    wchar_t *wstring;
    
    if (!string)
        return 0;
    
    if (!local_utf8)
        return strlen (string);
    
    num_char = mbstowcs (NULL, string, 0) + 1;
    wstring = (wchar_t *) malloc ((num_char + 1) * sizeof (wchar_t));
    if (!wstring)
        return utf8_strlen (string);
    
    if (mbstowcs (wstring, string, num_char) == (size_t)(-1))
    {
        free (wstring);
        return utf8_strlen (string);
    }
    
    length = wcswidth (wstring, num_char);
    free (wstring);
    return length;
}

/*
 * utf8_add_offset: moves forward N chars in an UTF-8 string
 */

char *
utf8_add_offset (char *string, int offset)
{
    int count;

    if (!string)
        return string;
    
    if (!local_utf8)
        return string + offset;
    
    count = 0;
    while (string && string[0] && (count < offset))
    {
        string = utf8_next_char (string);
        count++;
    }
    return string;
}

/*
 * utf8_real_pos: get real position in UTF-8
 *                for example: ("a�bc", 2) returns 3
 */

int
utf8_real_pos (char *string, int pos)
{
    int count, real_pos;
    char *next_char;
    
    if (!string || !local_utf8)
        return pos;
    
    count = 0;
    real_pos = 0;
    while (string && string[0] && (count < pos))
    {
        next_char = utf8_next_char (string);
        real_pos += (next_char - string);
        string = next_char;
        count++;
    }
    return real_pos;
}

/*
 * utf8_pos: get position in UTF-8
 *           for example: ("a�bc", 3) returns 2
 */

int
utf8_pos (char *string, int real_pos)
{
    int count;
    char *limit;
    
    if (!string || !local_charset)
        return real_pos;
    
    count = 0;
    limit = string + real_pos;
    while (string && string[0] && (string < limit))
    {
        string = utf8_next_char (string);
        count++;
    }
    return count;
}
