#include <iostream>
#include <iomanip>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;
using namespace std::chrono;


MemoryNetwork::MemoryNetwork(double Eg,   
                             double Ig,   
                             double Dg,   
                             double Amax, 
                             double Amin, 
                             double Arest,
                             double Lg,   
                             double Winit) :
                Eg(Eg),
                Ig(Ig),   
                Dg(Dg),   
                Amax(Amax), 
                Amin(Amin), 
                Arest(Arest),
                Lg(Lg),
                Winit(Winit),
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
    _activations -= Dg * (_activations - rest_activations);

    // Weights update
    // **************
    for (size_t i = 0; i < NB_INPUT_UNITS; i++)
    {
        for (size_t j = 0; j < NB_INPUT_UNITS; j++)
        {
            if (std::isnan(_weights(i,j))) continue;

            if (_activations(i) * _activations(j) > 0)
            {
            _weights(i,j) += Lg * _activations(i) * _activations(j) * (1 - _weights(i,j));
            }
            else
            {
            _weights(i,j) += Lg * _activations(i) * _activations(j) * (1 + _weights(i,j));
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

    cout << "External\tInternal\tNet\t\tActivation" << endl;
    cout << "--------\t--------\t---\t\t----------" << endl;
    cout << setprecision(4) << setw(6) << fixed;
    for (size_t i = 0; i < NB_INPUT_UNITS; i++) {
        cout << external_activations(i) << "\t\t";
        cout << internal_activations(i) << "\t\t";
        cout << net_activations(i) << "\t\t";
        cout << _activations(i) << endl;

    }
    cout << endl;
}
