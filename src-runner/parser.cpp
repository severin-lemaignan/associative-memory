
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
#include <string>
#include <set>
#include <map>
#include <tuple>
#include <utility>
#include <algorithm> // for copy

using namespace std;


struct timeperiod
{
    int start;
    int stop;
};

struct Experiment
{

    set<string> units;
    map<int, vector<string>> activations;
    vector<tuple<string, int, int>> plots;

    int endtime = 0;

    Experiment() {
        activations[0] = {};
    }

    void add_unit(string& unit) {
        units.insert(unit);
    }

    void add_activation(const string& unit, const timeperiod& period) {

        endtime = max(endtime, period.stop);

        vector<string> active_units_before_start;
        vector<string> active_units_before_stop;

        for (auto& kv : activations) {
            auto t = kv.first;
            if (t > period.start) break;

            active_units_before_start = kv.second;
        }

        // insert a point when the unit starts to activate
        activations[period.start] = active_units_before_start;
        activations[period.start].push_back(unit);

        for (auto& kv : activations) {
            auto t = kv.first;
            if (t > period.stop) break;
            active_units_before_stop = kv.second;

            if (t <= period.start) continue;

            // keep it actives at all time < stop
            if (t < period.stop)  {
                kv.second.push_back(unit);
            }
        }

        // if needed, add a point at stop time, and make sure the unit is not active
        if (activations.count(period.stop) == 0) {
            active_units_before_stop.erase(std::remove(active_units_before_stop.begin(), active_units_before_stop.end(), unit), active_units_before_stop.end());
            activations[period.stop] = active_units_before_stop;
        }
    }

    void add_plot(const string& unit, const timeperiod& period) {

        endtime = max(endtime, period.stop);

        plots.push_back(make_tuple(unit, period.start, period.stop));

    }

    void summary() const
    {

        cout << "Summary of the experiment" << endl;
        cout << "=========================" << endl << endl;

        cout << units.size() << " defined units:" << endl;
        for (auto unit : units) {
            cout << unit << endl;
        }

        cout << endl << "Activations plan:" << endl;
        for (auto& kv : activations) {
            cout << "  - at " << kv.first << "ms: ";
            if (kv.second.empty()) cout << "none";
            else
                copy(kv.second.begin(), kv.second.end(), std::ostream_iterator<string>(std::cout, ", "));

            cout << endl;
        }

        cout << endl << "Requested plots:" << endl;
        for (auto& plot : plots) {
            string unit;
            int start, end;

            tie(unit, start, end) = plot;
            cout << "Unit <" << unit << "> from " << start << "ms to " << end << "ms" << endl;

        }

        cout << endl << "Total duration: " << endtime << "ms" << endl;
    }
};

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;
namespace ascii = boost::spirit::ascii;

BOOST_FUSION_ADAPT_STRUCT(
        timeperiod,
        (int, start)
        (int, stop)
)

template <typename Iterator>
struct experiment_grammar : qi::grammar<Iterator, qi::locals<std::string>>
{
    experiment_grammar() : experiment_grammar::base_type(start)
    {
        using qi::eol;
        using qi::omit;
        using qi::lit;
        using qi::_a;
        //using qi::_1;
        using qi::_val;
        using qi::int_;
        using ascii::char_;
        using ascii::space;
        using qi::lexeme;

        double_ruler = lexeme[+(char_('='))] >> eol;
        simple_ruler = lexeme[+(char_('-'))] >> eol;

        text %= lexeme[+(char_ - eol - char_(':'))];

        listitem %= omit[   char_('-') 
                         >> space] 
                 >> text 
                 >> omit[ -char_(':')]
                 >> eol;

        perioditem %= omit[   *space
                           >> char_('-')
                           >> space 
                           >> '[' ]
                   >> int_ 
                   >> ',' 
                   >> omit[*space]
                   >> int_ 
                   >> ']' 
                   >> eol;

        title1 %= text >> eol >> double_ruler;
        title2 %= text >> eol >> simple_ruler;

        unitstitle = lit("Units") >> eol 
                  >> simple_ruler;

        activationstitle = lit("Activations") >> eol 
                        >> simple_ruler;

        plotstitle = lit("Plots") >> eol 
                  >> simple_ruler;

        start = title1 >> *eol 
             >> text >> *eol 

             >> unitstitle >> *eol
             >> *(listitem[boost::bind(&Experiment::add_unit, &expe, _1)]) 

             >> *eol
             >> activationstitle >> *eol 
             >> *(
                        listitem[_a = qi::_1]
                     >> +(perioditem[phx::bind(&Experiment::add_activation, &expe, qi::_a, qi::_1)])
                     >> *eol
                )

             >> *eol
             >> plotstitle >> *eol
             >> *(
                        listitem[_a = qi::_1]
                     >> +(perioditem[phx::bind(&Experiment::add_plot, &expe, qi::_a, qi::_1)])
                     >> *eol
                )

                ;

        //BOOST_SPIRIT_DEBUG_NODE(start);
    }

    Experiment expe;
    qi::rule<Iterator,qi::locals<std::string>> start;
    qi::rule<Iterator> double_ruler;
    qi::rule<Iterator> simple_ruler;
    qi::rule<Iterator, string()> text;
    qi::rule<Iterator, string()> listitem;
    qi::rule<Iterator, timeperiod()> perioditem;
    qi::rule<Iterator> title1;
    qi::rule<Iterator> title2;
    qi::rule<Iterator> unitstitle;
    qi::rule<Iterator> activationstitle;
    qi::rule<Iterator> plotstitle;
};


int main(int argc, char** argv) {

    ifstream experiment(argv[1]);


    string str((istreambuf_iterator<char>(experiment)),
                istreambuf_iterator<char>());

    string::const_iterator iter = str.begin();
    string::const_iterator end = str.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser,ascii::space);

//
//    if (r && iter == str.end()) {
//        cout << "-------------------------\n";
//        cout << "Parsing succeeded\n";
//        cout << str << " Parses OK: " << endl;
//    } else {
//        cout << "-------------------------\n";
//        cout << str << ": Parsing failed\n";
//        cout << "-------------------------\n";
//    }
//
    experiment_parser.expe.summary();

    return 0;
}
