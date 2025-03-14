#ifndef __OLS_CORE_HPP__
#define __OLS_CORE_HPP__

struct ols_task_info {
    ols_task_t task;
    void *param;
  };
  
  /* user sources, output channels, and displays */
  struct ols_core_data {
    /* Hash tables (uthash) */
    struct ols_source *sources;        /* Lookup by UUID (hh_uuid) */
    struct ols_source *public_sources; /* Lookup by name (hh) */
    struct ols_process *processes;     /* Lookup by UUID (hh_uuid) */
  
    /* Linked lists */
    struct ols_output *first_output;
  
    pthread_mutex_t sources_mutex;
    pthread_mutex_t processes_mutex;
    pthread_mutex_t outputs_mutex;
    DARRAY(struct tick_callback) tick_callbacks;
  
    // struct ols_view main_view;
  
    long long unnamed_index;
  
    ols_data_t *private_data;
  
    volatile bool valid;
  
    DARRAY(char *) protocols;
    DARRAY(ols_source_t *) sources_to_tick;
  };

class OlsCore {
    OlsModule *first_module;
    DARRAY(struct ols_module_path) module_paths;
    DARRAY(char *) safe_modules;
  
    //ols_source_info_array_t source_types;
    // ols_source_info_array_t input_types;
    // ols_source_info_array_t filter_types;
   // DARRAY(struct ols_process_info) process_types;
   // DARRAY(struct ols_output_info) output_types;
  
    signal_handler_t *signals;
    proc_handler_t *procs;
  
    char *locale;
    char *module_config_path;
    bool name_store_owned;
    profiler_name_store_t *name_store;
  
    /* segmented into multiple sub-structures to keep things a bit more
     * clean and organized */
    struct ols_core_data data;

    os_task_queue_t *destruction_task_thread;
    ols_task_handler_t ui_task_handler;
};
  
  extern  OlsCore *ols;
  
  /* ------------------------------------------------------------------------- */
  /* ols shared context data */
  
  struct ols_weak_ref {
    volatile long refs;
    volatile long weak_refs;
  };
  
  struct ols_weak_object {
    struct ols_weak_ref ref;
    struct ols_context_data *object;
  };
  
  typedef void (*ols_destroy_cb)(void *obj);
  
  struct ols_context_data {
    char *name;
    const char *uuid;
    void *data;
    ols_data_t *settings;
    signal_handler_t *signals;
    proc_handler_t *procs;
    enum ols_obj_type type;
  }  


#endif
