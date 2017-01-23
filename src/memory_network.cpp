#include <iostream>
#include <iomanip>
#include <algorithm>
#include <utility> // make_pair
#include <iterator>
#include <ratio>
#include <thread>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;
using namespace std::chrono;


MemoryNetwork::MemoryNetwork(LoggingFunction activations_log_fn,
                             LoggingFunction external_activations_log_fn,
                             double Dg,   
                             double Lg,   
                             double Eg,   
                             double Ig,   
                             double Amax, 
                             double Amin, 
                             double Arest,
                             double Winit) :
                Dg(Dg),   
                Lg(Lg),
                Eg(Eg),
                Ig(Ig),   
                Amax(Amax), 
                Amin(Amin), 
                Arest(Arest),
                Winit(Winit),
                _log_activation(activations_log_fn),
                _log_external_activation(external_activations_log_fn),
                gen(rd())
{

    reset();
}

void MemoryNetwork::reset() {

    rest_activations.fill(Arest);

    external_activations.fill(0);
    internal_activations.fill(0);
    net_activations.fill(0);

    _activations.fill(Arest);
    _weights.fill(NAN);

}

void MemoryNetwork::compute_internal_activations() {

    for (size_t i = 0; i < size(); i++)
    {
        double sum = 0;
        for (size_t j = 0; j < size(); j++)
        {
            if (std::isnan(_weights(i,j))) continue;
            sum += _weights(i, j) * _activations(j);
        }
        internal_activations(i) = sum;
    }
}

void MemoryNetwork::activate_unit(const string& unit,
                                  double level,
                                  microseconds duration) {
    auto id = unit_id(unit);
    activate_unit(id, level, duration);
}

void MemoryNetwork::activate_unit(size_t id,
                                  double level,
                                  microseconds duration) {

    // the excited unit has recently been added, and the network update thread has not yet resized the network. Skip this excitation 
    if (id >= size()) return;

    if(_is_recording) {
        auto now = elapsed_time();

        if (_activations_history[id].empty()) {
            _activations_history[id].push_back(make_tuple(level, now, duration));
        }
        else {
            // check if we can merge with the last record

            chrono::microseconds lasttime,lastduration;
            float lastlevel;
            tie(lastlevel, lasttime,lastduration) = _activations_history[id].back();

            if (lasttime + lastduration < now) { // no overlap with previous, good.
                _activations_history[id].push_back(make_tuple(level, now, duration));
            }
            else {
                if (level != lastlevel) { // trim previous record, and add new one
                    _activations_history[id].back() = make_tuple(lastlevel, lasttime, now - lasttime);
                    _activations_history[id].push_back(make_tuple(level, now, duration));
                }
                else { // same levels: merge!
                    _activations_history[id].back() = make_tuple(level, lasttime, now + duration - lasttime);
                }
            }
        }
    }


    external_activations(id) = level;
    external_activations_decay(id) = duration.count();
}

size_t MemoryNetwork::unit_id(const std::string& name) const {
    size_t i = 0;
    for ( ; i < _units_names.size(); i++) {
        if (_units_names[i] == name) return i;
    }
    throw range_error(name + ": Inexistant unit name!");
}

size_t MemoryNetwork::add_unit(const std::string& name) {

    cerr << "Adding unit " << name << endl;
    if (has_unit(name)) {
        throw runtime_error(name + " is already used. Two units can not have the same name.");
    }

    _units_names.push_back(name);

    return _units_names.size();
}

bool MemoryNetwork::has_unit(const std::string& name) const {
    return (find(_units_names.begin(), _units_names.end(), name) != _units_names.end());
}

void MemoryNetwork::set_parameter(const std::string& name, double value) {

    if (_is_running) throw runtime_error("Can not change the network parameters once the network is running.");

    cerr << "Setting memory network parameter " << name << " to " << value << endl;

    if(name == "Dg") {Dg = value; return;}
    if(name == "Lg") {Lg = value; return;}
    if(name == "Eg") {Eg = value; return;}
    if(name == "Ig") {Ig = value; return;}
    if(name == "Amax") {Amax = value; return;}
    if(name == "Amin") {Amin = value; return;}
    if(name == "Arest") {
        Arest = value;
        rest_activations.fill(Arest);
        _activations.fill(Arest);
        return;}
    if(name == "Winit") {Winit = value; return;}
}

