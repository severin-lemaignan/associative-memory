#ifndef _RUNNER_PARSER
#define _RUNNER_PARSER

//#define BOOST_SPIRIT_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


#include <boost/bind.hpp>

#include <string>

#include "experiment.hpp"

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

        title1 %= text >> omit[eol >> double_ruler];
        title2 %= text >> omit[eol >> simple_ruler];

        unitstitle = lit("Units") >> eol 
                  >> simple_ruler;

        activationstitle = lit("Activations") >> eol 
                        >> simple_ruler;

        plotstitle = lit("Plots") >> eol 
                  >> simple_ruler;

        start = title1[boost::bind(&Experiment::set_name, &expe, _1)] >> *eol 
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
    qi::rule<Iterator, std::string()> text;
    qi::rule<Iterator, std::string()> listitem;
    qi::rule<Iterator, timeperiod()> perioditem;
    qi::rule<Iterator, std::string()> title1;
    qi::rule<Iterator, std::string()> title2;
    qi::rule<Iterator> unitstitle;
    qi::rule<Iterator> activationstitle;
    qi::rule<Iterator> plotstitle;
};


#endif
