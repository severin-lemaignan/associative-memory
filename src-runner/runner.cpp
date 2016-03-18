#include <boost/program_options.hpp>

#include <iostream>
#include <fstream>

#include "parser.hpp"

using namespace std;
namespace po = boost::program_options;

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

}

