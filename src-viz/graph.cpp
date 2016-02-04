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

#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>

#include <iterator>
#include <utility>
#include <fstream>
#include <iostream>

#include "memoryview.h"
#include "macros.h"
#include "graph.h"
#include "edge.h"
#include "node_relation.h"

using namespace std;
using namespace boost;

Graph::Graph()
{
}


void Graph::step(float dt) {

    for(auto& e : edges) {
        e.step(*this, dt);
    }

    for(auto& n : nodes) {

        n.second.step(*this, dt);
    }
}

void Graph::render(rendering_mode mode, MemoryView& env, bool debug) {

    // Renders edges
    for(auto& e : edges) {
        e.render(mode, env);
    }

    // Renders nodes
    for(auto& n : nodes) {
        n.second.render(mode, env, debug);
    }

}

const Graph::NodeMap& Graph::getNodes() const {
    return nodes;
}

Node& Graph::getNode(int id) {
    return nodes.at(id);
}

const Node& Graph::getConstNode(int id) const {
    return nodes.at(id);
}

Node* Graph::getRandomNode() {
    if(nodes.size() == 0) return nullptr;
    NodeMap::iterator it = nodes.begin();
    advance( it, rand()%nodes.size());
    return &(it->second);
}

void Graph::select(Node *node) {

   //Already selected?
    if (node->selected()) return;

    node->setSelected(true);
    selectedNodes.insert(node);

    updateDistances();
}

void Graph::deselect(Node *node){

    //Already deselected?
    if (!node->selected()) return;

    node->setSelected(false);
    selectedNodes.erase(node);

    updateDistances();
}

void Graph::clearSelect(){
    for(auto node : selectedNodes) {
        node->setSelected(false);
    }

    selectedNodes.clear();

    updateDistances();
}

Node* Graph::getSelected() {
    if (selectedNodes.size() >= 1)
        return *selectedNodes.begin();

    return nullptr;
}

Node* Graph::getHovered() {
    for (auto& node : nodes) {
        if (node.second.hovered()) return &node.second;
    }

    return nullptr;
}


Node& Graph::addNode(int id, const string& label, const Node* neighbour) {

    pair<NodeMap::iterator, bool> res;

    //TODO: I'm doing 2 !! copies of Node, here??

    res = nodes.insert(make_pair(id,Node(id, label, neighbour)));

    if ( ! res.second )
        TRACE("Didn't add node " << label << " because it already exists.");
    else {
        TRACE("Added node " << label);
        updateDistances();
    }

    return res.first->second;
}

void Graph::addEdge(Node& from, Node& to) {

    if (&from == &to) {
        return;
    }

    NodeRelation& rel = from.addRelation(to);
    edges.push_back(Edge(rel, 0));
}

vector<const Edge*>  Graph::getEdgesFor(const Node& node) const{
    vector<const Edge*> res;

    for(const auto& e : edges) {
        if (e.getId1() == node.getID() ||
                e.getId2() == node.getID())
            res.push_back(&e);
    }
    return res;
}

Edge*  Graph::getEdge(const Node& node1, const Node& node2){

    for(Edge& e : edges) {
        if ((e.getId1() == node1.getID() && e.getId2() == node2.getID()) ||
            (e.getId1() == node2.getID() && e.getId2() == node1.getID()))
            return &e;
    }
    return nullptr;
}

void Graph::updateDistances() {

    // No node selected, set all distance to -1
    if (selectedNodes.empty()) {
        // Renders nodes
        for(auto& n : nodes) {
            n.second.distance_to_selected = -1;
        }

        return;
    }
    //Else, start from the selected node
    for(auto& n : nodes) {
        n.second.distance_to_selected_updated = false;
    }

    for(auto node : selectedNodes) {
        recurseUpdateDistances(node, nullptr, 0);
    }

}

void Graph::recurseUpdateDistances(Node* node, Node* parent, int distance) {
    node->distance_to_selected = distance;
    node->distance_to_selected_updated = true;
    TRACE("Node " << node->getID() << " is at " << distance << " nodes from closest selected");

    for(auto n : node->getConnectedNodes()){
        if (n != parent &&
            (!n->distance_to_selected_updated || distance < n->distance_to_selected))
                recurseUpdateDistances(n, node, distance + 1);
    }
}

int Graph::nodesCount() {
    return nodes.size();
}

int Graph::edgesCount() {
    return edges.size();
}

