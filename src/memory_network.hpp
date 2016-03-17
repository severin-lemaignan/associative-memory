#ifndef MEMORY_NETWORK
#define MEMORY_NETWORK

#include <Eigen/Dense>
#include <map>
#include <string>
#include <random>
#include <chrono>
#include <thread>

#include <boost/circular_buffer.hpp> // used to store the activation history of each unit

#define NB_INPUT_UNITS 50

const std::chrono::seconds HISTORY_DURATION(5);
const int HISTORY_SAMPLING_RATE=10; // Hz

typedef Eigen::Matrix<double, NB_INPUT_UNITS, NB_INPUT_UNITS> MemoryMatrix;
typedef Eigen::Matrix<double, NB_INPUT_UNITS, 1> MemoryVector;


class MemoryNetwork
{

public:

    MemoryNetwork(double Dg = 0.2,     // activation decay (per ms)
                  double Lg = 0.01,    // learning rate (per ms)
                  double Eg = 0.6,     // external influence
                  double Ig = 0.3,     // internal influence
                  double Amax = 1.0,   // maximum activation
                  double Amin = -0.2,  // minimum activation
                  double Arest = -0.1, // rest activation
                  double Winit = 0.0); // initial weights


    void activate_unit(int id, 
                    double level, 
                    std::chrono::milliseconds duration = std::chrono::milliseconds(200));


    std::vector<std::string> units_names() const {return _units_names;}
    MemoryVector activations() const {return _activations;}
    MemoryMatrix weights() const {return _weights;}

    boost::circular_buffer<double> activationHistory(size_t unit_id) const {
        return _activationsHistory[unit_id];
    }

    size_t size() const {return NB_INPUT_UNITS;}
    int frequency() const {return _frequency;}

    void start();
    void stop();
    bool isrunning() const {return _is_running;}

private:
    const double Dg;
    const double Lg;
    const double Eg;
    const double Ig;
    const double Amax;
    const double Amin;
    const double Arest;
    const double Winit;

    MemoryVector rest_activations; // constant

    std::vector<std::string> _units_names;

    MemoryVector external_activations;
    MemoryVector external_activations_decay;
    MemoryVector internal_activations;
    MemoryVector net_activations;
    MemoryVector _activations;
    MemoryMatrix _weights;


    std::vector<boost::circular_buffer<double>> _activationsHistory;

    std::random_device rd;
    std::default_random_engine gen;

    MemoryVector compute_internal_activations();

    void run();
    void step();

    std::thread _network_thread;

    bool _is_running = false;

    void printout();

    int _frequency = 0;
    int _steps_since_last_frequency_update = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> _last_timestamp;
    std::chrono::time_point<std::chrono::high_resolution_clock> _last_freq_computation;
    std::chrono::time_point<std::chrono::high_resolution_clock> _last_history_store;
};


#endif
