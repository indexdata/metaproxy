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
#include "filter_sort.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>

#include <yaz/diagbib1.h>
#include <yaz/copy_types.h>
#include <yaz/log.h>
#include <yaz/oid_std.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <yaz/zgdu.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Sort::Impl {
            friend class Frontend;
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        private:
            int m_prefetch;
            std::string m_xpath_expr;
            std::string m_namespaces;
            bool m_ascending;
            bool m_debug;
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<mp::Session, FrontendPtr> m_clients;
            FrontendPtr get_frontend(mp::Package &package);
            void release_frontend(mp::Package &package);
        };
        class Sort::Record {
            friend class RecordList;
            Z_NamePlusRecord *npr;
            std::string score;
            void get_xpath(xmlDoc *doc, const char *namespaces,
                           const char *expr, bool debug);
            bool register_namespaces(xmlXPathContextPtr xpathCtx,
                                     const char *nsList);
        public:
            Record(Z_NamePlusRecord *n, const char *namespaces,
                   const char *expr, bool debug);
            ~Record();
            bool operator < (const Record &rhs) const;
        };
        class Sort::RecordList : boost::noncopyable {
            Odr_oid *syntax;
            std::list<Record> npr_list;
            mp::odr m_odr;
            std::string namespaces;
            std::string xpath_expr;
            bool debug;
        public:
            bool cmp(Odr_oid *syntax);
            void add(Z_NamePlusRecord *s);
            int size();
            Z_NamePlusRecord *get(int i, bool ascending);
            void sort();
            RecordList(Odr_oid *, std::string namespaces,
                       std::string xpath_expr, bool debug);
            ~RecordList();
        };
        class Sort::ResultSet : boost::noncopyable {
            friend class Frontend;
            Odr_int hit_count;
            std::list<RecordListPtr> record_lists;
        };
        class Sort::Frontend : boost::noncopyable {
            friend class Impl;
            Impl *m_p;
            bool m_is_virtual;
            bool m_in_use;
            std::map<std::string, ResultSetPtr> m_sets;
            typedef std::map<std::string, ResultSetPtr>::iterator Sets_it;
            void handle_package(mp::Package &package);
            void handle_search(mp::Package &package, Z_APDU *apdu_req);
            void handle_present(mp::Package &package, Z_APDU *apdu_req);

            void handle_records(mp::Package &package,
                                Z_APDU *apdu_reqq,
                                Z_Records *records,
                                Odr_int start_pos,
                                ResultSetPtr s,
                                Odr_oid *syntax,
                                Z_RecordComposition *comp,
                                const char *resultSetId);
        public:
            Frontend(Impl *impl);
            ~Frontend();
        };
    }
}

static void print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output)
{
    xmlNodePtr cur;
    int size;
    int i;

    assert(output);
    size = nodes ? nodes->nodeNr : 0;

    fprintf(output, "Result (%d nodes):\n", size);
    for (i = 0; i < size; ++i) {
        assert(nodes->nodeTab[i]);

        if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL)
        {
            xmlNsPtr ns = (xmlNsPtr)nodes->nodeTab[i];
            cur = (xmlNodePtr)ns->next;
            if (cur->ns)
                fprintf(output, "= namespace \"%s\"=\"%s\" for node %s:%s\n",
                        ns->prefix, ns->href, cur->ns->href, cur->name);
            else
                fprintf(output, "= namespace \"%s\"=\"%s\" for node %s\n",
                        ns->prefix, ns->href, cur->name);
        }
        else if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE)
        {
            cur = nodes->nodeTab[i];
            if (cur->ns)
                fprintf(output, "= element node \"%s:%s\"\n",
                        cur->ns->href, cur->name);
            else
                fprintf(output, "= element node \"%s\"\n",  cur->name);
        }
        else
        {
            cur = nodes->nodeTab[i];
            fprintf(output, "= node \"%s\": type %d\n", cur->name, cur->type);
        }
    }
}

