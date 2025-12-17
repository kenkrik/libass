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

    if (line_id < 0 || line_id >= track->n_events)
        return NULL;

    ASS_Event *target_event = &track->events[line_id];
    
    // Find the clean text from char_boxes storage
    CharacterBoxStorage *storage = &render->char_boxes;
    
    for (size_t i = 0; i < storage->count; i++) {
        if (storage->boxes[i].event == target_event && storage->boxes[i].clean_text) {
            // Found it! Return a copy
            return strdup(storage->boxes[i].clean_text);
        }
    }
    
    // Fallback: event not rendered or no boxes
    return NULL;
}