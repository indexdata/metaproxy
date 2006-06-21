/* $Id: filter_session_shared.hpp,v 1.8 2006-06-21 09:16:54 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef FILTER_SESSION_SHARED_HPP
#define FILTER_SESSION_SHARED_HPP

#include <boost/scoped_ptr.hpp>
#include <list>
#include <map>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class SessionShared : public Base {
            class Rep;
            class InitKey;
            class BackendSet;
            class FrontendSet;
            class Worker;

            struct Frontend;
            class BackendClass;
            class BackendInstance;
            typedef boost::shared_ptr<Frontend> FrontendPtr;
            typedef boost::shared_ptr<BackendClass> BackendClassPtr;
            typedef boost::shared_ptr<BackendInstance> BackendInstancePtr;
            typedef boost::shared_ptr<BackendSet> BackendSetPtr;
            typedef boost::shared_ptr<FrontendSet> FrontendSetPtr;
            typedef std::list<std::string> Databases;

            typedef std::list<BackendInstancePtr> BackendInstanceList;
            typedef std::map<InitKey, BackendClassPtr> BackendClassMap;
            typedef std::list<BackendSetPtr> BackendSetList;
            typedef std::map<std::string, FrontendSetPtr> FrontendSets;
        public:
            ~SessionShared();
            SessionShared();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_session_shared;
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