void MemoryNetwork::max_frequency(double freq) {

    if (_is_running) throw runtime_error("Can not change the network parameters once the network is running.");

    if ( freq == 0 && !_use_physical_time) {
        cerr << "Can not set the frequency to infinite when not using physical time! Ignoring." << endl;
        return;
    }

    if (freq == 0) 
        _min_period = microseconds::zero();
    else 
        _min_period = microseconds(int(std::micro::den / freq));

    cerr << "Setting the internal minimal period to " << duration_cast<microseconds>(_min_period).count() << "us" << endl;
}

microseconds MemoryNetwork::elapsed_time() const
{
    if (!_is_running) return microseconds::zero();

    if(_use_physical_time) {
        return duration_cast<microseconds>(high_resolution_clock::now() - _start_time);
    }
    else {
        return _elapsed_time;
    }
}

double MemoryNetwork::get_parameter(const std::string& name) const {

    if(name == "Dg") {return Dg;}
    if(name == "Lg") {return Lg;}
    if(name == "Eg") {return Eg;}
    if(name == "Ig") {return Ig;}
    if(name == "Amax") {return Amax;}
    if(name == "Amin") {return Amin;}
    if(name == "Arest") {return Arest;}
    if(name == "Winit") {return Winit;}

    throw range_error(name + " is not a valid parameter name");
}

void MemoryNetwork::start() {

    _network_thread = thread(&MemoryNetwork::run, this);

    // wait for the thread to be effectively running
    while(!_is_running) {
        this_thread::sleep_for(milliseconds(1));
    };
}

void MemoryNetwork::stop() {

    _is_running = false;
    _network_thread.join();
}

void MemoryNetwork::run() {


    cerr << "Memory network thread started." << endl;
    _start_time = _last_timestamp = _last_freq_computation = high_resolution_clock::now();

    _elapsed_time = microseconds::zero();

    _is_running = true;
    while(_is_running) step();
    cerr << "Memory network finished." << endl;

}

void MemoryNetwork::step()
{

    microseconds dt;

    if (_use_physical_time)
    {
        // Compute dt
        auto now = high_resolution_clock::now();
        dt = duration_cast<microseconds>(now - _last_timestamp);
        _last_timestamp = now;

        if (   _min_period != microseconds::zero()
                && dt < _min_period) {
            this_thread::sleep_for(_min_period - dt);
        }

        // If needed, compute actual update frequency
        _steps_since_last_frequency_update++;
        auto time_since_last_freq = now - _last_freq_computation;
        if(time_since_last_freq > milliseconds(200)) {
            _frequency = _steps_since_last_frequency_update * std::micro::den * 1./duration_cast<microseconds>(time_since_last_freq).count();
            _last_freq_computation = now;
            _steps_since_last_frequency_update = 0;

            // print out the network activations + weights
            //printout();
        }

    }
    else
    {
        dt = _min_period;
        _elapsed_time += dt;
    }

    // If new units were added, resize the network
    // *******************************************
    
    auto nbunits = _units_names.size();
    for(size_t i=size();i<nbunits;i++) incrementsize();

    if (size() == 0) return;

    // Establish connections
    // *********************

    for (size_t i = 0; i < external_activations.size() - 1; i++) {

        if (external_activations(i) == 0) continue;

        for (size_t j = i + 1; j < external_activations.size(); j++) {

            if (external_activations(j) == 0) continue;

            if (std::isnan(_weights(i,j))) {
                    _weights(i,j) = _weights(j,i) = Winit;
            }
        }
    }

    compute_internal_activations();

    net_activations = Eg * external_activations + Ig * internal_activations;

    // Activations update
    // ******************
    for (size_t i = 0; i < size(); i++)
    {

        if (net_activations(i) > 0)
            _activations(i) +=  net_activations(i) * (Amax - _activations(i));
        else
            _activations(i) +=  net_activations(i) * (_activations(i) - Amin);

    }
    
    // dt since last update, in (floating) milliseconds
    double dt_ms = duration_cast<duration<double, std::milli>>(dt).count();

    // decay
    _activations -= Dg * dt_ms * (_activations - rest_activations);

    for (size_t i = 0; i < size(); i++)
    {
        // clamp in [Amin, Amax]
        _activations(i) = min(Amax, max(Amin, _activations(i)));
    }

    // if necessary, log the activations and external stimulations
    auto elapsed_time_so_far = elapsed_time();
    if(_log_activation) {
        _log_activation(elapsed_time_so_far, _activations);
    }
    if(_log_external_activation) {
        _log_external_activation(elapsed_time_so_far,
                                 external_activations);
    }

    // Weights update
    // **************
    for (size_t i = 0; i < size(); i++)
    {
        for (size_t j = 0; j < size(); j++)
        {
            if (std::isnan(_weights(i,j))) continue;

            // only update weights (ie, learn) if the units are co-activated
            if (external_activations(i) * external_activations(j) == 0) continue;

            if (_activations(i) * _activations(j) > 0)
            {
            _weights(i,j) += Lg * dt_ms * _activations(i) * _activations(j) * (1 - _weights(i,j));
            }
            else
            {
            _weights(i,j) += Lg * dt_ms * _activations(i) * _activations(j) * (1 + _weights(i,j));
            }
        }
    }


    // decay the external activations
    for (size_t i = 0; i < external_activations.size(); i++) {

        if (external_activations_decay(i) > 0) {
            external_activations_decay(i) -= duration_cast<microseconds>(dt).count();
        }
        else {
            external_activations(i) = 0;
        }
    }

}


