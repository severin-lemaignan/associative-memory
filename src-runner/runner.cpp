#include <boost/program_options.hpp>

#include <chrono>

#include <iostream>
#include <iomanip>
#include <fstream>

#include "memory_network.hpp"

#include "parser.hpp"

const int HISTORY_SAMPLING_RATE=500; //Hz

using namespace std;
using namespace std::chrono;
namespace po = boost::program_options;

map<size_t, vector<double>> logs;

microseconds _last_log;

void logging(microseconds time_from_start,
             const MemoryVector& levels)
{

    // if necessary, store the activation level
    auto us_since_last_log = time_from_start - _last_log;
    if(us_since_last_log.count() > (1000000./HISTORY_SAMPLING_RATE)) {
        _last_log = time_from_start;
        for(size_t i = 0; i < levels.size(); i++) {
            logs[i].push_back(levels[i]);
        }
    }


}

int main(int argc, char *argv[]) {

    po::positional_options_description p;
    p.add("configuration", 1);

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("configuration", po::value<string>(), "Description of the experiment (markdown)")
            ;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                        .options(desc)
                        .positional(p)
                        .run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << "memory-runner -- Runs experiments on associative memory networks\n\n" << desc << "\n";
        cout << "Séverin Lemaignan, Plymouth University 2016, " << endl;
        cout << "Report bugs to: " << endl;
        cout << "https://www.github.com/severin-lemaignan/associative-memory/issues" << endl;

        return 1;
    }

    if (!vm.count("configuration")) {
        cerr << "You must provide an experiment configuration." << endl;
        return 1;
    }

    auto conf = vm["configuration"].as<string>();

    ifstream experiment(conf);


    string str((istreambuf_iterator<char>(experiment)),
                istreambuf_iterator<char>());

    string::const_iterator iter = str.begin();
    string::const_iterator end = str.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser,ascii::space);


    if (r && iter == str.end()) {
        experiment_parser.expe.summary();
    } else {
        cerr << "Parsing of " << conf << " failed!";
        return 1;
    }

    cerr << "-------------------------------------------------" << endl;
    cerr << "        Configuring the memory network           " << endl;
    cerr << "-------------------------------------------------" << endl << endl;
    auto& expe = experiment_parser.expe;

    MemoryNetwork memory(expe.units.size(), &logging);
    memory.units_names(expe.units);

    if (expe.parameters.count("MaxFreq")) {
        cout << "Setting the max frequency to " << expe.parameters["MaxFreq"] << endl;
        memory.max_frequency(expe.parameters["MaxFreq"]);
    }

    cout << endl << "-------------------------------------------------" << endl;
    cout <<         "        Running the experiment                   " << endl;
    cout <<         "-------------------------------------------------" << endl << endl;

    memory.start();

    int last_activation = 0;
    auto start = high_resolution_clock::now();
    
    for (const auto& kv : expe.activations) {
        this_thread::sleep_for(milliseconds(kv.first - last_activation));

        for (auto& activation : kv.second) {
            cout << " - Activating " << activation.first << " for " << activation.second.count() << "ms" << endl;
            memory.activate_unit(activation.first, 1.0, activation.second);
        }

        last_activation = kv.first;
    }

    auto remaining_time = expe.duration - (high_resolution_clock::now() - start);
    this_thread::sleep_for(remaining_time);

    memory.stop();

    cout << "Experiment completed. Total duration: " << duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start).count() << "ms" << endl;
    cout << "-------------------------------------------------" << endl << endl;





    return 0;
}

