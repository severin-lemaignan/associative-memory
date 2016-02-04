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

#ifndef EDGE_H
#define EDGE_H

#include <vector>

#include "edge_renderer.h"
#include "styles.h"

class MemoryView;
class Graph;
class Node;
class NodeRelation;

class Edge
{
    Node* node1;
    Node* node2;

    void updateLength();

    vec2f spos;

    bool stepDone;
    bool renderingDone;

    EdgeRenderer renderer;

    vec4f computeColour() const;

public:
    Edge(const NodeRelation& rel, double weight = 0);

    float length;
    float spring_constant;
    double weight;
    float nominal_length;

    void step(Graph& g, float dt);
    void render(rendering_mode mode, MemoryView& env);

    void setWeight(double weight);

    int getId1() const;
    int getId2() const;

};

#endif // EDGE_H
