/*
    Copyright (c) 2010 Séverin Lemaignan (slemaign@laas.fr)
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

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

#include <string>

#include "node_renderer.h"
#include "macros.h"
#include "memoryview.h"

using namespace std;

NodeRenderer::NodeRenderer(int tagid, string label) :
    tagid(tagid),
    label(label),
    idle_time(0.0),
    hovered(false),
    selected(false),
    base_size(NODE_SIZE),
    base_fontsize(BASE_FONT_SIZE)
{

    size = base_size * 1.2;
    fontsize = base_fontsize;

#ifndef TEXT_ONLY
    base_col = UNITS_COLOUR;
    icon = texturemanager.grab("instances.png");

    col = base_col * 1.2;
#endif


}

void NodeRenderer::setColour(vec4f col) {
    base_col = col;
}

void NodeRenderer::computeColourSize() {
    if (selected) {
        size = base_size * SELECT_SIZE_FACTOR;
    }
    else {
        base_size = NODE_SIZE;

    }
    if (hovered) col = HOVERED_COLOUR;
    else {
        if (activation > 0) {
            col = UNITS_COLOUR + ((ACTIVE_COLOUR - UNITS_COLOUR) * activation);
        }
        else {
            col = UNITS_COLOUR + ((INHIBITED_COLOUR - UNITS_COLOUR) * -activation);
        }
    }
}

void NodeRenderer::decay() {

    if (decayRatio > 0.0) {
        col = base_col + ((col - base_col) * decayRatio);
        size = base_size + ((size - base_size) * decayRatio);
        fontsize = base_fontsize + ((fontsize - base_fontsize) * decayRatio);
    }
}

float NodeRenderer::getAlpha() {

    return std::max(0.0f, (FADE_TIME - idle_time) / FADE_TIME);
}

void NodeRenderer::increment_idle_time(float dt) {
    if (selected || hovered) idle_time = 0.0;
    else idle_time += dt;
}

void NodeRenderer::draw(const vec2f& pos, rendering_mode mode, MemoryView& env, int distance_to_selected) {

    std::stringstream str;
    str << fixed << setprecision( 1 ) << activation;

    switch (mode) {

    case NORMAL:
    case SIMPLE:
        computeColourSize();

        drawSimple(pos);
        break;

    case NAMES:
        if(!label.empty()) drawName(pos + vec2f(5,-2), env.font, label);
        drawName(pos + vec2f(5,8), env.font, str.str(), 0.8);

        break;

    case BLOOM:
        drawBloom(pos);
        break;

    case SHADOWS:
        drawShadow(pos);
        break;

    case GRAPHVIZ:
        float halfsize = size * 0.5f;
        vec2f offsetpos = pos - vec2f(halfsize, halfsize);

        std::stringstream strcol;
        strcol << "#" << setfill('0') << setw(2) << hex << (int) floor(col.x * 256) << setw(2) << (int) floor(col.y * 256) << setw(2) << (int) floor(col.z * 256);

        env.graphvizGraph << " [label=\"" << label
                          << "\", height=0.2"
                          << ", style=filled"
                          << ", fontcolor=\"" << ((col.x+col.y+col.z > 0.5) ? "black":"white")
                          << "\", fillcolor=\"" << strcol.str()
                          << "\", pos=\"" << offsetpos.x << "," << offsetpos.y << "\"];\n";
        return;
    }


}


void NodeRenderer::drawSimple(const vec2f& pos){


    glLoadName(tagid + 1); // '+1' because otherwise the node ID 0 can not be distinguished from background

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    float ratio = icon->h / (float) icon->w;
    float halfsize = size * 0.5f;
    vec2f offsetpos = pos - vec2f(halfsize, halfsize);

    glBindTexture(GL_TEXTURE_2D, getIcon()->textureid);

    glPushMatrix();
    glTranslatef(offsetpos.x, offsetpos.y, 0.0f);

    col.w = 1.0;

    glColor4fv(col);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f,0.0f);
    glVertex2f(0.0f, 0.0f);

    glTexCoord2f(1.0f,0.0f);
    glVertex2f(size, 0.0f);

    glTexCoord2f(1.0f,1.0f);
    glVertex2f(size, size*ratio);

    glTexCoord2f(0.0f,1.0f);
    glVertex2f(0.0f, size*ratio);
    glEnd();

    glPopMatrix();

    glLoadName(0);

}

void NodeRenderer::drawName(const vec2f& pos,
                            FXFont& font, 
                            string text, 
                            float font_scale)
{

    glPushMatrix();
    glTranslatef(pos.x, pos.y, 0.0);

    glColor4f(1.0, 1.0, 1.0, getAlpha());

    vec3f screenpos = display.project(vec3f(0.0, 0.0, 0.0));

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, display.width, display.height, 0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    font.setFontSize(fontsize * font_scale);
    font.draw(screenpos.x, screenpos.y, text);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopMatrix();
}

void NodeRenderer::drawBloom(const vec2f& pos){

    float bloom_radius = 50.0;

    vec4f bloom_col = col;

    float alpha = getAlpha();
    glColor4f(bloom_col.x *  alpha,
              bloom_col.y * alpha,
              bloom_col.z * alpha,
              1.0);

    glPushMatrix();
    glTranslatef(pos.x, pos.y, 0.0);

    glBegin(GL_QUADS);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(bloom_radius,bloom_radius);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(bloom_radius,-bloom_radius);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-bloom_radius,-bloom_radius);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(-bloom_radius,bloom_radius);
    glEnd();
    glPopMatrix();

}

void NodeRenderer::drawShadow(const vec2f& pos){

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    float ratio = icon->h / (float) icon->w;
    float halfsize = size * 0.5f;
    vec2f offsetpos = pos - vec2f(halfsize, halfsize) + SHADOW_OFFSET;;

    glBindTexture(GL_TEXTURE_2D, getIcon()->textureid);

    glPushMatrix();
    glTranslatef(offsetpos.x, offsetpos.y, 0.0f);

    glColor4f(0.0, 0.0, 0.0, SHADOW_STRENGTH * getAlpha());

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f,0.0f);
    glVertex2f(0.0f, 0.0f);

    glTexCoord2f(1.0f,0.0f);
    glVertex2f(size, 0.0f);

    glTexCoord2f(1.0f,1.0f);
    glVertex2f(size, size*ratio);

    glTexCoord2f(0.0f,1.0f);
    glVertex2f(0.0f, size*ratio);
    glEnd();

    glPopMatrix();

}


void NodeRenderer::setMouseOver(bool over) {
    hovered = over;

}

void NodeRenderer::setSelected(bool select) {
    selected = select;
}
