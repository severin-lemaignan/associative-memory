#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;
using namespace std::chrono;


MemoryNetwork::MemoryNetwork(size_t size,
                             LoggingFunction logging_function,
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
                _log_activation(logging_function),
                gen(rd())
{

    rest_activations.resize(size);
    _units_names.resize(size);

    external_activations.resize(size);
    external_activations_decay.resize(size);
    internal_activations.resize(size);
    net_activations.resize(size);
    _activations.resize(size);
    _weights.resize(size, size);


    for (size_t i = 0; i < size; i++) {
        _units_names.push_back(string("input") + to_string(i));
    }

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
                                  milliseconds duration) {
    auto id = unit_id(unit);
    if (id >= _units_names.size()) throw range_error(unit + ": Inexistant unit name!");

    activate_unit(id, level, duration);
}

void MemoryNetwork::activate_unit(size_t id,
                                  double level,
                                  milliseconds duration) {
    external_activations(id) = level;
    external_activations_decay(id) = duration.count();
}

size_t MemoryNetwork::unit_id(const std::string& name) const {
    size_t i = 0;
    for ( ; i < _units_names.size(); i++) {
        if (_units_names[i] == name) return i;
    }
    return i;
}

void MemoryNetwork::units_names(const std::set<std::string>& names) {
    // input is a set as we do not want to identical unit name,
    // but we internally store it as a vector as each unit must have
    // a unique ID.
    if (_is_running) throw runtime_error("Can not change the names of units once the network is running.");

    _units_names.clear();
    copy(names.begin(), names.end(), std::back_inserter(_units_names));
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
    _is_running = true;
    while(_is_running) step();
    cerr << "Memory network finished." << endl;

}

void MemoryNetwork::step()
{

    // Compute dt
    auto now = high_resolution_clock::now();
    duration<double, std::milli> dt = now - _last_timestamp; // fractional duration
    _last_timestamp = now;

    if (_max_freq != 0.f) {
        duration<double, std::milli> target_period(1000./_max_freq);
        std::this_thread::sleep_for(target_period);
    }


    // If needed, compute frequency + optional printout network values
    _steps_since_last_frequency_update++;
    duration<double, std::milli> ms_since_last_freq = now - _last_freq_computation;
    if(ms_since_last_freq.count() > 200) {
        _frequency = _steps_since_last_frequency_update * 1000./ms_since_last_freq.count();
        _last_freq_computation = now;
        _steps_since_last_frequency_update = 0;

        // print out the network activations + weights
        //printout();
    }


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
    
    // decay
    _activations -= Dg * dt.count() * (_activations - rest_activations);

    for (size_t i = 0; i < size(); i++)
    {
        // clamp in [Amin, Amax]
        _activations(i) = min(Amax, max(Amin, _activations(i)));
    }

    // if necessary, log the activations
    if(_log_activation) {
        for (size_t i = 0; i < size(); i++) {
            _log_activation(duration_cast<microseconds>(now - _start_time),
                            _activations);
        }
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
            _weights(i,j) += Lg * dt.count() * _activations(i) * _activations(j) * (1 - _weights(i,j));
            }
            else
            {
            _weights(i,j) += Lg * dt.count() * _activations(i) * _activations(j) * (1 + _weights(i,j));
            }
        }
    }


    // decay the external activations
    for (size_t i = 0; i < external_activations.size(); i++) {

        if (external_activations_decay(i) > 0) {
            external_activations_decay(i) -= dt.count();
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
