#ifndef _RUNNER_EXPE
#define _RUNNER_EXPE

#include <string>
#include <set>
#include <map>
#include <vector>
#include <tuple>

struct timeperiod
{
    int start;
    int stop;
};

struct Experiment
{

    std::set<std::string> units;
    std::map<int, std::vector<std::string>> activations;
    std::vector<std::tuple<std::string, int, int>> plots;

    std::string name;
    int endtime = 0;

    Experiment();

    void set_name(const std::string& _name);

    void add_unit(std::string& unit);

    void add_activation(const std::string& unit, const timeperiod& period);
   
    void add_plot(const std::string& unit, const timeperiod& period);

    void summary() const;
};

#endif
