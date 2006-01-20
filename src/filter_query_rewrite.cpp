/* $Id: filter_query_rewrite.cpp,v 1.2 2006-01-20 22:38:12 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"

//#include <boost/thread/mutex.hpp>
#include <boost/regex.hpp>

#include "util.hpp"
#include "filter_query_rewrite.hpp"

#include <yaz/zgdu.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class QueryRewrite::Rep {
            //friend class QueryRewrite;
        public:
            void process(yp2::Package &package) const;
        private:
            void rewriteRegex(Z_Query *query) const;
        };
    }
}

yf::QueryRewrite::QueryRewrite() : m_p(new Rep)
{
}

yf::QueryRewrite::~QueryRewrite()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::QueryRewrite::process(yp2::Package &package) const
{
    m_p->process(package);
}

void yf::QueryRewrite::Rep::process(yp2::Package &package) const
{
    if (package.session().is_closed())
    {
        std::cout << "Got Close.\n";
    }
    
    Z_GDU *gdu = package.request().get();
    
    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_initRequest)
    {
        std::cout << "Got Z3950 Init PDU\n";         
        //Z_InitRequest *req = gdu->u.z3950->u.initRequest;
        //package.request() = gdu;
    } 
    else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
             Z_APDU_searchRequest)
    {
        std::cout << "Got Z3950 Search PDU\n";   
        Z_SearchRequest *req = gdu->u.z3950->u.searchRequest;

        // applying regex query rewriting
        rewriteRegex(req->query);
            
        // fold new query structure into gdu package ..       
        // yp2::util::pqf(odr, gdu->u.z3950, query_out);
        // question: which odr structure to use in this call ??
        // memory alignment has to be correct, this is a little tricky ...
        // I'd rather like to alter the gdu and pack it back using:
        package.request() = gdu;
    } 
    else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
             Z_APDU_scanRequest)
    {
        std::cout << "Got Z3950 Scan PDU\n";   
        //Z_ScanRequest *req = gdu->u.z3950->u.scanRequest;
        //package.request() = gdu;
    } 
    package.move();
}


void yf::QueryRewrite::Rep::rewriteRegex(Z_Query *query) const
{
    std::string query_in = yp2::util::zQueryToString(query);
    std::cout << "QUERY IN  '" << query_in << "'\n";

    std::string query_out;
    
    boost::regex rgx;
    try{
        // make regular expression replacement here 
        std::string expression("@attr 1=4");
        std::string format("@attr 1=4 @attr 4=3");
        //std::string expression("the");
        //std::string format("else");
        //std::string expression("(<)|(>)|\\r");
        //std::string format("(?1&lt;)(?2&gt;)");

        std::cout << "EXPRESSION  '" << expression << "'\n";
        std::cout << "FORMAT      '" << format << "'\n";

        rgx.assign(expression.c_str());

        bool match(false);
        bool search(false);

        // other flags
        // see http://www.boost.org/libs/regex/doc/match_flag_type.html
        //boost::match_flag_type flags = boost::match_default;
        // boost::format_default
        // boost::format_perl
        // boost::format_literal
        // boost::format_all
        // boost::format_no_copy
        // boost::format_first_only

        boost::match_flag_type flags 
            = boost::match_default | boost::format_all;

        match = regex_match(query_in, rgx, flags);
        search = regex_search(query_in, rgx, flags);
        query_out = boost::regex_replace(query_in, rgx, format, flags);
        std::cout << "MATCH  '" << match <<  "'\n";
        std::cout << "SEARCH '" << search <<  "'\n";
        std::cout << "QUERY OUT '" << query_out << "'\n";

    }
    catch(boost::regex_error &e)
    {
        std::cout << "REGEX Error code=" << e.code() 
                  << " position=" << e.position() << "\n";
    }
    
    //std::cout << "QUERY OUT '" << query_out << "'\n";
    // still need to fold this new rpn query string into Z_Query structure...
}



static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::QueryRewrite;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_query_rewrite = {
        0,
        "query-rewrite",
        filter_creator
    };
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_query_rewrite;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
