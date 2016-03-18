#include <iostream>
#include <iterator>
#include <algorithm> // for copy


#include "experiment.hpp"

using namespace std;

Experiment::Experiment() {
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

    endtime = max(endtime, period.stop);

    vector<string> active_units_before_start;
    vector<string> active_units_before_stop;

    for (auto& kv : activations) {
        auto t = kv.first;
        if (t > period.start) break;

        active_units_before_start = kv.second;
    }

    // insert a point when the unit starts to activate
    activations[period.start] = active_units_before_start;
    activations[period.start].push_back(unit);

    for (auto& kv : activations) {
        auto t = kv.first;
        if (t > period.stop) break;
        active_units_before_stop = kv.second;

        if (t <= period.start) continue;

        // keep it actives at all time < stop
        if (t < period.stop)  {
            kv.second.push_back(unit);
        }
    }

    // if needed, add a point at stop time, and make sure the unit is not active
    if (activations.count(period.stop) == 0) {
        active_units_before_stop.erase(remove(active_units_before_stop.begin(), active_units_before_stop.end(), unit), active_units_before_stop.end());
        activations[period.stop] = active_units_before_stop;
    }
}

void Experiment::add_plot(const string& unit, const timeperiod& period) {

    endtime = max(endtime, period.stop);

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
        cout << "  - at " << kv.first << "ms: ";
        if (kv.second.empty()) cout << "none";
        else
            copy(kv.second.begin(), kv.second.end(), ostream_iterator<string>(cout, ", "));

        cout << endl;
    }

    cout << endl << "Requested plots:" << endl;
    for (auto& plot : plots) {
        string unit;
        int start, end;

        tie(unit, start, end) = plot;
        cout << "Unit <" << unit << "> from " << start << "ms to " << end << "ms" << endl;

    }

    cout << endl << "Total duration: " << endtime << "ms" << endl;
}

