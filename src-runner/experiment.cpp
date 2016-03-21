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

void Experiment::add_activation(const string& unit, const timeperiod& period) {

    duration = milliseconds(max(duration.count(), period.stop));

    vector<string> active_units_before_start;
    vector<string> active_units_before_stop;

    activations[period.start].push_back({unit, milliseconds(period.stop - period.start)});
}

void Experiment::add_plot(const string& unit, const timeperiod& period) {

    duration = milliseconds(max(duration.count(), period.stop));

    plots.push_back(make_tuple(unit, period.start, period.stop));

}

void Experiment::summary() const
{

    cout << "Summary of the experiment \"" << name << "\"" << endl << endl;

    cout << "Network parameters:" << endl;
    for (auto& kv : parameters) {
        cout << "- " << kv.first << ": " << kv.second << endl;
    }


    cout << endl << units.size() << " defined units:" << endl;
    for (auto unit : units) {
        cout << unit << endl;
    }

    cout << endl << "Activations plan:" << endl;
    for (auto& kv : activations) {
        cout << "  - at " << kv.first << "ms: " << endl;
        for (auto& activation : kv.second) {
            cout << "    - " << activation.first << " for " << activation.second.count() << "ms" << endl;
        }

    }

    cout << endl << "Requested plots:" << endl;
    for (auto& plot : plots) {
        string unit;
        int start, end;

        tie(unit, start, end) = plot;
        cout << "Unit <" << unit << "> from " << start << "ms to " << end << "ms" << endl;

    }

    cout << endl << "Total duration: " << duration.count() << "ms" << endl;
}

