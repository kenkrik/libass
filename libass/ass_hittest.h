/*
 * Copyright (C) 2025 libass contributors
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#ifndef LIBASS_HITTEST_H
#define LIBASS_HITTEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Point in 26.6 fixed-point format
 */
typedef struct ass_box_point {
    int32_t x;
    int32_t y;
} AssBoxPoint;

/**
 * \brief Structure representing a character fragment's bounding box
 * * This structure contains the geometry and text mapping information
 * for a single rendered character or text fragment.
 */
typedef struct ass_character_box {
    /** Bounding box x-coordinate in 26.6 fixed-point format */
    int32_t x;
    
    /** Bounding box y-coordinate in 26.6 fixed-point format */
    int32_t y;
    
    /** Bounding box width in 26.6 fixed-point format */
    int32_t w;
    
    /** Bounding box height in 26.6 fixed-point format */
    int32_t h;
    
    /** Rotated quad vertices in 26.6 fixed-point format (screen space) */
    AssBoxPoint top_left;
    AssBoxPoint top_right;
    AssBoxPoint bottom_left;
    AssBoxPoint bottom_right;

    /** Unique identifier for the source subtitle line */
    int line_id;
    
    /** Start index in unformatted dialogue text (inclusive) */
    int char_start_index;
    
    /** End index in unformatted dialogue text (exclusive) */
    int char_end_index;
} AssCharacterBox;

/**
 * \brief Get bounding boxes for all visible character fragments
 * * This function returns an array of bounding boxes for all visible
 * characters/fragments at the given timestamp. The boxes include
 * mapping information back to the source text.
 * * \param render The renderer instance
 * \param time_ms Current playback time in milliseconds
 * \param count_out Output pointer for the number of boxes returned
 * \return Dynamically allocated array of AssCharacterBox structures,
 * or NULL on failure. Must be freed by caller using ass_free().
 */
AssCharacterBox* ass_get_current_fragment_boxes(
    ASS_Renderer *render,
    int64_t time_ms,
    int *count_out
);

/**
 * \brief Get the plain text content of a subtitle line
 * * Returns the unformatted, tag-stripped text content of the subtitle
 * line identified by line_id. This text corresponds to the indices
 * provided in AssCharacterBox structures.
 * * \param render The renderer instance
 * \param line_id The line identifier from AssCharacterBox
 * \return Dynamically allocated UTF-8 string, or NULL if not found.
 * Must be freed by caller using ass_free().
 */
char* ass_get_dialogue_plaintext(
    ASS_Renderer *render,
    int line_id
);

#ifdef __cplusplus
}
#endif

#endif /* LIBASS_HITTEST_H */