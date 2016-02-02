#ifndef MEMORY_NETWORK
#define MEMORY_NETWORK

#include <Eigen/Dense>
#include <map>
#include <string>
#include <random>

#define NB_INPUT_UNITS 50

typedef Eigen::Matrix<double, NB_INPUT_UNITS, NB_INPUT_UNITS> MemoryMatrix;
typedef Eigen::Matrix<double, NB_INPUT_UNITS, 1> MemoryVector;


const double Eg = 0.8;     // external influence
const double Ig = 0.2;     // internal influence
const double Dg = 0.5;     // activation decay
const double Amax = 1.0;   // maximum activation
const double Amin = -0.2;  // minimum activation
const double Arest = -0.1; // rest activation
const double Lg = 0.01;    // learning rate
const double Winit = 0.0;   // initial weights

class MemoryNetwork
{

public:

    MemoryVector rest_activations;

    std::vector<std::string> units_names;
    MemoryVector external_activations;
    MemoryVector internal_activations;
    MemoryVector net_activations;
    MemoryVector activations;
    MemoryMatrix weights;

    MemoryNetwork();

    void activate_unit(int id, double level);
    void step();

private:
    std::random_device rd;
    std::default_random_engine gen;

    MemoryVector compute_internal_activations();

    void printout();
};


#endif
