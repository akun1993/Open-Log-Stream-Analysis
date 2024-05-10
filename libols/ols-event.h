
#pragma once
#include <stdint.h>

typedef enum {
  OLS_EVENT_LINK_OK = 0,
  OLS_EVENT_LINK_WRONG_HIERARCHY = -1,
  OLS_EVENT_LINK_WAS_LINKED = -2,
  OLS_EVENT_LINK_WRONG_DIRECTION = -3,
  OLS_EVENT_LINK_NOFORMAT = -4,
} ols_event_type;

struct ols_event {

  ols_event_type ev_type;
};

typedef struct ols_event ols_event_t;