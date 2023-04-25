#include <iostream>

#include "runtime/builtins/builtins.hpp"
#include "runtime/cell.hpp"
#include "runtime/runtime.hpp"

#include "cpp_macros.hpp"

#include <iterator>
namespace builtins {

cell_c *builtin_fn_env_assignment(cell_list_t &list, env_c &env) {

  LIST_ENFORCE_SIZE(==, 3)

  auto it = list.begin();
  std::advance(it, 1);

  auto &target_variable_name = (*it)->as_string();

  auto *target_assignment_value =
      global_runtime->execute_cell(list_get_nth_arg(2, list, env), env);

  // Explicitly clone the value as we might be reading from
  // an instruction that will be mutated later

  target_assignment_value = target_assignment_value->clone();

  // We could shortcut this with something like a `local` keyword
  // so we don't scan multiple maps and just go straight to the
  // given env

  auto *target_env = env.get_env(target_variable_name);

  if (!target_env) {
    target_env = &env;
  }

  target_env->set(target_variable_name, *target_assignment_value);

  // Return a pointer to the new cell so assignments can be chained
  return target_assignment_value;
}

cell_c *builtin_fn_env_drop(cell_list_t &list, env_c &env) {

  LIST_ENFORCE_SIZE(>=, 2)

  LIST_ITER_SKIP_N(1, {
    if (!env.drop((*it)->as_symbol())) {
      throw runtime_c::exception_c("Could not find symbol with name :" +
                                       (*it)->as_symbol(),
                                   (*it)->locator);
    }
  })
  return global_cell_true;
}

namespace {
cell_c *handle_thrown_error_in_try(std::string message, cell_c &recover_cell,
                                   env_c &env) {
  auto *e_cell = global_runtime->get_runtime_memory().allocate(message);
  env.set("$e", *e_cell);
  auto *result = global_runtime->execute_cell(&recover_cell, env, true);
  env.drop("$e");
  return result;
}
} // namespace

cell_c *builtin_fn_env_try(cell_list_t &list, env_c &env) {

  /*
    (try () ())

    The first param can be a process list, or a data list that will be walked
    and itered

    The second param is a process list of a data list used to catch the error
    and deal with it

    Temporarily, we will inject an environment with `$e` set to the error
    message

  */

  LIST_ENFORCE_SIZE(==, 3)

  auto it = list.begin();

  std::advance(it, 1);
  auto *attempt_cell = (*it);

  std::advance(it, 1);
  auto *recover_cell = (*it);

  try {
    // Call execute with the process_data_cell flag set to true
    // which will allow us to walk over multiple cells and catch on them
    return global_runtime->execute_cell(attempt_cell, env, true);
  } catch (runtime_c::exception_c &e) {
    return handle_thrown_error_in_try(e.what(), *recover_cell, env);
  } catch (cell_access_exception_c &e) {
    return handle_thrown_error_in_try(e.what(), *recover_cell, env);
  }
  return global_cell_nil;
}

} // namespace builtins
