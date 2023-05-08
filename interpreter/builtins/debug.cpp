#include <iostream>
#include <string>

#include "interpreter/builtins/builtins.hpp"
#include "interpreter/interpreter.hpp"
#include "libnibi/cell.hpp"

#include "cpp_macros.hpp"

namespace builtins {

namespace {
void add_debug_info(std::string &result, cell_c &cell_value, cell_c &processed,
                    const char *prefix = "") {

  if (processed.type == cell_type_e::SYMBOL) {
    result += prefix;
    result += "Symbol: ";
    result += processed.to_string();
    result += "\n";
  }

  result += prefix;
  result += "Type: ";
  result += cell_type_to_string(cell_value.type);
  result += "\n";

  result += prefix;
  result += "Value: ";
  result += cell_value.to_string();
  result += "\n\n";

  result += "----------------\n";
}
} // namespace

cell_ptr builtin_fn_debug_dbg_dbg(cell_list_t &list, env_c &env) {
  if (!global_interpreter->is_debug_enabled()) {
    return nullptr;
  }
  LIST_ENFORCE_SIZE("dbg", >, 1)
  LIST_ITER_AND_LOAD_SKIP_N(
      1, { global_interpreter->execute_cell(arg, env, true); });
  return ALLOCATE_CELL(cell_type_e::NIL);
}

cell_ptr builtin_fn_debug_dbg_out(cell_list_t &list, env_c &env) {
  if (!global_interpreter->is_debug_enabled()) {
    return ALLOCATE_CELL(cell_type_e::NIL);
  }

  LIST_ENFORCE_SIZE("dbg-out", >, 1)
  LIST_ITER_AND_LOAD_SKIP_N(1, { std::cout << arg->to_string() << std::endl; });
  return ALLOCATE_CELL(cell_type_e::NIL);
}

cell_ptr builtin_fn_debug_dbg_var(cell_list_t &list, env_c &env) {

  if (!global_interpreter->is_debug_enabled()) {
    return ALLOCATE_CELL(cell_type_e::NIL);
  }

  LIST_ENFORCE_SIZE("dbg-var", >, 1)

  auto origin = list.begin();

  auto dbg_cell = (*origin);

  std::string result;

  result += "\n----------------\n\n";
  result += "`dbg-var` instruction [";

  if (dbg_cell->locator) {
    result += dbg_cell->locator->get_source_name();
    result += ":(";
    result += std::to_string(dbg_cell->locator->get_line());
    result += ",";
    result += std::to_string(dbg_cell->locator->get_column());
    result += ")";
  } else {
    result += "< no location info given >";
  }
  result += "]\n\nCells given: ";
  result += std::to_string(list.size() - 1);
  result += "\n";

  std::size_t args_count{0};
  LIST_ITER_AND_LOAD_SKIP_N(1, {
    result += "\nArg " + std::to_string(args_count++) + ":\n";

    add_debug_info(result, *arg, *(*it), "\t");
  });

  return ALLOCATE_CELL(result);
}

} // namespace builtins
