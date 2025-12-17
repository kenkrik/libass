/*
 * Copyright (C) 2025 libass contributors
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "config.h"
#include "ass_compat.h"

#include <stdlib.h>
#include <string.h>

#include "ass.h"
#include "ass_hittest.h"
#include "ass_render.h"
#include "ass_utils.h"

/**
 * \brief Helper to strip ASS formatting tags from text
 * 
 * Creates a plain text version of ASS dialogue text by removing
 * all override tags like {\tag} and drawing commands.
 * 
 * \param text Source text with ASS formatting
 * \return Newly allocated plain text string, or NULL on error
 */
static char* strip_ass_tags(const char *text)
{
    if (!text)
        return NULL;
    
    size_t len = strlen(text);
    char *result = malloc(len + 1);
    if (!result)
        return NULL;
    
    const char *src = text;
    char *dst = result;
    int brace_level = 0;
    
    while (*src) {
        if (*src == '{') {
            brace_level++;
            src++;
        } else if (*src == '}') {
            if (brace_level > 0)
                brace_level--;
            src++;
        } else if (brace_level == 0) {
            // Outside of tags, copy character
            *dst++ = *src++;
        } else {
            // Inside tags, skip
            src++;
        }
    }
    
    *dst = '\0';
    return result;
}

/**
 * \brief Get UTF-8 byte length of a character
 */
static int utf8_char_length(const char *str)
{
    unsigned char c = (unsigned char)*str;
    
    if (c < 0x80)
        return 1;
    else if ((c & 0xE0) == 0xC0)
        return 2;
    else if ((c & 0xF0) == 0xE0)
        return 3;
    else if ((c & 0xF8) == 0xF0)
        return 4;
    else
        return 1; // Invalid, but don't crash
}

/**
 * \brief Convert character index to byte offset in UTF-8 string
 */
static int char_index_to_byte_offset(const char *str, int char_index)
{
    int byte_offset = 0;
    int char_count = 0;
    
    while (str[byte_offset] && char_count < char_index) {
        byte_offset += utf8_char_length(str + byte_offset);
        char_count++;
    }
    
    return byte_offset;
}

/**
 * Public API Implementation
 */

AssCharacterBox* ass_get_current_fragment_boxes(
    ASS_Renderer *render,
    int64_t time_ms,
    int *count_out)
{
    if (!render || !count_out) {
        if (count_out)
            *count_out = 0;
        return NULL;
    }
    
    // Ensure we have up-to-date rendering data
    int detect_change;
    ASS_Track *track = render->track;
    
    if (!track) {
        *count_out = 0;
        return NULL;
    }
    
    // Trigger rendering to populate char_boxes
    ass_render_frame(render, track, time_ms, &detect_change);
    
    CharacterBoxStorage *storage = &render->char_boxes;
    
    if (storage->count == 0) {
        *count_out = 0;
        return NULL;
    }
    
    // Allocate output array
    AssCharacterBox *boxes = ass_malloc(storage->count * sizeof(AssCharacterBox));
    if (!boxes) {
        *count_out = 0;
        return NULL;
    }
    
    // Convert internal format to public API format
    for (size_t i = 0; i < storage->count; i++) {
        CharacterBoxData *src = &storage->boxes[i];
        AssCharacterBox *dst = &boxes[i];
        
        // Convert bounding box (already in 26.6 format)
        dst->x = src->bbox.x_min;
        dst->y = src->bbox.y_min;
        dst->w = src->bbox.x_max - src->bbox.x_min;
        dst->h = src->bbox.y_max - src->bbox.y_min;

        // Copy rotated vertices
        dst->top_left = src->c1;
        dst->top_right = src->c2;
        dst->bottom_right = src->c3;
        dst->bottom_left = src->c4;
        
        // FIX: Calculate array index instead of casting pointer to int
        // Pointer arithmetic: (EventPtr - BasePtr) = Index
        if (src->event >= track->events && 
            src->event < (track->events + track->n_events)) {
            dst->line_id = (int)(src->event - track->events);
        } else {
            dst->line_id = -1; // Fallback for safety
        }
        
        // Text indices
        dst->char_start_index = src->text_start;
        dst->char_end_index = src->text_end;
    }
    
    *count_out = (int)storage->count;
    return boxes;
}

char* ass_get_dialogue_plaintext(
    ASS_Renderer *render,
    int line_id)
{
    if (!render || !render->track)
        return NULL;
    
    ASS_Track *track = render->track;

    // FIX: line_id is now the array index.
    // Bounds check the index directly.
    if (line_id < 0 || line_id >= track->n_events) {
        return NULL;
    }

    ASS_Event *target_event = &track->events[line_id];
    
    if (!target_event->Text)
        return NULL;
    
    // Strip ASS formatting tags and return plain text
    return strip_ass_tags(target_event->Text);
}