bool yf::Sort::Record::register_namespaces(xmlXPathContextPtr xpathCtx,
                                           const char *nsList)
{
    xmlChar* nsListDup;
    xmlChar* prefix;
    xmlChar* href;
    xmlChar* next;

    assert(xpathCtx);
    assert(nsList);

    nsListDup = xmlStrdup((const xmlChar *) nsList);
    if (!nsListDup)
        return false;

    next = nsListDup;
    while (next)
    {
        /* skip spaces */
        while (*next == ' ')
            next++;
        if (*next == '\0')
            break;

        /* find prefix */
        prefix = next;
        next = (xmlChar *) xmlStrchr(next, '=');
        if (next == NULL)
        {
            xmlFree(nsListDup);
            return false;
        }
        *next++ = '\0';

        /* find href */
        href = next;
        next = (xmlChar*)xmlStrchr(next, ' ');
        if (next)
            *next++ = '\0';

        /* do register namespace */
        if (xmlXPathRegisterNs(xpathCtx, prefix, href) != 0)
        {
            xmlFree(nsListDup);
            return false;
        }
    }

    xmlFree(nsListDup);
    return true;
}



void yf::Sort::Record::get_xpath(xmlDoc *doc, const char *namespaces,
                                 const char *expr, bool debug)
{
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx)
    {
        register_namespaces(xpathCtx, namespaces);
        xmlXPathObjectPtr xpathObj =
            xmlXPathEvalExpression((const xmlChar *) expr, xpathCtx);
        if (xpathObj)
        {
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            if (debug)
                print_xpath_nodes(nodes, yaz_log_file());
            if (nodes)
            {
                int i;
                for (i = 0; i < nodes->nodeNr; i++)
                {
                    std::string content;
                    xmlNode *ptr = nodes->nodeTab[i];
                    if (ptr->type == XML_ELEMENT_NODE ||
                        ptr->type == XML_ATTRIBUTE_NODE)
                    {
                        content = mp::xml::get_text(ptr->children);
                    }
                    else if (ptr->type == XML_TEXT_NODE)
                    {
                        content = mp::xml::get_text(ptr);
                    }
                    if (content.length())
                    {
                        score = content;
                        break;
                    }
                }
            }
            xmlXPathFreeObject(xpathObj);
        }
        xmlXPathFreeContext(xpathCtx);
    }
}

yf::Sort::Record::Record(Z_NamePlusRecord *n,
                         const char *namespaces,
                         const char *expr,
                         bool debug) : npr(n)
{
    if (npr->which == Z_NamePlusRecord_databaseRecord)
    {
        Z_External *ext = npr->u.databaseRecord;

        if (ext->which == Z_External_octet &&
            !oid_oidcmp(ext->direct_reference, yaz_oid_recsyn_xml))
        {
            xmlDoc *doc = xmlParseMemory(
                (const char *) ext->u.octet_aligned->buf,
                ext->u.octet_aligned->len);
            if (doc)
            {
                get_xpath(doc, namespaces, expr, debug);
                xmlFreeDoc(doc);
            }
        }
    }
}

yf::Sort::Record::~Record()
{
}

bool yf::Sort::Record::operator < (const Record &rhs) const
{
    if (strcmp(this->score.c_str(), rhs.score.c_str()) < 0)
        return true;
    return false;
}

yf::Sort::RecordList::RecordList(Odr_oid *syntax,
                                 std::string a_namespaces,
                                 std::string a_xpath_expr,
                                 bool a_debug)
    : namespaces(a_namespaces), xpath_expr(a_xpath_expr), debug(a_debug)

{
    if (syntax)
        this->syntax = odr_oiddup(m_odr, syntax);
    else
        this->syntax = 0;
}

yf::Sort::RecordList::~RecordList()
{

}

bool yf::Sort::RecordList::cmp(Odr_oid *syntax)
{
    if ((!this->syntax && !syntax)
        ||
        (this->syntax && syntax && !oid_oidcmp(this->syntax, syntax)))
        return true;
    return false;
}

int yf::Sort::RecordList::size()
{
    return npr_list.size();
}

