#ifndef _RUNNER_EXPE
#define _RUNNER_EXPE

#include <string>
#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <chrono>

struct timeperiod
{
    int start;
    int stop;
};

struct parameter
{
    std::string name;
    double value;
};

struct Experiment
{
    typedef std::pair<std::string, std::chrono::milliseconds> Activation;

    std::map<std::string, double> parameters;
    std::set<std::string> units;
    std::map<int, std::vector<Activation>> activations;
    std::vector<std::tuple<std::string, int, int>> plots;

    std::string name;
    int endtime = 0;

    Experiment();

    void set_name(const std::string& _name);

    void store_param(parameter& param);

    void add_unit(std::string& unit);

    void add_activation(const std::string& unit, const timeperiod& period);
   
    void add_plot(const std::string& unit, const timeperiod& period);

    void summary() const;
};

#endif
