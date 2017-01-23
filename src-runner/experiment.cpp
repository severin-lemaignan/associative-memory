#include <iostream>
#include <iterator>
#include <algorithm> // for copy


#include "experiment.hpp"

using namespace std;
using namespace std::chrono;

Experiment::Experiment() : duration(0) {
    activations[0] = {};
}

void Experiment::set_name(const string& _name) {name = _name;}

void Experiment::store_param(parameter& param) {
    parameters[param.name] = param.value;
}

void Experiment::add_unit(string& unit) {
    units.insert(unit);
}

void Experiment::add_activation(const string& unit, const activationperiod& period) {

    duration = milliseconds(max(duration.count(), period.stop));

    activations[period.start].push_back(make_tuple(unit, period.level, milliseconds(period.stop - period.start)));
}

void Experiment::add_plot(const string& unit, const timeperiod& period) {

    duration = milliseconds(max(duration.count(), period.stop));

    plots[unit].push_back(period);

}

void Experiment::summary() const
{

    cerr << "Summary of the experiment \"" << name << "\"" << endl << endl;

    cerr << "Network parameters:" << endl;
    for (auto& kv : parameters) {
        cerr << "- " << kv.first << ": " << kv.second << endl;
    }


    cerr << endl << units.size() << " defined units:" << endl;
    for (auto unit : units) {
        cerr << unit << endl;
    }

    cerr << endl << "Activations plan:" << endl;
    for (auto& kv : activations) {
        cerr << "  - at " << kv.first << "ms: " << endl;
        for (auto& activation : kv.second) {
            cerr << "    - " << std::get<0>(activation) << 
                " at level " << std::get<1>(activation) << 
                     " for " << std::get<2>(activation).count() << "ms" << endl;
        }

    }

    cerr << endl << "Requested plots:" << endl;
    for (auto& kv : plots) {
        cerr << "  - for " << kv.first << ": " << endl;
        for (auto& period : kv.second) {
            cerr << "    - from " << period.start << "ms to " << period.stop << "ms" << endl;
        }

    }

    cerr << endl << "Total duration: " << duration.count() << "ms" << endl;
}