vec2f Graph::coulombRepulsionFor(const Node& node) const {

    vec2f force(0.0, 0.0);

    //TODO: a simple optimization can be to compute Coulomb force
    //at the same time than Hooke force when possible -> one
    // less distance computation (not sure it makes a big difference)

    for(const auto& nm : nodes) {
        const Node& n = nm.second;
        if (&n != &node) {
            vec2f delta = n.pos - node.pos;

            //Coulomb repulsion force is in 1/r^2
            float len = delta.length2();
            if (len < 0.01) len = 0.01; //avoid dividing by zero

            float f = COULOMB_CONSTANT * n.charge * node.charge / len;

            force += project(f, delta);
        }
    }

    return force;

}

vec2f Graph::coulombRepulsionAt(const vec2f& pos) const {

    vec2f force(0.0, 0.0);

    //TODO: a simple optimization can be to compute Coulomb force
    //at the same time than Hooke force when possible -> one
    // less distance computation (not sure it makes a big difference)

    for(const auto& nm : nodes) {
        const Node& n = nm.second;

        vec2f delta = n.pos - pos;

        //Coulomb repulsion force is in 1/r^2
        float len = delta.length2();
        if (len < 0.01) len = 0.01; //avoid dividing by zero

        float f = COULOMB_CONSTANT * n.charge * INITIAL_CHARGE / len;

        force += project(f, delta);
    }

    return force;

}

vec2f Graph::hookeAttractionFor(const Node& node) const {

    vec2f force(0.0, 0.0);

    //TODO: a simple optimization can be to compute Coulomb force
    //at the same time than Hooke force when possible -> one
    // less distance computation (not sure it makes a big difference)

    for(const auto e : getEdgesFor(node)) {

        // shortcut if the spring constant is zero or undefined
        if (e->spring_constant == 0 || std::isnan(e->spring_constant)) continue;

        const Node& n_tmp = getConstNode(e->getId1());

        //Retrieve the node at the edge other extremity
        const Node& n2 = ( (&n_tmp != &node) ? n_tmp : getConstNode(e->getId2()) );

        TRACE("\tComputing Hooke force from " << node.getID() << " to " << n2.getID());

        vec2f delta = n2.pos - node.pos;

        float f = - e->spring_constant * (e->length - e->nominal_length);

        force += project(f, delta);
    }

    return force;
}

vec2f Graph::gravityFor(const Node& node) const {
    //Gravity... well, it's actually more like anti-gravity, since it's in:
    // f = g * m * d
    vec2f force(0.0, 0.0);

    float len = node.pos.length2();

    if (len < 0.01) len = 0.01; //avoid dividing by zero

    float f = GRAVITY_CONSTANT * node.mass * len * 0.01;

    force += project(f, node.pos);

    return force;
}

vec2f Graph::project(float force, const vec2f& d) const {
    //we need to project this force on x and y
    //-> Fx = F.cos(arctan(Dy/Dx)) = F/sqrt(1-(Dy/Dx)^2)
    //-> Fy = F.sin(arctan(Dy/Dx)) = F.(Dy/Dx)/sqrt(1-(Dy/Dx)^2)
    vec2f res(0.0, 0.0);

    TRACE("\tForce: " << force << " - Delta: (" << d.x << ", " << d.y << ")");

    if (d.y == 0.0) {
        res.x = force;
        return res;
    }

    if (d.x == 0.0) {
        res.y = force;
        return res;
    }

    float dydx = d.y/d.x;
    float sqdydx = 1/sqrt(1 + dydx * dydx);

    res.x = force * sqdydx;
    if (d.x > 0.0) res.x = - res.x;
    res.y = force * sqdydx * abs(dydx);
    if (d.y > 0.0) res.y = - res.y;

    TRACE("\t-> After projection: Fx=" << res.x << ", Fy=" << res.y);

    return res;
}

void Graph::saveToGraphViz(MemoryView& env) {

    env.graphvizGraph << "strict graph memorynetwork {\n";

    // Renders edges
    for(auto& e : edges) {
        e.render(GRAPHVIZ, env);
    }

    // Renders nodes
    for(auto& n : nodes) {
        n.second.render(GRAPHVIZ, env, false);
    }

    env.graphvizGraph << "}\n";

    ofstream graphvizFile;
    graphvizFile.open ("memory.dot");
    graphvizFile << env.graphvizGraph.str();
    graphvizFile.close();

    cout << "Model correctly exported to memory.dot" << endl;
}

