
#include <iostream>

#include "interpreter/builtins/builtins.hpp"
#include "interpreter/interpreter.hpp"
#include "libnibi/cell.hpp"
#include "libnibi/keywords.hpp"
#include "macros.hpp"

namespace nibi {

namespace builtins {

#define NIBI_CONVERSION_TO_TYPE(type, conversion_method)                       \
  auto value = ci.process_cell(list[1], env);                                  \
  try {                                                                        \
    type result = conversion_method(value->to_string());                       \
    return allocate_cell((type)result);                                        \
  } catch (std::invalid_argument & e) {                                        \
    throw interpreter_c::exception_c(                                          \
        std::string("Invalid argument to integer conversion: ") +              \
            value->to_string(),                                                \
        value->locator);                                                       \
  } catch (std::out_of_range & e) {                                            \
    throw interpreter_c::exception_c(                                          \
        std::string("Out of range argument to integer conversion: ") +         \
            value->to_string(),                                                \
        value->locator);                                                       \
  } catch (...) {                                                              \
    throw interpreter_c::exception_c(                                          \
        std::string("Unknown error in integer conversion: ") +                 \
            value->to_string(),                                                \
        value->locator);                                                       \
  }                                                                            \
  return allocate_cell((type)0);

cell_ptr builtin_fn_cvt_to_string(cell_processor_if &ci, cell_list_t &list,
                                  env_c &env) {

  NIBI_LIST_ENFORCE_SIZE(nibi::kw::STR, ==, 2)
  return allocate_cell(ci.process_cell(list[1], env)->to_string());
}

cell_ptr builtin_fn_cvt_to_integer(cell_processor_if &ci, cell_list_t &list,
                                   env_c &env){
    NIBI_LIST_ENFORCE_SIZE(nibi::kw::INT, ==, 2)
        NIBI_CONVERSION_TO_TYPE(int64_t, std::stoll)}

cell_ptr
    builtin_fn_cvt_to_i8(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::I8, ==, 2)
            NIBI_CONVERSION_TO_TYPE(int8_t, std::stoll)}

cell_ptr
    builtin_fn_cvt_to_i16(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::I16, ==, 2)
            NIBI_CONVERSION_TO_TYPE(int16_t, std::stoll)}

cell_ptr
    builtin_fn_cvt_to_i32(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::I32, ==, 2)
            NIBI_CONVERSION_TO_TYPE(int32_t, std::stoll)}

cell_ptr
    builtin_fn_cvt_to_i64(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::I64, ==, 2)
            NIBI_CONVERSION_TO_TYPE(int64_t, std::stoll)}

cell_ptr
    builtin_fn_cvt_to_u8(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::U8, ==, 2)
            NIBI_CONVERSION_TO_TYPE(uint8_t, std::stoull)}

cell_ptr
    builtin_fn_cvt_to_u16(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::U16, ==, 2)
            NIBI_CONVERSION_TO_TYPE(uint16_t, std::stoull)}

cell_ptr
    builtin_fn_cvt_to_u32(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::U32, ==, 2)
            NIBI_CONVERSION_TO_TYPE(uint32_t, std::stoull)}

cell_ptr
    builtin_fn_cvt_to_u64(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::U64, ==, 2)
            NIBI_CONVERSION_TO_TYPE(uint64_t, std::stoull)}

cell_ptr builtin_fn_cvt_to_float(cell_processor_if &ci, cell_list_t &list,
                                 env_c &env){
    NIBI_LIST_ENFORCE_SIZE(nibi::kw::FLOAT, ==, 2)
        NIBI_CONVERSION_TO_TYPE(double, std::stod)}

cell_ptr
    builtin_fn_cvt_to_f32(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::F32, ==, 2)
            NIBI_CONVERSION_TO_TYPE(float, std::stof)}

cell_ptr
    builtin_fn_cvt_to_f64(cell_processor_if &ci, cell_list_t &list, env_c &env){
        NIBI_LIST_ENFORCE_SIZE(nibi::kw::F64, ==, 2)
            NIBI_CONVERSION_TO_TYPE(double, std::stod)}

cell_ptr builtin_fn_cvt_to_split(cell_processor_if &ci, cell_list_t &list,
                                 env_c &env) {
  NIBI_LIST_ENFORCE_SIZE(nibi::kw::SPLIT, >=, 2)
  auto value = ci.process_cell(list[1], env);

  if (value->type == cell_type_e::LIST) {
    NIBI_LIST_ENFORCE_SIZE(nibi::kw::SPLIT, ==, 3)
    auto as_list = value->to_list();

    int64_t count = 0;

    auto target = ci.process_cell(list[2], env)->to_integer();

    if (0 == target) {
      return value->clone(env);
    }
    cell_list_t data_list;
    cell_list_t inner_list;

    for (auto &cell : as_list) {
      if (count == target) {
        data_list.push_back(
            allocate_cell(list_info_s{list_types_e::DATA, inner_list}));
        inner_list.clear();
        count = 0;
      }
      inner_list.push_back(cell->clone(env));
      count++;
    }
    if (inner_list.size()) {
      data_list.push_back(
          allocate_cell(list_info_s{list_types_e::DATA, inner_list}));
    }
    return allocate_cell(list_info_s{list_types_e::DATA, data_list});
  }
  NIBI_LIST_ENFORCE_SIZE(nibi::kw::SPLIT, ==, 2)

  auto as_string = value->to_string();
  cell_list_t data_list;

  for (auto ch : as_string) {
    data_list.push_back(allocate_cell(std::string(1, ch)));
  }

  return allocate_cell(list_info_s{list_types_e::DATA, data_list});
}

} // namespace builtins

} // namespace nibi
