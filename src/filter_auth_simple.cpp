/* $Id: filter_auth_simple.cpp,v 1.3 2006-01-16 16:32:33 mike Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "util.hpp"
#include "filter_auth_simple.hpp"

#include <yaz/zgdu.h>
#include <stdio.h>
#include <errno.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class AuthSimple::Rep {
            friend class AuthSimple;
            typedef std::map<std::string, std::string> userpass;
            userpass reg;
        };
    }
}

yf::AuthSimple::AuthSimple() : m_p(new Rep)
{
    // nothing to do
}

yf::AuthSimple::~AuthSimple()
{  // must have a destructor because of boost::scoped_ptr
}


void reject(yp2::Package &package, const char *addinfo) {
    // Make an Init rejection APDU
    Z_GDU *gdu = package.request().get();
    yp2::odr odr;
    Z_APDU *apdu = odr.create_initResponse(gdu->u.z3950, 1014, addinfo);
    apdu->u.initResponse->implementationName = "YP2/YAZ";
    *apdu->u.initResponse->result = 0; // reject
    package.response() = apdu;
    package.session().close();
}


void yf::AuthSimple::process(yp2::Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950 ||
        gdu->u.z3950->which != Z_APDU_initRequest) {
        // pass on package -- I think that means authentication is
        // accepted which may not be the correct thing for non-Z APDUs
        // as it means that SRW sessions don't demand authentication
        return package.move();
    }

    Z_IdAuthentication *auth = gdu->u.z3950->u.initRequest->idAuthentication;
    if (!auth)
        return reject(package, "no credentials supplied");
    if (auth->which != Z_IdAuthentication_idPass)
        return reject(package, "only idPass authentication is supported");
    Z_IdPass *idPass = auth->u.idPass;
    // groupId is ignored, in accordance with ancient tradition
    if (m_p->reg[idPass->userId] == idPass->password) {
        // Success!   Should the authentication information now be
        // altered or deleted?  That could be configurable.
        return package.move();
    }
    return reject(package, "username/password combination rejected");
}


// Read XML config.. Put config info in m_p.
void yp2::filter::AuthSimple::configure(const xmlNode * ptr)
{
    std::string filename;
    bool got_filename = false;

    for (ptr = ptr->children; ptr != 0; ptr = ptr->next) {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "filename")) {
            filename = yp2::xml::get_text(ptr);
            got_filename = true;
        } else {
            throw yp2::filter::FilterException("Bad element in auth_simple: " 
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }

    if (!got_filename)
        throw yp2::filter::FilterException("auth_simple: no user-register "
                                           "filename specified");

    FILE *fp = fopen(filename.c_str(), "r");
    if (fp == 0)
        throw yp2::filter::FilterException("can't open auth_simple " 
                                           "user-register '" + filename +
                                           "': " + strerror(errno));

    char buf[1000];
    while (fgets(buf, sizeof buf, fp)) {
        if (*buf == '\n' || *buf == '#')
            continue;
        buf[strlen(buf)-1] = 0;
        char *cp = strchr(buf, ':');
        if (cp == 0)
            throw yp2::filter::FilterException("auth_simple user-register '" +
                                               filename + "': bad line: " +
                                               buf);
        *cp++ = 0;
        m_p->reg[buf] = cp;
        //printf("Added user '%s' -> password '%s'\n", buf, cp);
    }
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::AuthSimple;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_auth_simple = {
        0,
        "auth_simple",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
