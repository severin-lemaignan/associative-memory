#ifndef MEMORY_NETWORK
#define MEMORY_NETWORK

#include <Eigen/Dense>
#include <map>
#include <set>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <functional>

typedef Eigen::MatrixXd MemoryMatrix;
typedef Eigen::VectorXd MemoryVector;


typedef std::function<void(std::chrono::duration<long int, std::micro>,
                           const MemoryVector&)> LoggingFunction;

class MemoryNetwork
{

public:

    /** Creates a new associative memory network, initially empty.
     *
     * Call `add_unit` to add new units to the network. `size` returns the
     * current size of the network.
     *
     * Call `start` to start the network. Note that new units can be added at
     * any time, including when the network is running.
     *
     */
    MemoryNetwork(LoggingFunction activations_log_fn = nullptr, // user-defined callback used to store activation history
                  LoggingFunction external_activations_log_fn = nullptr, // user-defined callback used to store external activation history
                  double Dg = 0.2,     // activation decay (per ms)
                  double Lg = 0.01,    // learning rate (per ms)
                  double Eg = 0.6,     // external influence
                  double Ig = 0.3,     // internal influence
                  double Amax = 1.0,   // maximum activation
                  double Amin = -0.2,  // minimum activation
                  double Arest = -0.1, // rest activation
                  double Winit = 0.0); // initial weights

    void reset();

    /** Activate one unit at a specific level, for a specific duration.
     */
    void activate_unit(size_t id, 
                    double level = 1.0, 
                    std::chrono::microseconds duration = std::chrono::milliseconds(200));

    /** Activate one unit at a specific level, for a specific duration.
     *
     * Raises a `range_error` exception is the unit does not exist.
     */
    void activate_unit(const std::string& name, 
                    double level = 1.0, 
                    std::chrono::microseconds duration = std::chrono::milliseconds(200));

    /** Returns the list of all unit names, ordered by their internal IDs.
     *
     * The order is guaranteed to remain the same from one call to the other,
     * even after calling `reset` or `stop`.
     */
    std::vector<std::string> units_names() const {return _units_names;}

    /**
     * Adds a new unit to the network.
     *
     * Returns the internal ID of the newly created unit.
     *
     * Raises a `runtime_error` if the name is already in used.
     */
    size_t add_unit(const std::string& name);

    /** Returns true if the network already has a unit named `name`, false
     * otherwise.
     */
    bool has_unit(const std::string& name) const;

    /** Returns the internal ID of a unit.
     *
     * Raises a `range_error` exception is the unit does not exist.
     */
    size_t unit_id(const std::string& name) const;

    MemoryVector activations() const {return _activations;}
    MemoryMatrix weights() const {return _weights;}

    size_t size() const {return _size;}
    int frequency() const {return _frequency;}

    /** Slow down the memory update mechanism (typically, for debugging) up to
     * 'freq' updated per seconds
     *
     * The update frequency is set to max_freq.  Calling slowdown with
     * max_freq=0 removes any previously set limit.
     */
    void max_frequency(double freq);
    std::chrono::microseconds internal_period() const {return _min_period;}

    /** Changes between physical time and simulated time.
     *
     * By default, the network uses real, physical time.
     */
    void use_physical_time(bool use) {_use_physical_time = use;}

    /** Returns true if using real physical time, false if using simulated time.
     */
    bool is_using_physical_time() const {return _use_physical_time;}

    /** Returns the elapsed time since the network started.
     *
     * If the network has not started yet, returns 0.
     */
    std::chrono::microseconds elapsed_time() const;

    /** Configure the network parameters.
     * Possible parameter names are:
     *  - Dg: decay rate
     *  - Lg: learning rate
     *  - Eg: external influence
     *  - Ig: internal influence
     *  - Amax: maximum activation
     *  - Amin: minimum activation
     *  - Arest: rest activation
     *  - Winit: initial weights
     */
    void set_parameter(const std::string& name, double value);

    /* Returns the current value of a network parameter.
     * See `set_parameter` documentation for the list of parameters.
     *
     * Throws a `range_error` exception if the parameter does not exist.
     */
    double get_parameter(const std::string& name) const;

    /** Starts the memory system
     *
     * Note that the current weights and activations are *not* reset: as such,
     * one may call stop() then start() to pause/unpause the network.
     * Call reset() to actually reset the network to its initial empty
     * state.
     */
    void start();
    void stop();
    bool isrunning() const {return _is_running;}

    void record(bool enabled) {_is_recording=enabled;}
    bool isrecording() {return _is_recording;}
    void save_record();

    double Dg;
    double Lg;
    double Eg;
    double Ig;
    double Amax;
    double Amin;
    double Arest;
    double Winit;

private:
    MemoryVector rest_activations; // constant

    std::vector<std::string> _units_names;

    MemoryVector external_activations;
    MemoryVector external_activations_decay;
    MemoryVector internal_activations;
    MemoryVector net_activations;
    MemoryVector _activations;
    MemoryMatrix _weights;

    LoggingFunction _log_activation;
    LoggingFunction _log_external_activation;

    std::random_device rd;
    std::default_random_engine gen;

    void compute_internal_activations();

    void run();
    void step();

    size_t _size = 0;

    /** Conservatively increment the size the network. Conserves the current
     * weights, activations.
     *
     * *Needs to be called from the network update thread!*
     */
    void incrementsize();

    std::thread _network_thread;

    bool _is_running = false;

    bool _is_recording = false;
    std::map<size_t, std::vector<std::pair<std::chrono::microseconds, std::chrono::microseconds>>> _activations_history;

    void printout();

    std::chrono::microseconds _min_period = std::chrono::microseconds::zero();
    bool _use_physical_time = true;

    int _frequency = 0;
    int _steps_since_last_frequency_update = 0;

    std::chrono::high_resolution_clock::time_point _start_time;
    std::chrono::high_resolution_clock::time_point _last_timestamp;
    std::chrono::high_resolution_clock::time_point _last_freq_computation;

    // only used when _use_physical_time = false
    std::chrono::microseconds _elapsed_time;
};


#endif
