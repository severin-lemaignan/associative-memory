#include <iostream>
#include <iomanip>

#include "memory_network.hpp"

using namespace Eigen;
using namespace std;

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
        units_names.push_back(string("input") + to_string(i));
    }

    rest_activations.fill(Arest);

    external_activations.fill(0);
    internal_activations.fill(0);
    net_activations.fill(0);

    activations.fill(Arest);
    weights.fill(NAN);
}

MemoryVector MemoryNetwork::compute_internal_activations() {

    for (size_t i = 0; i < activations.rows(); i++)
    {
        double sum = 0;
        for (size_t j = 0; j < activations.rows(); j++)
        {
            if (std::isnan(weights(i,j))) continue;
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

    // Establish connections
    // *********************
    for (size_t i = 0; i < external_activations.size() - 1; i++) {

        if (external_activations(i) == 0) continue;

        for (size_t j = i + 1; j < external_activations.size(); j++) {

            if (external_activations(j) == 0) continue;

            if (std::isnan(weights(i,j))) {
                    weights(i,j) = weights(j,i) = Winit;
            }
        }
    }

    compute_internal_activations();

    net_activations = Eg * external_activations + Ig * internal_activations;

    // Activations update
    // ******************
    for (size_t i = 0; i < net_activations.rows(); i++)
    {

        if (net_activations(i) > 0)
            activations(i) +=  net_activations(i) * (Amax - activations(i));
        else
            activations(i) +=  net_activations(i) * (activations(i) - Amin);


        // clamp in [Amin, Amax]
        activations(i) = min(Amax, max(Amin, activations(i)));
    }
    
    // decay
    activations -= Dg * (activations - rest_activations);

    // Weights update
    // **************
    for (size_t i = 0; i < activations.rows(); i++)
    {
        for (size_t j = 0; j < activations.rows(); j++)
        {
            if (std::isnan(weights(i,j))) continue;

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


    //printout();
    
    // decay the external activations
    external_activations *= 0.99;

}

void MemoryNetwork::printout() {

    cout << "Weights" << endl << setprecision(2) << weights << endl;

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
