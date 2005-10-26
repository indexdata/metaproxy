/* $Id: router_chain.cpp,v 1.1 2005-10-26 10:55:26 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "router_chain.hpp"


const yp2::filter::Base * yp2::RouterChain::move(const filter::Base *filter,                                   const Package *package) const {
            std::list<const filter::Base *>::const_iterator it;
            it = m_filter_list.begin();
            if (filter)
                {
                    for (; it != m_filter_list.end(); it++)
                        if (*it == filter)
                            {
                                it++;
                                break;
                            }
                }
            if (it == m_filter_list.end())
                {
                    //throw RouterException("no routing rules known");
                    return 0;
                }
            return *it;
        };

        yp2::RouterChain & yp2::RouterChain::append(const filter::Base &filter){
            m_filter_list.push_back(&filter);
            return *this;
        };




/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