void yf::Sort::RecordList::add(Z_NamePlusRecord *s)
{
    ODR oi = m_odr;
    Z_NamePlusRecord *npr = yaz_clone_z_NamePlusRecord(s, oi->mem);
    Record record(npr, namespaces.c_str(), xpath_expr.c_str(), debug);
    npr_list.push_back(record);
}

Z_NamePlusRecord *yf::Sort::RecordList::get(int pos, bool ascending)
{
    std::list<Record>::const_iterator it = npr_list.begin();
    int i = pos;
    if (!ascending)
        i = npr_list.size() - pos - 1;
    for (; it != npr_list.end(); it++, --i)
        if (i <= 0)
        {
            return it->npr;
        }
    return 0;
}

void yf::Sort::RecordList::sort()
{
    npr_list.sort();
}

yf::Sort::Sort() : m_p(new Impl)
{
}

yf::Sort::~Sort()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Sort::configure(const xmlNode *xmlnode, bool test_only,
                         const char *path)
{
    m_p->configure(xmlnode, test_only, path);
}

void yf::Sort::process(mp::Package &package) const
{
    m_p->process(package);
}


yf::Sort::Frontend::Frontend(Impl *impl) :
    m_p(impl), m_is_virtual(false), m_in_use(true)
{
}

yf::Sort::Frontend::~Frontend()
{
}


yf::Sort::Impl::Impl() : m_prefetch(20), m_ascending(true), m_debug(false)
{
}

yf::Sort::Impl::~Impl()
{
}

yf::Sort::FrontendPtr yf::Sort::Impl::get_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<mp::Session,yf::Sort::FrontendPtr>::iterator it;

    while(true)
    {
        it = m_clients.find(package.session());
        if (it == m_clients.end())
            break;

        if (!it->second->m_in_use)
        {
            it->second->m_in_use = true;
            return it->second;
        }
        m_cond_session_ready.wait(lock);
    }
    FrontendPtr f(new Frontend(this));
    m_clients[package.session()] = f;
    f->m_in_use = true;
    return f;
}

void yf::Sort::Impl::release_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<mp::Session,yf::Sort::FrontendPtr>::iterator it;

    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        if (package.session().is_closed())
        {
            m_clients.erase(it);
        }
        else
        {
            it->second->m_in_use = false;
        }
        m_cond_session_ready.notify_all();
    }
}

void yf::Sort::Impl::configure(const xmlNode *ptr, bool test_only,
                               const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "sort"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "prefetch"))
                {
                    m_prefetch = mp::xml::get_int(attr->children, -1);
                    if (m_prefetch < 0)
                    {
                        throw mp::filter::FilterException(
                            "Bad or missing value for attribute " +
                            std::string((const char *) attr->name));
                    }
                }
                else if (!strcmp((const char *) attr->name, "xpath"))
                {
                    m_xpath_expr = mp::xml::get_text(attr->children);
                }
                else if (!strcmp((const char *) attr->name, "namespaces"))
                {
                    m_namespaces = mp::xml::get_text(attr->children);
                }
                else if (!strcmp((const char *) attr->name, "ascending"))
                {
                    m_ascending = mp::xml::get_bool(attr->children, true);
                }
                else if (!strcmp((const char *) attr->name, "debug"))
                {
                    m_debug = mp::xml::get_bool(attr->children, false);
                }
                else
                    throw mp::filter::FilterException(
                        "Bad attribute " +
                        std::string((const char *) attr->name));
            }
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element "
                 + std::string((const char *) ptr->name)
                 + " in sort filter");
        }
    }
    if (m_xpath_expr.length() == 0)
    {
        throw mp::filter::FilterException
            ("Missing xpath attribute for config element in sort filter");
    }

}

