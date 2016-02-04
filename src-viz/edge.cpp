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

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <boost/foreach.hpp>

#include "core/vectors.h"

#include "macros.h"
#include "memoryview_exceptions.h"

#include "memoryview.h"
#include "edge.h"
#include "node_relation.h"
#include "node.h"
#include "graph.h"

using namespace std;
using namespace boost;

Edge::Edge(const NodeRelation& rel, double weight) :
    node1(rel.from),
    node2(rel.to),
    weight(weight),
    renderer(EdgeRenderer(hash_value(rel.from->getID() + rel.to->getID())))
{
    //    addReferenceRelation(rel);

    spring_constant = INITIAL_SPRING_CONSTANT * weight;
    nominal_length = NOMINAL_EDGE_LENGTH;

    length = 0.0;
}

void Edge::setWeight(double _weight){
    weight = _weight;
    spring_constant = max(0., INITIAL_SPRING_CONSTANT * weight);
}

void Edge::step(Graph& g, float dt){

    if(std::isnan(weight)) return;

    updateLength();

#ifndef TEXT_ONLY

    const vec2f& pos1 = node1->pos;
    const vec2f& pos2 = node2->pos;

    //update the spline point
    vec2f td = (pos2 - pos1) * 0.5;

    vec2f mid = pos1 + td;// - td.perpendicular() * pos.normal();// * 10.0;

    vec2f delta = (mid - spos);

    //dont let spos get more than half the length of the distance behind
    if(delta.length2() > td.length2()) {
        spos += delta.normal() * (delta.length() - td.length());
    }

    spos += delta * min(1.0, dt * 2.0);

    td.normalize();

    vec2f out_of_node1_distance = td * (
                (node1->selected() ? NODE_SIZE * SELECT_SIZE_FACTOR : NODE_SIZE) / 2 + 2
                );

    vec2f out_of_node2_distance = td * (
                (node2->selected() ? NODE_SIZE * SELECT_SIZE_FACTOR : NODE_SIZE) / 2 + 2
                );

    if (   node1->selected() 
        || node2->selected()
        || node1->hovered()
        || node2->hovered())
    {
        renderer.selected = true;
    }
    else {
        renderer.selected = false;
    }

    //Update the age of the edge renderer
    renderer.increment_idle_time(dt);


    //renderer.update(pos1 + out_of_node1_distance , node1->renderer.col,
    //                pos2  - out_of_node2_distance , node2->renderer.col, spos);

    auto col = computeColour();
    renderer.update(pos1 + out_of_node1_distance , col,
                    pos2  - out_of_node2_distance , col, spos);

#endif
    TRACE("Edge between " << node1->getID() << " and " << node2->getID() << " updated.");

}

vec4f Edge::computeColour() const {
    vec4f col;
    if (renderer.selected) {
        col = HOVERED_COLOUR;
    }
    else {
        if (weight > 0) {
            col = vec4f((float) weight, (float) weight * 0.5, 0, 0.7);
        }
        else {
            col = vec4f(0,0, (float) -weight, 0.7);
        }
    }

    return col;
}

void Edge::render(rendering_mode mode, MemoryView& env){



#ifndef TEXT_ONLY
    if (mode == GRAPHVIZ and !std::isnan(weight)) {
        std::stringstream str;
        auto col = computeColour();
        str << "#" << setfill('0') << setw(2) << hex << (int) floor(col.x * 256) << setw(2) << (int) floor(col.y * 256) << setw(2) << (int) floor(col.z * 256);

        env.graphvizGraph << node1->getSafeID() << " -- " << node2->getSafeID() << " [label=" << fixed << setprecision( 2 ) << weight <<", color=\"" << str.str() << "\"];\n";
        return;
    }

    if (std::isnan(weight)) {
        renderer.label = "";
    }
    else {
        std::stringstream str;
        str << fixed << setprecision( 2 ) << weight;
        renderer.label = str.str();
    }
    renderer.draw(mode, env);
#endif
    //TRACE("Edge between " << node1->getID() << " and " << node2->getID() << " rendered.");

}

void Edge::updateLength() {
    //TODO: optimisation by using length2 here?
    length = (node1->pos -  node2->pos).length();
}

int Edge::getId1() const {
    return node1->getID();
}

int Edge::getId2() const {
    return node2->getID();
}
