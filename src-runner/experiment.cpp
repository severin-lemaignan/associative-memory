#include <iostream>
#include <iterator>
#include <algorithm> // for copy


#include "experiment.hpp"


Experiment::Experiment() {
    activations[0] = {};
}

void Experiment::set_name(const std::string& _name) {name = _name;}

void Experiment::add_unit(std::string& unit) {
    units.insert(unit);
}

void Experiment::add_activation(const std::string& unit, const timeperiod& period) {

    endtime = std::max(endtime, period.stop);

    std::vector<std::string> active_units_before_start;
    std::vector<std::string> active_units_before_stop;

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
        active_units_before_stop.erase(std::remove(active_units_before_stop.begin(), active_units_before_stop.end(), unit), active_units_before_stop.end());
        activations[period.stop] = active_units_before_stop;
    }
}

void Experiment::add_plot(const std::string& unit, const timeperiod& period) {

    endtime = std::max(endtime, period.stop);

    plots.push_back(make_tuple(unit, period.start, period.stop));

}

void Experiment::summary() const
{

    std::cout << "Summary of the experiment \"" << name << "\"" << std::endl << std::endl;

    std::cout << units.size() << " defined units:" << std::endl;
    for (auto unit : units) {
        std::cout << unit << std::endl;
    }

    std::cout << std::endl << "Activations plan:" << std::endl;
    for (auto& kv : activations) {
        std::cout << "  - at " << kv.first << "ms: ";
        if (kv.second.empty()) std::cout << "none";
        else
            copy(kv.second.begin(), kv.second.end(), std::ostream_iterator<std::string>(std::cout, ", "));

        std::cout << std::endl;
    }

    std::cout << std::endl << "Requested plots:" << std::endl;
    for (auto& plot : plots) {
        std::string unit;
        int start, end;

        tie(unit, start, end) = plot;
        std::cout << "Unit <" << unit << "> from " << start << "ms to " << end << "ms" << std::endl;

    }

    std::cout << std::endl << "Total duration: " << endtime << "ms" << std::endl;
}

