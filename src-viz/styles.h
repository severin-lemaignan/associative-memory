/*
    Copyright (c) 2010 Séverin Lemaignan (slemaign@laas.fr)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STYLES_H
#define STYLES_H

#include "core/vectors.h"

enum rendering_mode {NORMAL, SIMPLE, SHADOWS, BLOOM, NAMES, GRAPHVIZ};

static const float NODE_SIZE = 15.0;
static const int BASE_FONT_SIZE = 14;
static const int MEDIUM_FONT_SIZE = 16;
static const int LARGE_FONT_SIZE = 42;
static const float ARROW_SIZE = 5.0;
static const float SELECT_SIZE_FACTOR = 1.5; //selected node will appear SELECT_SIZE_FACTOR bigger.

static const int FOOTER_SPEED = 4; //how fast item names move on the bottom of the screen

static const float FADE_TIME = .3; //idle time (in sec) before labels vanish
static const float DECAY_TIME = 2.0; //idle time (in sec) before nodes vanish
static const float SHADOW_STRENGTH = 0.5; //intensity of shadows (from 0.0 to 1.0)
static const vec2f SHADOW_OFFSET(1.0, 1.0); //offset of shadows

static const vec4f DEFAULT_HOVERED_COLOUR(0.55, 0.1, 0.1, 1.0); // Dark red

static const vec4f DEFAULT_ACTIVE_COLOUR(0.99, 0.69, 0.24, 1.0); //Bright orange
static const vec4f DEFAULT_INHIBITED_COLOUR(0., 0.7, 1.0, 1.0); //Blue

static const vec4f DEFAULT_UNITS_COLOUR(0.1, 0.1, 0.1, 1.0);

//static const vec4f DEFAULT_BACKGROUND_COLOUR(0.0, 0.0, 0.0);
static const vec4f DEFAULT_BACKGROUND_COLOUR(1.0, 1.0, 1.0);

/******************************************************************************/

// Colours are defined as global variables
extern vec4f HOVERED_COLOUR;
extern vec4f ACTIVE_COLOUR;
extern vec4f INHIBITED_COLOUR;
extern vec4f UNITS_COLOUR;
extern vec4f BACKGROUND_COLOUR;


#endif // STYLES_H