void MemoryNetwork::printout() {

    cerr << "Weights" << endl << setprecision(2) << _weights << endl;

    cerr << setprecision(4) << setw(6) << fixed << "\033[2J";
    cerr << "ID\t\tExternal\tInternal\tNet\t\tActivation" << endl;
    cerr << "--\t\t--------\t--------\t---\t\t----------" << endl;
    for (size_t i = 0; i < size(); i++) {
        cerr << i << "\t\t";
        cerr << external_activations(i) << "\t\t";
        cerr << internal_activations(i) << "\t\t";
        cerr << net_activations(i) << "\t\t";
        cerr << _activations(i) << endl;

    }
    cerr << endl;
}

void MemoryNetwork::incrementsize() {

    auto size = _size + 1;

    rest_activations.conservativeResize(size);
    rest_activations(size-1) = Arest;

    external_activations.conservativeResize(size);
    external_activations(size-1) = 0;

    external_activations_decay.conservativeResize(size);
    external_activations_decay(size-1) = 0;

    internal_activations.conservativeResize(size);
    internal_activations(size-1) = 0;

    net_activations.conservativeResize(size);
    net_activations(size-1) = 0;

    _activations.conservativeResize(size);
    _activations(size-1) = Arest;

    _weights.conservativeResize(size, size);
    _weights.row(size-1).fill(NAN);
    _weights.col(size-1).fill(NAN);

    _size = size;
}

void MemoryNetwork::save_record() {

    stringstream ss;

    double period= chrono::duration<double>(_min_period).count(); 
    std::time_t result = std::time(nullptr);

    ss <<
        "Experiment\n"
        "==========\n"
        "\n"
        "Experiment recorded on " << std::asctime(std::localtime(&result)) << "\n"
        "\n"
        "Network Parameters\n"
        "------------------\n"
        "\n" <<
        "- Dg: " << to_string(Dg) << " (activation decay per ms)\n" <<
        "- Lg: " << to_string(Lg) << " (learning rate per ms)\n" << 
        "- Eg: " << to_string(Eg) << " (external influence)\n" <<
        "- Ig: " << to_string(Ig) << " (internal influence)\n" <<
        "- Amax: " << to_string(Amax) << " (maximum activation)\n" <<
        "- Amin: " << to_string(Amin) << " (minimum activation)\n" <<
        "- Arest: " << to_string(Arest) << " (rest activation)\n" <<
        "- Winit: " << to_string(Winit) << " (initial weights)\n" <<
        "- MaxFreq: " << to_string(period == 0 ? 0 : 1/period) << " (maximum network update frequency -- 0 means no limit)\n" <<
        "\n"
        "Units\n"
        "-----\n"
        "\n";

    for (const auto& unit: _units_names) {
        ss << "- " << unit << "\n";
    }

    ss << "\n"
          "Activations\n"
          "-----------\n"
          "\n";

    for (const auto& unit: _units_names) {
        if (_activations_history[unit_id(unit)].empty()) continue;

        ss << "- " << unit << ":\n";
        for (const auto& interval : _activations_history[unit_id(unit)]) {
            chrono::microseconds start,duration;
            float level;
            tie(level,start,duration) = interval;

            ss << "    - [" << duration_cast<milliseconds>(start).count() << "," << duration_cast<milliseconds>(start + duration).count() << "] at " << level << "\n";

        }
    }

    cout << ss.str();
}
