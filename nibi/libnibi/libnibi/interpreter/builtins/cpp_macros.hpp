#pragma once

#include "interpreter/interpreter.hpp"
#include <iterator>

namespace nibi {

#define LIST_ITER_SKIP_N(___n, ___loop_body)                                   \
  auto it = list.begin();                                                      \
  std::advance(it, ___n);                                                      \
  for (; it != list.end(); ++it)                                               \
  ___loop_body

#define LIST_ITER_AND_LOAD_SKIP_N(___n, ___loop_body)                          \
  auto it = list.begin();                                                      \
  std::advance(it, ___n);                                                      \
  for (; it != list.end(); ++it) {                                             \
    cell_ptr arg = ci.execute_cell(*it, env);                                  \
    ___loop_body                                                               \
  }

// Check if the list is a given size, and if not error out.
// converts size to number of arguments expected, so if you want 2 arguments
// you should pass 3
#define LIST_ENFORCE_SIZE(___cmd, ___op, ___size)                              \
  if (!(list.size() ___op ___size)) {                                          \
    ci.halt_with_error(error_c(list.front()->locator,                          \
                               std::string(___cmd) + " instruction expects " + \
                                   std::to_string(___size - 1) +               \
                                   " parameters, got " +                       \
                                   std::to_string(list.size() - 1) + "."));    \
    return allocate_cell(cell_type_e::NIL);                                    \
  }

//! \brief Get the nth argument from a list of cells from an execution list,
//!        and execute it with the runtime to load the target value
//! \warning This function does not check to ensure that the list is long enough
//!          to contain the nth argument.  It is the responsibility of the
//!          caller
static inline cell_ptr list_get_nth_arg(interpreter_c &ci, std::size_t n,
                                        cell_list_t &list, env_c &env) {
  auto it = list.begin();
  std::advance(it, n);
  return ci.execute_cell(*it, env);
}
} // namespace nibi