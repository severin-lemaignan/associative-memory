#ifndef _RUNNER_PARSER
#define _RUNNER_PARSER

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


#include <boost/bind.hpp>

#include <iostream>
#include <string>

#include "experiment.hpp"

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;
namespace ascii = boost::spirit::ascii;

BOOST_FUSION_ADAPT_STRUCT(
        timeperiod,
        (long int, start)
        (long int, stop)
)

BOOST_FUSION_ADAPT_STRUCT(
        activationperiod,
        (long int, start)
        (long int, stop)
        (float, level)
)

BOOST_FUSION_ADAPT_STRUCT(
        parameter,
        (std::string, name)
        (double, value)
)


template <typename Iterator>
struct experiment_grammar : qi::grammar<Iterator, qi::locals<std::string>>
{
    experiment_grammar() : experiment_grammar::base_type(start, "memoryexperiment")
    {
        using qi::eol;
        using qi::omit;
        using qi::lit;
        using qi::_a;
        using qi::long_;
        using qi::double_;
        using ascii::char_;
        using ascii::space;
        using qi::lexeme;

        // error handling
        using qi::on_error;
        using qi::fail;
        using phx::construct;
        using phx::val;


        double_ruler = lexeme[+(char_('='))] >> eol;
        double_ruler.name("double_ruler");
        simple_ruler = lexeme[+(char_('-'))] >> eol;
        simple_ruler.name("simple_ruler");

        text %= lexeme[+(char_ - eol)];
        text.name("text");
        identifier %= lexeme[+(char_ - eol - char_(':'))];
        identifier.name("identifier");

        listitem %= omit[   char_('-') 
                         >> space] 
                 >> identifier 
                 >> omit[ -char_(':')]
                 >> eol;
        listitem.name("list");

        paramitem %= omit[   *space 
                          >> char_('-') 
                          >> space] 
                 >> identifier
                 >> omit[   -char_(':')
                         >> *space]
                 >> double_
                 >> omit[ -(  +space
                            >> text)]
                 >> eol;
        paramitem.name("parameter");

        perioditem %= omit[   *space
                           >> char_('-')
                           >> space 
                           >> '[' ]
                   >> long_ 
                   >> ',' 
                   >> omit[*space]
                   >> long_ 
                   >> ']'
                   >> eol;
        perioditem.name("time interval");

        activationitem %= omit[   *space
                           >> char_('-')
                           >> space 
                           >> '[' ]
                   >> long_ 
                   >> ',' 
                   >> omit[*space]
                   >> long_ 
                   >> ']' >> omit[*space] >> lit("at") >> omit[*space]
                   >> double_
                   >> eol;
        activationitem.name("activation interval");

        title1 %= text >> omit[eol >> double_ruler];
        title1.name("title1");
        title2 %= text >> omit[eol >> simple_ruler];
        title2.name("title2");

        paramstitle = lit("Network Parameters") >> eol 
                  >> simple_ruler;
        paramstitle.name("parameter section title");

        unitstitle = lit("Units") >> eol 
                  >> simple_ruler;
        unitstitle.name("units section title");

        activationstitle = lit("Activations") >> eol 
                        >> simple_ruler;
        activationstitle.name("activations section title");

        plotstitle = lit("Plots") >> eol 
                  >> simple_ruler;
        plotstitle.name("plots section title");

        start = title1[boost::bind(&Experiment::set_name, &expe, _1)] >> *eol 
             >> *text >> *eol

             >> paramstitle >> *eol
             >> *(paramitem[boost::bind(&Experiment::store_param, &expe, _1)]) 

             >> *eol
             >> unitstitle >> *eol
             >> *(listitem[boost::bind(&Experiment::add_unit, &expe, _1)]) 

             >> *eol
             >> activationstitle >> *eol 
             >> *(
                        listitem[_a = qi::_1]
                     >> +(activationitem[phx::bind(&Experiment::add_activation, &expe, qi::_a, qi::_1)])
                     >> *eol
                )

             >> *eol
             >> *plotstitle >> *eol
             >> *(
                        listitem[_a = qi::_1]
                     >> +(perioditem[phx::bind(&Experiment::add_plot, &expe, qi::_a, qi::_1)])
                     >> *eol
                )

                ;
        start.name("experiment");


        // TODO: never called for some reason...
        on_error<fail>
        (
            start
          , std::cout
                << val("Error! Expecting ")
             //   << _4                               // what failed?
             //   << val(" here: \"")
             //   << construct<std::string>(_3, _2)   // iterators to error-pos, end
             //   << val("\"")
                << std::endl
        );

        //debug(start);
        //debug(title1);
        //debug(paramstitle);
        //debug(paramitem);
        //debug(paramitem);
    }

    Experiment expe;
    qi::rule<Iterator,qi::locals<std::string>> start;
    qi::rule<Iterator> double_ruler;
    qi::rule<Iterator> simple_ruler;
    qi::rule<Iterator, std::string()> text;
    qi::rule<Iterator, std::string()> identifier;
    qi::rule<Iterator, std::string()> listitem;
    qi::rule<Iterator, parameter()> paramitem;
    qi::rule<Iterator, timeperiod()> perioditem;
    qi::rule<Iterator, activationperiod()> activationitem;
    qi::rule<Iterator, std::string()> title1;
    qi::rule<Iterator, std::string()> title2;
    qi::rule<Iterator> paramstitle;
    qi::rule<Iterator> unitstitle;
    qi::rule<Iterator> activationstitle;
    qi::rule<Iterator> plotstitle;
};


#endif
