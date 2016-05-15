/* This file is part of Metaproxy.
   Copyright (C) Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/filter.hpp>
#include "filter_load_balance.hpp"
#include <metaproxy/util.hpp>


#include <boost/thread/mutex.hpp>

#include <yaz/diagbib1.h>
#include <yaz/log.h>
#include <yaz/zgdu.h>

// remove max macro if already defined (defined later in <limits>)
#ifdef max
#undef max
#endif

#include <list>
#include <map>
#include <limits>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1
{
    namespace filter
    {
        class LoadBalance::Impl
        {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
        private:
            // statistic manipulating functions,
            void add_dead(unsigned long session_id);
            //void clear_dead(unsigned long session_id);
            void add_package(unsigned long session_id);
            void remove_package(unsigned long session_id);
            void add_session(unsigned long session_id, std::string target);
            void remove_session(unsigned long session_id);
            std::string find_session_target(unsigned long session_id);

            // cost functions
            unsigned int cost(std::string target);
            unsigned int dead(std::string target);

            // local classes
            class TargetStat {
            public:
                unsigned int sessions;
                unsigned int packages;
                unsigned int deads;
                unsigned int cost() {
                    unsigned int c = sessions + packages + deads;
                    return c;
                }
            };

            // local protected databases
            boost::mutex m_mutex;
            std::map<std::string, TargetStat> m_target_stat;
            std::map<unsigned long, std::string> m_session_target;
        };
    }
}

// define Pimpl wrapper forwarding to Impl

yf::LoadBalance::LoadBalance() : m_p(new Impl)
{
}

yf::LoadBalance::~LoadBalance()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::LoadBalance::configure(const xmlNode *xmlnode, bool test_only,
                                const char *path)
{
    m_p->configure(xmlnode);
}

void yf::LoadBalance::process(mp::Package &package) const
{
    m_p->process(package);
}


yf::LoadBalance::Impl::Impl()
{
}

yf::LoadBalance::Impl::~Impl()
{
}

void yf::LoadBalance::Impl::configure(const xmlNode *xmlnode)
{
}

void yf::LoadBalance::Impl::process(mp::Package &package)
{
    bool is_closed_front = false;

    // checking for closed front end packages
    if (package.session().is_closed())
    {
        is_closed_front = true;
    }

    Z_GDU *gdu_req = package.request().get();

    // passing anything but z3950 packages
    if (gdu_req && gdu_req->which == Z_GDU_Z3950)
    {
        // target selecting only on Z39.50 init request
        if (gdu_req->u.z3950->which == Z_APDU_initRequest)
        {
            yazpp_1::GDU base_req(gdu_req);
            Z_APDU *apdu = base_req.get()->u.z3950;

            Z_InitRequest *org_init = base_req.get()->u.z3950->u.initRequest;
            mp::odr odr_en(ODR_ENCODE);

            std::list<std::string> vhosts;
            mp::util::remove_vhost_otherinfo(&(org_init->otherInfo), vhosts);
            // get lowest of all vhosts.. Remove them if individually if
            // they turn out to be bad..
            while (1)
            {
                std::list<std::string>::iterator ivh = vhosts.begin();
                std::list<std::string>::iterator ivh_pick = vhosts.end();

                Package init_pkg(package.session(), package.origin());
                init_pkg.copy_filter(package);

                unsigned int cost_i = std::numeric_limits<unsigned int>::max();
                {
                    boost::mutex::scoped_lock scoped_lock(m_mutex);

                    for (; ivh != vhosts.end(); ivh++)
                    {
                        if ((*ivh).size() != 0)
                        {
                            unsigned int cost
                                = yf::LoadBalance::Impl::cost(*ivh);

                            std::ostringstream os;
                            os  << "LB" << " "
                                << package << " "
                                << "0.000000" << " "
                                << "Consider " << *ivh
                                << " cost=" << cost;
                            yaz_log(YLOG_LOG, "%s", os.str().c_str());
                            if (cost_i > cost)
                            {
                                ivh_pick = ivh;
                                cost_i = cost;
                            }
                        }
                    }
                }
                if (ivh_pick == vhosts.end())
                    break;
                std::string target = *ivh_pick;
                vhosts.erase(ivh_pick);
                // copying new target into init package
                yazpp_1::GDU init_gdu(base_req);
                Z_InitRequest *init_req = init_gdu.get()->u.z3950->u.initRequest;

                mp::util::set_vhost_otherinfo(&(init_req->otherInfo),
                                              odr_en, target, 1);

                init_pkg.request() = init_gdu;

                // moving all package types
                init_pkg.move();

                // checking for closed back end packages
                if (!init_pkg.session().is_closed())
                {
                    add_session(package.session().id(), target);

                    package.response() = init_pkg.response();
                    return;
                }
                std::ostringstream os;
                os  << "LB" << " "
                    << package << " "
                    << "0.000000" << " "
                    << "Failed " << target;
                yaz_log(YLOG_LOG, "%s", os.str().c_str());
            }
            mp::odr odr;
            package.response() = odr.create_initResponse(
                apdu, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                "load_balance: no available targets");
            package.session().close();
            return;
        }
        // frontend Z39.50 close request is added to statistics and marked
        else if (gdu_req->u.z3950->which == Z_APDU_close)
        {
            is_closed_front = true;
            boost::mutex::scoped_lock scoped_lock(m_mutex);
            add_package(package.session().id());
        }
        // any other Z39.50 package is added to statistics
        else
        {
            boost::mutex::scoped_lock scoped_lock(m_mutex);
            add_package(package.session().id());
        }
    }

    // moving all package types
    package.move();

    bool is_closed_back = false;

    // checking for closed back end packages
    if (package.session().is_closed())
        is_closed_back = true;

    Z_GDU *gdu_res = package.response().get();

    // passing anything but z3950 packages
    if (gdu_res && gdu_res->which == Z_GDU_Z3950)
    {
        // session closing only on Z39.50 close response
        if (gdu_res->u.z3950->which == Z_APDU_close)
        {
            is_closed_back = true;
            boost::mutex::scoped_lock scoped_lock(m_mutex);
            remove_package(package.session().id());
        }
        // any other Z39.50 package is removed from statistics
        else
        {
            boost::mutex::scoped_lock scoped_lock(m_mutex);
            remove_package(package.session().id());
        }
    }

    // finally removing sessions and marking deads
    if (is_closed_back || is_closed_front)
    {
        boost::mutex::scoped_lock scoped_lock(m_mutex);

        // marking backend dead if backend closed without fronted close
        if (is_closed_front == false)
            add_dead(package.session().id());

        remove_session(package.session().id());

        // making sure that package is closed
        package.session().close();
    }
}

// statistic manipulating functions,
void yf::LoadBalance::Impl::add_dead(unsigned long session_id)
{
    std::string target = find_session_target(session_id);

    if (target.size() != 0)
    {
        std::map<std::string, TargetStat>::iterator itarg;
        itarg = m_target_stat.find(target);
        if (itarg != m_target_stat.end()
            && itarg->second.deads < std::numeric_limits<unsigned int>::max())
        {
            itarg->second.deads += 1;
            // std:.cout << "add_dead " << session_id << " " << target
            //          << " d:" << itarg->second.deads << "\n";
        }
    }
}

void yf::LoadBalance::Impl::add_package(unsigned long session_id)
{
    std::string target = find_session_target(session_id);

    if (target.size() != 0)
    {
        std::map<std::string, TargetStat>::iterator itarg;
        itarg = m_target_stat.find(target);
        if (itarg != m_target_stat.end()
            && itarg->second.packages
               < std::numeric_limits<unsigned int>::max())
        {
            itarg->second.packages += 1;
        }
    }
}

void yf::LoadBalance::Impl::remove_package(unsigned long session_id)
{
    std::string target = find_session_target(session_id);

    if (target.size() != 0)
    {
        std::map<std::string, TargetStat>::iterator itarg;
        itarg = m_target_stat.find(target);
        if (itarg != m_target_stat.end()
            && itarg->second.packages > 0)
        {
            itarg->second.packages -= 1;
        }
    }
}

void yf::LoadBalance::Impl::add_session(unsigned long session_id,
                                        std::string target)
{
    // finding and adding session
    std::map<unsigned long, std::string>::iterator isess;
    isess = m_session_target.find(session_id);
    if (isess == m_session_target.end())
    {
        m_session_target.insert(std::make_pair(session_id, target));
    }

    // finding and adding target statistics
    std::map<std::string, TargetStat>::iterator itarg;
    itarg = m_target_stat.find(target);
    if (itarg == m_target_stat.end())
    {
        TargetStat stat;
        stat.sessions = 1;
        stat.packages = 0;
        stat.deads = 0;
        m_target_stat.insert(std::make_pair(target, stat));
    }
    else if (itarg->second.sessions < std::numeric_limits<unsigned int>::max())
    {
        itarg->second.sessions += 1;
    }
}

void yf::LoadBalance::Impl::remove_session(unsigned long session_id)
{
    std::map<unsigned long, std::string>::iterator isess;
    isess = m_session_target.find(session_id);
    if (isess == m_session_target.end())
        return;

    std::string target = isess->second;
    m_session_target.erase(isess);

    // finding target statistics
    std::map<std::string, TargetStat>::iterator itarg;
    itarg = m_target_stat.find(target);
    if (itarg == m_target_stat.end())
        return;

    if (itarg->second.sessions > 0)
        itarg->second.sessions -= 1;

    if (itarg->second.sessions == 0 || itarg->second.deads == 0)
        m_target_stat.erase(itarg);
}

std::string yf::LoadBalance::Impl::find_session_target(unsigned long session_id)
{
    std::string target;
    std::map<unsigned long, std::string>::iterator isess;
    isess = m_session_target.find(session_id);
    if (isess != m_session_target.end())
        target = isess->second;
    return target;
}


// cost functions
unsigned int yf::LoadBalance::Impl::cost(std::string target)
{
    unsigned int cost = 0;

    if (target.size() != 0)
    {
        std::map<std::string, TargetStat>::iterator itarg;
        itarg = m_target_stat.find(target);
        if (itarg != m_target_stat.end())
            cost = itarg->second.cost();
    }
    return cost;
}

unsigned int yf::LoadBalance::Impl::dead(std::string target)
{
    unsigned int dead = 0;

    if (target.size() != 0)
    {
        std::map<std::string, TargetStat>::iterator itarg;
        itarg = m_target_stat.find(target);
        if (itarg != m_target_stat.end())
            dead = itarg->second.deads;
    }
    return dead;
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::LoadBalance;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_load_balance = {
        0,
        "load_balance",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

