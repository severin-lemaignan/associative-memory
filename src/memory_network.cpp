#include <iostream>
#include <iomanip>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;
using namespace std::chrono;


MemoryNetwork::MemoryNetwork(double Dg,   
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
                _activationsHistory(NB_INPUT_UNITS, boost::circular_buffer<double>(HISTORY_DURATION.count() * HISTORY_SAMPLING_RATE,0.0)),
                gen(rd())
{

    for (size_t i = 0; i < NB_INPUT_UNITS; i++) {
        _units_names.push_back(string("input") + to_string(i));
    }

    rest_activations.fill(Arest);

    external_activations.fill(0);
    internal_activations.fill(0);
    net_activations.fill(0);

    _activations.fill(Arest);
    _weights.fill(NAN);
}

MemoryVector MemoryNetwork::compute_internal_activations() {

    for (size_t i = 0; i < NB_INPUT_UNITS; i++)
    {
        double sum = 0;
        for (size_t j = 0; j < NB_INPUT_UNITS; j++)
        {
            if (std::isnan(_weights(i,j))) continue;
            sum += _weights(i, j) * _activations(j);
        }
        internal_activations(i) = sum;
    }
}

void MemoryNetwork::activate_unit(int id,
                                  double level,
                                  milliseconds duration) {
    external_activations(id) = level;
    external_activations_decay(id) = 1.0/duration.count();
}

void MemoryNetwork::start() {

    _is_running = true;
     _network_thread = thread(&MemoryNetwork::run, this);
}

void MemoryNetwork::stop() {

    _is_running = false;
    _network_thread.join();
}

void MemoryNetwork::run() {


    cerr << "Memory network thread started." << endl;
    _last_timestamp = _last_freq_computation = high_resolution_clock::now();
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
        printout();
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
    for (size_t i = 0; i < NB_INPUT_UNITS; i++)
    {

        if (net_activations(i) > 0)
            _activations(i) +=  net_activations(i) * (Amax - _activations(i));
        else
            _activations(i) +=  net_activations(i) * (_activations(i) - Amin);


        // clamp in [Amin, Amax]
        _activations(i) = min(Amax, max(Amin, _activations(i)));
    }
    
    // decay
    _activations -= Dg * dt.count() * (_activations - rest_activations);

    // if necessary, store the activation history
    duration<double, std::milli> ms_since_last_history = now - _last_history_store;
    if(ms_since_last_history.count() > (1000./HISTORY_SAMPLING_RATE)) {
        _last_history_store = now;

        for (size_t i = 0; i < NB_INPUT_UNITS; i++) {
            _activationsHistory[i].push_back(_activations[i]);
        }
    }

    // Weights update
    // **************
    for (size_t i = 0; i < NB_INPUT_UNITS; i++)
    {
        for (size_t j = 0; j < NB_INPUT_UNITS; j++)
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

        external_activations(i) -= external_activations_decay(i) * dt.count();

        if (external_activations(i) <= 0) {
            external_activations_decay(i) = 0;
            external_activations(i) = 0;
        }
    }

}

void MemoryNetwork::printout() {

    //cout << "Weights" << endl << setprecision(2) << _weights << endl;

    cout << setprecision(4) << setw(6) << fixed << "\033[2J";
    cout << "ID\t\tExternal\tInternal\tNet\t\tActivation" << endl;
    cout << "--\t\t--------\t--------\t---\t\t----------" << endl;
    for (size_t i = 0; i < NB_INPUT_UNITS; i++) {
        cout << i << "\t\t";
        cout << external_activations(i) << "\t\t";
        cout << internal_activations(i) << "\t\t";
        cout << net_activations(i) << "\t\t";
        cout << _activations(i) << endl;

    }
    cout << endl;
}
