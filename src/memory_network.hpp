#ifndef MEMORY_NETWORK
#define MEMORY_NETWORK

#include <Eigen/Dense>
#include <map>
#include <string>
#include <random>

#define NB_INPUT_UNITS 50

typedef Eigen::Matrix<double, NB_INPUT_UNITS, NB_INPUT_UNITS> MemoryMatrix;
typedef Eigen::Matrix<double, NB_INPUT_UNITS, 1> MemoryVector;


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

    MemoryNetwork(double Eg = 0.6,     // external influence
                  double Ig = 0.3,     // internal influence
                  double Dg = 0.5,     // activation decay
                  double Amax = 1.0,   // maximum activation
                  double Amin = -0.2,  // minimum activation
                  double Arest = -0.1, // rest activation
                  double Lg = 0.01,    // learning rate
                  double Winit = 0.0); // initial weights


    void activate_unit(int id, double level);
    void step();

private:
    const double Eg;
    const double Ig;
    const double Dg;
    const double Amax;
    const double Amin;
    const double Arest;
    const double Lg;
    const double Winit;

    std::random_device rd;
    std::default_random_engine gen;

    MemoryVector compute_internal_activations();

    void printout();
};


#endif