void yf::Sort::Frontend::handle_records(mp::Package &package,
                                        Z_APDU *apdu_req,
                                        Z_Records *records,
                                        Odr_int start_pos,
                                        ResultSetPtr s,
                                        Odr_oid *syntax,
                                        Z_RecordComposition *comp,
                                        const char *resultSetId)
{
    if (records && records->which == Z_Records_DBOSD && start_pos == 1)
    {
        std::list<RecordListPtr>::const_iterator it = s->record_lists.begin();

        for (; it != s->record_lists.end(); it++)
            if ((*it)->cmp(syntax))
                return;

        Z_NamePlusRecordList *nprl = records->u.databaseOrSurDiagnostics;
        int i;    // i is number of records fetched in last response

        int pos = 1;
        RecordListPtr rlp(new RecordList(syntax,
                                         m_p->m_namespaces.c_str(),
                                         m_p->m_xpath_expr.c_str(),
                                         m_p->m_debug));
        for (i = 0; i < nprl->num_records; i++, pos++)
            rlp->add(nprl->records[i]);

        int end_pos = m_p->m_prefetch;
        if (end_pos > s->hit_count)
            end_pos = s->hit_count;
        while (i && pos <= end_pos)
        {
            mp::odr odr;
            i = 0;

            Package present_package(package.session(), package.origin());
            present_package.copy_filter(package);

            Z_APDU *p_apdu = zget_APDU(odr, Z_APDU_presentRequest);
            Z_PresentRequest *p_req = p_apdu->u.presentRequest;

            *p_req->resultSetStartPoint = pos;
            *p_req->numberOfRecordsRequested = end_pos - pos + 1;
            p_req->preferredRecordSyntax = syntax;
            p_req->resultSetId = odr_strdup(odr, resultSetId);
            p_req->recordComposition = comp;

            present_package.request() = p_apdu;
            present_package.move();

            Z_GDU *gdu_res = present_package.response().get();
            if (gdu_res && gdu_res->which == Z_GDU_Z3950 &&
                gdu_res->u.z3950->which == Z_APDU_presentResponse)
            {
                Z_PresentResponse *res = gdu_res->u.z3950->u.presentResponse;
                Z_Records *records = res->records;
                if (records && records->which == Z_Records_DBOSD)
                {
                    Z_NamePlusRecordList *nprl =
                        records->u.databaseOrSurDiagnostics;
                    for (i = 0; i < nprl->num_records; i++, pos++)
                        rlp->add(nprl->records[i]);
                }
            }
        }
        s->record_lists.push_back(rlp);
        rlp->sort();

        for (i = 0; i < nprl->num_records; i++)
            nprl->records[i] = rlp->get(i, m_p->m_ascending);
    }
}

void yf::Sort::Frontend::handle_search(mp::Package &package, Z_APDU *apdu_req)
{
    Z_SearchRequest *req = apdu_req->u.searchRequest;
    std::string resultSetId = req->resultSetName;
    mp::odr odr;
    Odr_oid *syntax = 0;

    if (req->preferredRecordSyntax)
        syntax = odr_oiddup(odr, req->preferredRecordSyntax);

    Sets_it sets_it = m_sets.find(req->resultSetName);
    if (sets_it != m_sets.end())
    {
        // result set already exist
        // if replace indicator is off: we return diagnostic if
        // result set already exist.
        if (*req->replaceIndicator == 0)
        {
            Z_APDU *apdu =
                odr.create_searchResponse(
                    apdu_req,
                    YAZ_BIB1_RESULT_SET_EXISTS_AND_REPLACE_INDICATOR_OFF,
                    0);
            package.response() = apdu;
            return;
        }
        m_sets.erase(resultSetId);
    }
    ResultSetPtr s(new ResultSet);
    m_sets[resultSetId] = s;

    Package b_package(package.session(), package.origin());
    b_package.copy_filter(package);
    b_package.request() = apdu_req;
    b_package.move();

    Z_GDU *gdu_res = b_package.response().get();
    if (gdu_res && gdu_res->which == Z_GDU_Z3950 && gdu_res->u.z3950->which ==
        Z_APDU_searchResponse)
    {
        Z_SearchResponse *res = gdu_res->u.z3950->u.searchResponse;
        Z_RecordComposition *record_comp =
            mp::util::piggyback_to_RecordComposition(odr,
                                                     *res->resultCount, req);
        s->hit_count = *res->resultCount;
        handle_records(package, apdu_req, res->records, 1, s,
                       syntax, record_comp, resultSetId.c_str());
        package.response() = gdu_res;
    }
    else
        package.response() = b_package.response();
    if (b_package.session().is_closed())
        b_package.session().close();
}

