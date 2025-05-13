#pragma once

#include "exit.h"
#include "printk.h"

#define assert(cond)                                                           \
  if (!(cond)) {                                                               \
    printk("Assertion Failed: %s\n", #cond);                                   \
    EXIT;                                                                      \
  }
