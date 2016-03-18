
#define BOOST_SPIRIT_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


#include <boost/bind.hpp>

#include <iostream>
#include <fstream>
#include <streambuf>

#include "parser.hpp"

using namespace std;


int main(int argc, char** argv) {

    ifstream experiment(argv[1]);


    string str((istreambuf_iterator<char>(experiment)),
                istreambuf_iterator<char>());

    string::const_iterator iter = str.begin();
    string::const_iterator end = str.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser,ascii::space);


    if (r && iter == str.end()) {
        experiment_parser.expe.summary();
    } else {
        cout << "Parsing of " << argv[1] << " failed!";
    }


    return 0;
}
