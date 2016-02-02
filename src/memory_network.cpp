#include <iostream>
#include <iomanip>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;


MemoryNetwork::MemoryNetwork():
    gen(rd())
{
    uniform_real_distribution<> dis(-0.2, 0.2);

    for (size_t i = 0; i < NB_INPUT_UNITS; i++) {
        units_names.push_back(string("input") + to_string(i));
        external_activations(i) = dis(gen);
        //external_activations(i) = 0.1;
    }

    rest_activations.fill(Arest);

    internal_activations.fill(0);
    net_activations.fill(0);

    activations.fill(Arest);
    weights.fill(Winit);
}

MemoryVector MemoryNetwork::compute_internal_activations() {

    for (size_t i = 0; i < activations.rows(); i++)
    {
        double sum = 0;
        for (size_t j = 0; j < activations.rows(); j++)
        {
            sum += weights(i, j) * activations(j);
        }
        internal_activations(i) = sum;
    }
}

void MemoryNetwork::activate_unit(int id, double level) {
    external_activations(id) = level;
}

void MemoryNetwork::step()
{

    //uniform_int_distribution<> dist(0, NB_INPUT_UNITS-1);

    //external_activations(dist(gen)) = 1.;

    compute_internal_activations();

    net_activations = Eg * external_activations + Ig * internal_activations;

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

        // clamp in [Amin, Amax]
        activations(i) = min(Amax, max(Amin, activations(i)));
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


    printout();
    cout << "Weights" << endl << weights << std::endl;
    
    // decay the external activations
    external_activations += 0.01 * (-external_activations + rest_activations);

}

void MemoryNetwork::printout() {

    cout << "External\tInternal\tNet\t\tActivation" << endl;
    cout << "--------\t--------\t---\t\t----------" << endl;
    cout << setprecision(4) << setw(6) << fixed;
    for (size_t i = 0; i < NB_INPUT_UNITS; i++) {
        cout << external_activations(i) << "\t\t";
        cout << internal_activations(i) << "\t\t";
        cout << net_activations(i) << "\t\t";
        cout << activations(i) << endl;

    }
    cout << endl;
}
