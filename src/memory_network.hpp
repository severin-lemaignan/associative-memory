#ifndef MEMORY_NETWORK
#define MEMORY_NETWORK

#include <Eigen/Dense>
#include <map>
#include <string>

#define NB_INPUT_UNITS 10

typedef Eigen::Matrix<double, NB_INPUT_UNITS, NB_INPUT_UNITS> MemoryMatrix;
typedef Eigen::Matrix<double, NB_INPUT_UNITS, 1> MemoryVector;


const double Eg = 0.6;     // external influence
const double Ig = 0.3;     // internal influence
const double Dg = 0.2;     // activation decay
const double Amax = 1.0;   // maximum activation
const double Amin = -0.2;  // minimum activation
const double Arest = -0.1; // rest activation
const double Lg = 0.01;    // learning rate


class MemoryNetwork
{

public:
    std::vector<std::string> units_names;
    MemoryVector external_activations;
    MemoryVector activations;
    MemoryMatrix weights;

    MemoryNetwork();

    void step();

private:
    MemoryVector internal_activations();
};


#endif
