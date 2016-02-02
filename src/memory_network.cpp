#include <iostream>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;


MemoryNetwork::MemoryNetwork() :
    units_names({
            "input0",
            "input1",
            "input2",
            "input3",
            "input4",
            "input5",
            "input6",
            "input7",
            "input8",
            "input9"})
{
    external_activations <<
            0.2,
            -0.1,
            0.3,
            0.6,
            0.6,
            0.1,
            -0.2,
            0.4,
            -0.1,
            0.;
    activations.fill(0);
    weights.fill(0);
}

MemoryVector MemoryNetwork::internal_activations() {

    MemoryVector internal_activation;

    for (size_t i = 0; i < activations.rows(); i++)
    {
        double sum = 0;
        for (size_t j = 0; j < activations.rows(); j++)
        {
            sum += weights(i, j) * activations(j);
        }
        internal_activation(i) = sum;
    }

    return internal_activation;
}

void MemoryNetwork::step()
{

    auto net_activations = Eg * external_activations + Ig * internal_activations();


    // Activations update
    // ******************
    for (size_t i = 0; i < net_activations.rows(); i++)
    {

        if (net_activations(i) > 0)
        {
        activations(i) +=  net_activations(i) * (Amax - activations(i)) 
                         - Dg * (activations(i) - Arest);
        }
        else
        {
        activations(i) +=  net_activations(i) * (activations(i) - Amin) 
                         - Dg * (activations(i) - Arest);
        }
    }

    // Weights update
    // **************
    for (size_t i = 0; i < activations.rows(); i++)
    {
        for (size_t j = 0; j < activations.rows(); j++)
        {
            if (activations(i) * activations(j) > 0)
            {
            weights(i,j) += Lg * activations(i) * activations(j) * (1 - weights(i,j));
            }
            else
            {
            weights(i,j) += Lg * activations(i) * activations(j) * (1 + weights(i,j));
            }
        }
    }


    //cout << "Activations" << endl << activations << endl;
    //cout << "Weights" << endl << weights << std::endl;

}
