#include <stdio.h>

#include "ols.h"

int main(int argc, char **argv) {
  if (!ols_startup("en-US", NULL, NULL)) printf("Couldn't create OBS");
  struct ols_module_failure_info mfi;

  // AddExtraModulePaths();
  printf("---------------------------------\n");
  ols_load_all_modules2(&mfi);
  printf("---------------------------------\n");
  ols_log_loaded_modules();
  printf("---------------------------------\n");
  ols_post_load_modules();

  return 0;
}
