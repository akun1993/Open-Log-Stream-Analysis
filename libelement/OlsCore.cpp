
#include "OlsCore.hpp"

static bool ols_init(const char *locale, const char *module_config_path,profiler_name_store_t *store) {
    ols = bzalloc(sizeof(struct ols_core));

    ols->name_store_owned = !store;
    ols->name_store = store ? store : profiler_name_store_create();
    if (!ols->name_store) {
    blog(LOG_ERROR, "Couldn't create profiler name store");
    return false;
    }

    log_system_info();

// if (!ols_init_data())
// return false;
// if (!ols_init_handlers())
// return false;


// ols->destruction_task_thread = os_task_queue_create();
// if (!ols->destruction_task_thread)
// return false;

    if (module_config_path)
    ols->module_config_path = bstrdup(module_config_path);
    ols->locale = bstrdup(locale);

    add_default_module_paths();
    return true;
}

#ifdef _WIN32
extern bool initialize_com(void);
extern void uninitialize_com(void);
static bool com_initialized = false;
#endif

/* Separate from actual context initialization
* since this can be set before startup and persist
* after shutdown. */
static DARRAY(struct dstr) core_module_paths = {0};

char *ols_find_data_file(const char *file) {
    struct dstr path = {0};

    char *result = find_libols_data_file(file);
    if (result)
    return result;

    for (size_t i = 0; i < core_module_paths.num; ++i) {
        if (check_path(file, core_module_paths.array[i].array, &path))
            return path.array;
    }

    blog(LOG_ERROR, "Failed to find file '%s' in libols data directory", file);

    dstr_free(&path);
    return NULL;
}

void ols_add_data_path(const char *path) {
    struct dstr *new_path = da_push_back_new(core_module_paths);
    dstr_init_copy(new_path, path);
}

bool ols_remove_data_path(const char *path) {
    for (size_t i = 0; i < core_module_paths.num; ++i) {
        int result = dstr_cmp(&core_module_paths.array[i], path);

        if (result == 0) {
            dstr_free(&core_module_paths.array[i]);
            da_erase(core_module_paths, i);
            return true;
        }
    }

    return false;
}

static const char *ols_startup_name = "ols_startup";
bool ols_startup(const char *locale, const char *module_config_path,profiler_name_store_t *store) {
    bool success;

    profile_start(ols_startup_name);

    if (ols) {
        blog(LOG_WARNING, "Tried to call ols_startup more than once");
        return false;
    }

    #ifdef _WIN32
    com_initialized = initialize_com();
    #endif

    success = ols_init(locale, module_config_path, store);
    profile_end(ols_startup_name);
    if (!success)
      ols_shutdown();

    return success;
}

static struct ols_cmdline_args cmdline_args = {0, NULL};

void ols_set_cmdline_args(int argc, const char *const *argv) {
    char *data;
    size_t len;
    int i;

    /* Once argc is set (non-zero) we shouldn't call again */
    if (cmdline_args.argc)
        return;

    cmdline_args.argc = argc;

    /* Safely copy over argv */
    len = 0;
    for (i = 0; i < argc; i++)
        len += strlen(argv[i]) + 1;

    cmdline_args.argv = bmalloc(sizeof(char *) * (argc + 1) + len);
    data = (char *)cmdline_args.argv + sizeof(char *) * (argc + 1);

    for (i = 0; i < argc; i++) {
        cmdline_args.argv[i] = data;
        len = strlen(argv[i]) + 1;
        memcpy(data, argv[i], len);
        data += len;
    }

    cmdline_args.argv[argc] = NULL;
}

struct ols_cmdline_args ols_get_cmdline_args(void) {
    return cmdline_args;
}

void ols_shutdown(void) {
    struct ols_module *module;

    ols_wait_for_destroy_queue();

    for (size_t i = 0; i < ols->source_types.num; i++) {
    struct ols_source_info *item = &ols->source_types.array[i];
        if (item->type_data && item->free_type_data)
            item->free_type_data(item->type_data);
        if (item->id)
            bfree((void *)item->id);
    }
    da_free(ols->source_types);

    #define FREE_REGISTERED_TYPES(structure, list)                                 \
    do {                                                                         \
    for (size_t i = 0; i < list.num; i++) {                                    \
    struct structure *item = &list.array[i];                                 \
    if (item->type_data && item->free_type_data)                             \
    item->free_type_data(item->type_data);                                 \
    }                                                                          \
    da_free(list);                                                             \
    } while (false)

    FREE_REGISTERED_TYPES(ols_output_info, ols->output_types);

    #undef FREE_REGISTERED_TYPES

    // da_free(ols->input_types);

    stop_hotkeys();

    module = ols->first_module;
    while (module) {
    struct ols_module *next = module->next;
    free_module(module);
    module = next;
    }
    ols->first_module = NULL;

    ols_free_data();
    os_task_queue_destroy(ols->destruction_task_thread);
    ols_free_hotkeys();
    proc_handler_destroy(ols->procs);
    signal_handler_destroy(ols->signals);
    ols->procs = NULL;
    ols->signals = NULL;

    for (size_t i = 0; i < ols->module_paths.num; i++)
    free_module_path(ols->module_paths.array + i);
    da_free(ols->module_paths);

    for (size_t i = 0; i < ols->safe_modules.num; i++)
    bfree(ols->safe_modules.array[i]);
    da_free(ols->safe_modules);

    if (ols->name_store_owned)
    profiler_name_store_free(ols->name_store);

    bfree(ols->module_config_path);
    bfree(ols->locale);
    bfree(ols);
    ols = NULL;
    bfree(cmdline_args.argv);

    #ifdef _WIN32
    if (com_initialized)
    uninitialize_com();
    #endif
}

bool ols_initialized(void) { return ols != NULL; }

uint32_t ols_get_version(void) { return LIBOLS_API_VER; }

const char *ols_get_version_string(void) { return OLS_VERSION; }

void ols_set_locale(const char *locale) {
    struct ols_module *module;

    if (ols->locale)
        bfree(ols->locale);

    ols->locale = bstrdup(locale);

    module = ols->first_module;
    while (module) {
        if (module->set_locale)
            module->set_locale(locale);

        module = module->next;
    }
}

const char *ols_get_locale(void) { return ols->locale; }

