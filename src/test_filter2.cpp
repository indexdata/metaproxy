/* $Id: test_filter2.cpp,v 1.16 2005-12-02 12:21:07 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "config.hpp"
#include "filter.hpp"
#include "router_chain.hpp"
#include "package.hpp"

#include <iostream>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;


class FilterConstant: public yp2::filter::Base {
public:
    FilterConstant() : m_constant(1234) { };
    void process(yp2::Package & package) const {
	package.data() = m_constant;
	package.move();
    };
    void configure(const xmlNode* ptr = 0);
    int get_constant() const { return m_constant; };
private:
    bool parse_xml_text(const xmlNode *xml_ptr, bool &val);
    bool parse_xml_text(const xmlNode *xml_ptr, std::string &val);
private:
    const xmlNode *m_ptr;
    int m_constant;
};


void FilterConstant::configure(const xmlNode* ptr)
{
    m_ptr = ptr;
    
    BOOST_CHECK_EQUAL (ptr->type, XML_ELEMENT_NODE);
    BOOST_CHECK_EQUAL(std::string((const char *) ptr->name), "filter");
    
    const struct _xmlAttr *attr;
    
    for (attr = ptr->properties; attr; attr = attr->next)
    {
        BOOST_CHECK_EQUAL( std::string((const char *)attr->name), "type");
        const xmlNode *val = attr->children;
        BOOST_CHECK_EQUAL(val->type, XML_TEXT_NODE);
        BOOST_CHECK_EQUAL(std::string((const char *)val->content), "constant");
    }
    const xmlNode *p = ptr->children;
    for (; p; p = p->next)
    {
        if (p->type != XML_ELEMENT_NODE)
            continue;
        
        BOOST_CHECK_EQUAL (p->type, XML_ELEMENT_NODE);
        BOOST_CHECK_EQUAL(std::string((const char *) p->name), "value");
        
        const xmlNode *val = p->children;
        BOOST_CHECK(val);
        if (!val)
            continue;
        
        BOOST_CHECK_EQUAL(val->type, XML_TEXT_NODE);
        BOOST_CHECK_EQUAL(std::string((const char *)val->content), "2");

        m_constant = atoi((const char *) val->content);
    }
}

bool FilterConstant::parse_xml_text(const xmlNode  *xml_ptr, bool &val)
{
    std::string v;
    if (!parse_xml_text(xml_ptr, v))
        return false;
    if (v.length() == 1 && v[0] == '1')
        val = true;
    else
        val = false;
    return true;
}

bool FilterConstant::parse_xml_text(const xmlNode *xml_ptr, std::string &val)
{
    xmlNodePtr ptr = (xmlNodePtr) xml_ptr;
    bool found = false;
    std::string v;
    for(ptr = ptr->children; ptr; ptr = ptr->next)
        if (ptr->type == XML_TEXT_NODE)
        {
            xmlChar *t = ptr->content;
            if (t)
            {
                v += (const char *) t;
                found = true;
            }
        }
    if (found)
        val = v;
    return found;
}

// This filter dose not have a configure function
    
class FilterDouble: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {
	package.data() = package.data() * 2;
	package.move();
    };
};

    
BOOST_AUTO_UNIT_TEST( testfilter2_1 ) 
{
    try {
	FilterConstant fc;
	FilterDouble fd;

	{
	    yp2::RouterChain router1;
	    
	    // test filter set/get/exception
	    router1.append(fc);
	    
	    router1.append(fd);

            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack(session, origin);
	    
	    pack.router(router1).move(); 
	    
            BOOST_CHECK (pack.data() == 2468);
            
        }
        
        {
	    yp2::RouterChain router2;
	    
	    router2.append(fd);
	    router2.append(fc);
	    
            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack(session, origin);
	 
            pack.router(router2).move();
     
            BOOST_CHECK (pack.data() == 1234);
            
	}

    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ...) {
        BOOST_CHECK (false);
    }

}

BOOST_AUTO_UNIT_TEST( testfilter2_2 ) 
{
    try {
	FilterConstant fc;
        BOOST_CHECK_EQUAL(fc.get_constant(), 1234);

        yp2::filter::Base *base = &fc;

        std::string some_xml = "<?xml version=\"1.0\"?>\n"
            "<filter type=\"constant\">\n"
            " <value>2</value>\n"
            "</filter>";
        
        // std::cout << some_xml  << std::endl;

        xmlDocPtr doc = xmlParseMemory(some_xml.c_str(), some_xml.size());

        BOOST_CHECK(doc);

        if (doc)
        {
            xmlNodePtr root_element = xmlDocGetRootElement(doc);
            
            base->configure(root_element);
            
            xmlFreeDoc(doc);
        }

        BOOST_CHECK_EQUAL(fc.get_constant(), 2);
    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ...) {
        BOOST_CHECK (false);
    }

}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