void yf::Sort::Frontend::handle_present(mp::Package &package, Z_APDU *apdu_req)
{
    Z_PresentRequest *req = apdu_req->u.presentRequest;
    std::string resultSetId = req->resultSetId;
    mp::odr odr;
    Odr_oid *syntax = 0;
    Odr_int start = *req->resultSetStartPoint;

    if (req->preferredRecordSyntax)
        syntax = odr_oiddup(odr, req->preferredRecordSyntax);

    Sets_it sets_it = m_sets.find(resultSetId);
    if (sets_it == m_sets.end())
    {
        Z_APDU *apdu =
            odr.create_presentResponse(
                apdu_req,
                YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
                resultSetId.c_str());
        package.response() = apdu;
        return;
    }
    ResultSetPtr rset = sets_it->second;
    std::list<RecordListPtr>::const_iterator it = rset->record_lists.begin();
    for (; it != rset->record_lists.end(); it++)
        if ((*it)->cmp(req->preferredRecordSyntax))
        {
            if (*req->resultSetStartPoint - 1 + *req->numberOfRecordsRequested
                <= (*it)->size())
            {
                int i;
                Z_APDU *p_apdu = zget_APDU(odr, Z_APDU_presentResponse);
                Z_PresentResponse *p_res = p_apdu->u.presentResponse;

                *p_res->nextResultSetPosition = *req->resultSetStartPoint +
                    *req->numberOfRecordsRequested;
                *p_res->numberOfRecordsReturned =
                    *req->numberOfRecordsRequested;
                p_res->records = (Z_Records *)
                    odr_malloc(odr, sizeof(*p_res->records));
                p_res->records->which = Z_Records_DBOSD;
                Z_NamePlusRecordList *nprl =  (Z_NamePlusRecordList *)
                    odr_malloc(odr, sizeof(*nprl));
                p_res->records->u.databaseOrSurDiagnostics = nprl;
                nprl->num_records = *req->numberOfRecordsRequested;
                nprl->records = (Z_NamePlusRecord **)
                    odr_malloc(odr, nprl->num_records * sizeof(*nprl->records));
                for (i = 0; i < nprl->num_records; i++)
                {
                    int pos = i + *req->resultSetStartPoint - 1;
                    nprl->records[i] = (*it)->get(pos, m_p->m_ascending);
                }
                package.response() = p_apdu;
                return;
            }
            break;
        }


    Package b_package(package.session(), package.origin());
    b_package.copy_filter(package);
    b_package.request() = apdu_req;
    b_package.move();
    Z_GDU *gdu_res = b_package.response().get();
    if (gdu_res && gdu_res->which == Z_GDU_Z3950 && gdu_res->u.z3950->which ==
        Z_APDU_presentResponse)
    {
        Z_PresentResponse *res = gdu_res->u.z3950->u.presentResponse;
        handle_records(package, apdu_req, res->records,
                       start, rset, syntax, req->recordComposition,
                       resultSetId.c_str());
        package.response() = gdu_res;
    }
    else
        package.response() = b_package.response();
    if (b_package.session().is_closed())
        b_package.session().close();
}

void yf::Sort::Frontend::handle_package(mp::Package &package)
{
    Z_GDU *gdu = package.request().get();
    if (gdu && gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        switch (apdu_req->which)
        {
        case Z_APDU_searchRequest:
            handle_search(package, apdu_req);
            return;
        case Z_APDU_presentRequest:
            handle_present(package, apdu_req);
            return;
        }
    }
    package.move();
}

void yf::Sort::Impl::process(mp::Package &package)
{
    FrontendPtr f = get_frontend(package);
    Z_GDU *gdu = package.request().get();

    if (f->m_is_virtual)
    {
        f->handle_package(package);
    }
    else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
             Z_APDU_initRequest)
    {
        package.move();
        f->m_is_virtual = true;
    }
    else
        package.move();

    release_frontend(package);
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::Sort;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_sort = {
        0,
        "sort",
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

