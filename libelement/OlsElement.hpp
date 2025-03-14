#ifndef __OLS_ELEMENT_HPP__
#define __OLS_ELEMENT_HPP__

/*
*  element class. Override the vmethods to implement the element
* functionality.
*/

#include <list>
#include <stdint.h>
#include "OlsPad.hpp"
#include "OlsMessage.hpp"
#include "OlsEvent.hpp"

class OlsElement{

public:
    void (*pad_added)     (OlsElement *element, OlsPad *pad);
    void (*pad_removed)   (OlsElement *element, OlsPad *pad);

        /* query functions */
    bool  (*send_event) (OlsElement *element, OlsEvent *event);


    bool  (*post_message) (OlsElement *element, OlsMessage *message);

    /* element pads, these lists can only be iterated while holding
     * the LOCK or checking the cookie after each LOCK. */
    uint16_t               numpads;
    std::list<OlsPad>      pads;
    uint16_t               numsrcpads;
    std::list<OlsPad>      srcpads;
    uint16_t               numsinkpads;
    std::list<OlsPad>      sinkpads;
    uint32_t               pads_cookie;
  
    /* with object LOCK */
    //std::list              contexts;

};

#endif
