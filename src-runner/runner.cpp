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
        cerr << "memory-runner -- Runs experiments on associative memory networks.\n\n";
        cerr << "The results can to piped to a csv file.\n\n" << desc << "\n";
        cerr << "Séverin Lemaignan, Plymouth University 2016, " << endl;
        cerr << "Report bugs to: " << endl;
        cerr << "https://www.github.com/severin-lemaignan/associative-memory/issues" << endl;

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

    MemoryNetwork memory(&logging);
    for (const auto& unit : expe.units) {
        memory.add_unit(unit);
    }

    if (expe.parameters.count("MaxFreq")) {
        memory.max_frequency(expe.parameters["MaxFreq"]);
    }

#define set_param(PARAM) if(expe.parameters.count(PARAM)) {memory.set_parameter(PARAM, expe.parameters[PARAM]);}

    set_param("Dg")
    set_param("Lg")
    set_param("Eg")
    set_param("Ig")
    set_param("Amax")
    set_param("Amin")
    set_param("Arest")
    set_param("Winit")

    cerr << endl << "-------------------------------------------------" << endl;
    cerr <<         "        Running the experiment                   " << endl;
    cerr <<         "-------------------------------------------------" << endl << endl;

    memory.start();

    int last_activation = 0;
    auto start = high_resolution_clock::now();
    
    for (const auto& kv : expe.activations) {
        this_thread::sleep_for(milliseconds(kv.first - last_activation));

        for (auto& activation : kv.second) {
            cerr << " - Activating " << activation.first << " for " << activation.second.count() << "ms" << endl;
            memory.activate_unit(activation.first, 1.0, activation.second);
        }

        last_activation = kv.first;
    }

    auto remaining_time = expe.duration - (high_resolution_clock::now() - start);
    this_thread::sleep_for(remaining_time);

    memory.stop();

    cerr << endl << "EXPERIMENT COMPLETED. Total duration: " << duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start).count() << "ms" << endl;

    cerr << endl << "-------------------------------------------------" << endl;
    cerr <<         "        Preparing experiment's data              " << endl;
    cerr <<         "-------------------------------------------------" << endl << endl;

    auto date = system_clock::to_time_t(system_clock::now());
    tm tm = *std::localtime(&date);

    cout << "# Results for experiment <" << expe.name << "> (run at " << std::put_time(&tm, "%c %Z") << ")." << endl;
    cout << "# Sampling frequency: " << HISTORY_SAMPLING_RATE << "Hz" << endl;
    

    vector<vector<double>> data;
    vector<string> header;

    header.push_back("time");
    for (size_t idx = 0;
                idx < double(duration_cast<std::chrono::milliseconds>(expe.duration).count()) / (1000./HISTORY_SAMPLING_RATE);
                idx++)
    {
        data.push_back(vector<double>());
        data[idx].push_back(idx * 1000./HISTORY_SAMPLING_RATE);

        for (auto& kv : expe.plots) {
            //data[idx].push_back(NAN); // pre-fill with NaN
            data[idx].push_back(0); // pre-fill with zeros
        }
    }

    size_t plot_idx = 0;
    for (auto& kv : expe.plots) {
        header.push_back(kv.first);
        size_t id = memory.unit_id(kv.first);
        cerr << "  - for " << kv.first << ": " << endl;

        for (auto& period : kv.second) {

            cerr << "    - from " << period.start << "ms to " << period.stop << "ms" << endl;


            for (size_t idx = double(period.start) / (1000./HISTORY_SAMPLING_RATE);
                    idx < double(period.stop) / (1000./HISTORY_SAMPLING_RATE);
                    idx++)
            {
                data[idx][plot_idx + 1] = logs[id][idx];
            }
        }

        plot_idx++;
    }

    for(auto h : header) cout << h << ", ";
    cout << endl;

    for(auto row : data) {
        for(auto item : row) cout << item << ", ";
        cout << endl;
    }

    return 0;
}

