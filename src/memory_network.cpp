#include <iostream>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;


MemoryNetwork::MemoryNetwork() :
    external_activations({
            {"input0", 0.2},
            {"input1", -0.1},
            {"input2", 0.3},
            {"input3", 0.6},
            {"input4", 0.6},
            {"input5", 0.1},
            {"input6", -0.2},
            {"input7", 0.4},
            {"input8", -0.1},
            {"input9", 0.}
        })
{
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
    MemoryVector ext_activations;

    size_t i = 0;
    for(auto& vals: external_activations)
    {
        ext_activations(i) = vals.second;
        i++;
    }

    auto net_activations = Eg * ext_activations + Ig * internal_activations();


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


    cout << "Activations" << endl << activations << endl;
    cout << "Weights" << endl << weights << std::endl;

}
