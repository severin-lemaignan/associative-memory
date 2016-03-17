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

#include <algorithm>

#include "macros.h"
#include "constants.h"

#include "memoryview.h"
#include "graph.h"
#include "node.h"
#include "node_relation.h"


using namespace std;
using namespace boost;

/** A simple boolean function that returns true for characters that are not
  "safe" (currently in the sense of GraphViz names.
**/
bool safeIdFilter(char c) {
    return c=='-' || c==':' || c=='_';
}

Node::Node(int id, const string& label, const Node* neighbour) :
    id(id),
    safeid(to_string(id)),
    label(label),
    renderer(NodeRenderer(id, label)),
    _selected(false),
    decayTime(0.0),
    decaySpeed(1.0),
    decaying(true),
    distance_to_selected(-1),
    distance_to_selected_updated(false),
    base_charge(INITIAL_CHARGE),
    charge(INITIAL_CHARGE)
{

    safeid.resize(std::remove_if(safeid.begin(), safeid.end(), safeIdFilter) - safeid.begin());

    //If a neighbour is given, we set our initial position close to it.
    if (neighbour)
        pos = neighbour->pos + vec2f(10.0 * (float)rand()/RAND_MAX - 5 , 10.0 * (float)rand()/RAND_MAX - 5);
    else
        pos = vec2f(100.0 * (float)rand()/RAND_MAX - 50 , 100 * (float)rand()/RAND_MAX - 50);

    speed = vec2f(0.0, 0.0);

    mass = INITIAL_MASS;
    damping = INITIAL_DAMPING;


}

bool Node::operator< (const Node& node2) const {
    return id < node2.getID();
}

const string& Node::getSafeID() const {
    return safeid;
}

void Node::setColour(vec4f col) {
    renderer.setColour(col);
}

std::vector<NodeRelation>& Node::getRelations() {
    return relations;
}

vector<Node*>  Node::getConnectedNodes(){
    vector<Node*> res;

    for(auto& rel : relations) {
        if (rel.to == this)
            res.push_back(rel.from);
        else
            res.push_back(rel.to);
    }
    return res;
}

bool Node::isConnectedTo(Node* node) {
    for(auto& rel : relations) {
        if (rel.to == node || rel.from == node)
            return true;
    }
    return false;
}

NodeRelation& Node::addRelation(Node& to) {

    vector<const NodeRelation*> rels = getRelationTo(to);

    relations.push_back(NodeRelation(this, &to)); //Add a new relation

    if (!to.isConnectedTo(this)){
        to.addRelation(*this);
    }

    TRACE("Added relation from " << label << " to " << to.label);

    return relations.back();

}

vector<const NodeRelation*> Node::getRelationTo(Node& node) const {

    vector<const NodeRelation*> res;

    for(const auto& rel : relations) {
        if (rel.to == &node)
            res.push_back(&rel);
    }
    return res;
}


void Node::updateKineticEnergy() {
    kinetic_energy = mass * speed.length2();
}


void Node::step(Graph& g, float dt){

    TRACE("Updating " << label << " color based on activity");
    if (activity > 0)
        setColour(vec4f(activity + 0.1, activity * 0.5 + 0.1, 0.1, 1.0));
    else
        setColour(vec4f(0.1,0.1, -activity + 0.1, 1.0));


    /** Compute here the new position of the node **/

    TRACE("Stepping for node " << label);
    vec2f force = vec2f(0.0, 0.0);

    // Algo from Wikipedia -- http://en.wikipedia.org/wiki/Force-based_layout

    coulombForce = g.coulombRepulsionFor(*this);
    force +=coulombForce;

    hookeForce = g.hookeAttractionFor(*this);
    force += hookeForce;

    TRACE("** Total force applying: Fx=" << force.x << ", Fy= " << force.y);

    speed = (speed + force * dt) * damping;
    speed.x = CLAMP(speed.x, -MAX_SPEED, MAX_SPEED);
    speed.y = CLAMP(speed.y, -MAX_SPEED, MAX_SPEED);

    updateKineticEnergy();

    //Check we have enough energy to move :)
    if (kinetic_energy > MIN_KINETIC_ENERGY) {
        pos += speed * dt;
    }

        if (decaying) decayTime += dt;
        decay();
        //Update the age of the node renderer
        renderer.increment_idle_time(dt);

    TRACE("Node " << label << " now in pos=(" << pos.x << ", " << pos.y <<")");


    //TRACE("Step computed for " << id << ". Speed is " << speed.x << ", " << speed.y << " (energy: " << kinetic_energy << ").");

}

void Node::render(rendering_mode mode, MemoryView& env, bool debug){

#ifndef TEXT_ONLY
        if (mode == GRAPHVIZ) {
            env.graphvizGraph << safeid;
        }
        renderer.activation = activity;
        renderer.draw(pos, mode, env, distance_to_selected);

        if (debug) {
            vec4f col(1.0, 0.2, 0.2, 0.7);
            MemoryView::drawVector(hookeForce , pos, col);

            col = vec4f(0.2, 1.0, 0.2, 0.7);
            MemoryView::drawVector(coulombForce , pos, col);
        }

#endif

}

void Node::decay() {

    if(decaying) {
        float decayRatio = 1.0 - decayTime / (DECAY_TIME * 100.0 / decaySpeed);

        if (decayRatio < 0.0) {
            // End of decay period
            decayRatio = 0.0;
            decaying = false;
            decayTime = 0.0;
            decaySpeed = 1.0;
        }

        renderer.decayRatio = decayRatio;

        charge = base_charge + ((charge - base_charge) * decayRatio);
    }
}

void Node::setSelected(bool select) {

    if ((select && _selected)||
    (!select && !_selected)) return;

    _selected = select;
    renderer.setSelected(select);

}

void Node::hovered(bool hovered) {

    renderer.setMouseOver(hovered);
    _hovered = hovered;
}

std::ostream& operator<<(std::ostream& os, const Node& n)
{
    os << n.label;
    return os;
}

