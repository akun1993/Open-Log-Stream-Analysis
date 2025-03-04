

#pragma once
#include "ols.h"
#include <stdbool.h>

typedef struct ols_pipeline ols_pipeline_t;

/**
 * OLS_PIPELINE_NUMCHILDREN:
 * @bin: a #OlsBin
 *
 * Gets the number of children in a bin.
 */
#define OLS_BIN_NUMCHILDREN(bin) (OLS_BIN_CAST(bin)->numchildren)
/**
 * OLS_BIN_CHILDREN:
 * @bin: a #OlsBin
 *
 * Gets the list of children in a bin.
 */
#define OLS_BIN_CHILDREN(bin) (OLS_BIN_CAST(bin)->children)
/**
 * OLS_BIN_CHILDREN_COOKIE:
 * @bin: a #OlsBin
 *
 * Gets the children cookie that watches the children list.
 */
#define OLS_BIN_CHILDREN_COOKIE(bin) (OLS_BIN_CAST(bin)->children_cookie)

/**
 * OlsBin:
 * @numchildren: the number of children in this bin
 * @children: (element-type Ols.Element): the list of children in this bin
 * @children_cookie: updated whenever @children changes
 * @child_bus: internal bus for handling child messages
 * @messages: (element-type Ols.Message): queued and cached messages
 * @polling: the bin is currently calculating its state
 * @state_dirty: the bin needs to recalculate its state (deprecated)
 * @clock_dirty: the bin needs to select a new clock
 * @provided_clock: the last clock selected
 * @clock_provider: the element that provided @provided_clock
 *
 * The OlsBin base class. Subclasses can access these fields provided
 * the LOCK is taken.
 */
struct ols_pipeline {
  // ols_object_t obj;

  int32_t numchildren;
  // GList *children;

  // OlsBus *child_bus;
  // GList *messages;

  bool polling;
  bool state_dirty;

  bool clock_dirty;
  //   OlsClock *provided_clock;
  //   OlsElement *clock_provider;
  char *name;
};

ols_pipeline_t *ols_pipeline_new(const char *name,const char *pipe_desc);


ols_pipeline_t *ols_pipeline_new_test(const char *name) ;
/* add and remove elements from the bin */

// /* retrieve a single child */

// OlsElement *ols_bin_get_by_name(OlsBin *bin, const gchar *name);

// OlsElement *ols_bin_get_by_interface(OlsBin *bin, GType iface);
