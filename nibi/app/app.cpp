#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

#include "libnibi/cell.hpp"
#include "libnibi/common/error.hpp"
#include "libnibi/common/input.hpp"
#include "libnibi/common/list.hpp"
#include "libnibi/common/platform.hpp"
#include "libnibi/config.hpp"
#include "libnibi/environment.hpp"
#include "libnibi/interpreter/interpreter.hpp"
#include "libnibi/modules.hpp"
#include "libnibi/source.hpp"
#include "libnibi/version.hpp"

#ifndef NIBI_BUILD_HASH
#define NIBI_BUILD_HASH "unknown"
#endif

#define CALCULATE_EXECUTION_TIME 0

#if CALCULATE_EXECUTION_TIME
#include <chrono>
#endif

using namespace nibi;

namespace {

class program_data_controller_c {
public:

   program_data_controller_c(std::vector<std::string> &args, std::vector<std::filesystem::path> &include_dirs)
      : args_(args), include_dirs_(include_dirs), interpreter_(env_, source_manager_)
   {
      reinit_platform();
   }

   void reset() {
      source_manager_.clear();
      env_.get_map().clear();
      reinit_platform();
   }

   void add_include_dir(std::filesystem::path dir) {
      include_dirs_.push_back(dir);
   }

   env_c &get_env() { return env_; }
   interpreter_c &get_interpreter() { return interpreter_; }
   source_manager_c &get_source_manager() { return source_manager_; }

private:

   void reinit_platform() {
      global_platform_destroy();
      if (!global_platform_init(include_dirs_, args_)) {
         std::cerr << "Failed to initialize global platform" << std::endl;
         exit(1);
      }
   }

   std::vector<std::filesystem::path> &include_dirs_;
   std::vector<std::string> &args_;
   env_c env_;
   source_manager_c source_manager_;
   interpreter_c interpreter_;
};

std::unique_ptr<program_data_controller_c> pdc{nullptr};

} // namespace


void run_from_file(std::filesystem::path file_name) {

  // List builder that will build lists from parsed tokens
  // and pass lists to a interpreter
  list_builder_c list_builder(pdc->get_interpreter());

  // File reader that reads file and kicks off parser/ scanner
  // that will send tokens to the list builder
  file_reader_c file_reader(list_builder, pdc->get_source_manager());

  // Read the file and start the process
  file_reader.read_file(file_name);
}

void run_from_dir(const std::string &file_name) {
  std::filesystem::path dir_path(file_name);
  auto expected_file = dir_path / "main.nibi";
  if (std::filesystem::exists(expected_file)) {
    run_from_file(expected_file);
    return;
  }
}

void show_help() {
  std::cout << "Usage: nibi [options] [file | directory]\n" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help            Show this help message" << std::endl;
  std::cout << "  -v, --version         Show version info" << std::endl;
  std::cout << "  -m, --module <name>   Show module info" << std::endl;
  std::cout << "  -t, --test            Run tests" << std::endl;
  std::cout << "  -i, --include <dirs>  Add include directory (`:` delimited)"
            << std::endl;
}

void show_version() {
  std::cout << "libnibi version: " << LIBNIBI_VERSION << std::endl;
  std::cout << "application build hash: " << NIBI_BUILD_HASH << std::endl;
}

void show_module_info(std::string module_name) {

  auto info = modules_c(pdc->get_source_manager(), pdc->get_interpreter())
                  .get_module_info(module_name);

  std::cout << "Description: " << std::endl;
  if (info.description.has_value()) {
    std::cout << "  " << info.description.value() << std::endl;
  } else {
    std::cout << "  <none listed>\n";
  }

  std::cout << "Version: " << std::endl;
  if (info.version.has_value()) {
    std::cout << "  " << info.version.value() << std::endl;
  } else {
    std::cout << "  <none listed>\n";
  }

  std::cout << "Authors: " << std::endl;
  if (info.authors.has_value()) {
    for (auto &author : info.authors.value()) {
      std::cout << "  " << author << std::endl;
    }
  } else {
    std::cout << "  <none listed>\n";
  }

  std::cout << "Licenses: " << std::endl;
  if (info.licenses.has_value()) {
    for (auto &license : info.licenses.value()) {
      std::cout << "  " << license << std::endl;
    }
  } else {
    std::cout << "<none listed>\n";
  }

  std::cout << "Tests files: " << std::endl;
  if (info.test_files.has_value()) {
    std::cout << "  " << info.test_files.value().size() << " files present"
              << std::endl;
  } else {
    std::cout << "  <none listed>\n";
  }
}

void run_each_test(std::vector<std::filesystem::path> &files) {

  if (files.size() == 0) {
    std::cout << "No test files found" << std::endl;
    return;
  }

  std::cout << "Tests files: " << files.size() << std::endl;

  for (auto &test_file : files) {
    std::cout << "Running test file: " << test_file << std::endl;
    run_from_file(test_file);
    pdc->reset();

    std::cout << "COMPLETE\n" << std::endl;
  }

  // if we get here, all tests passed
  std::cout << "All tests passed!" << std::endl;
}

void run_local_tests_dir(std::filesystem::path &dir) {

  auto test_dir = dir / "tests";

  if (!std::filesystem::exists(test_dir)) {
    std::cout << "No tests directory found" << std::endl;
    return;
  }
  if (!std::filesystem::is_directory(test_dir)) {
    std::cout << "Suspected directory `tests` exists but is not a directory"
              << std::endl;
    return;
  }
  auto file_list = std::vector<std::filesystem::path>();
  for (auto &p : std::filesystem::directory_iterator(test_dir)) {
    if (p.is_regular_file()) {
      file_list.push_back(p.path());
    }
  }
  return run_each_test(file_list);
}

void run_tests(std::string &dir) {

  // Check if its an application first
  {
    auto fpd = std::filesystem::path(dir);
    auto app_entry = fpd / nibi::config::NIBI_APP_ENTRY_FILE_NAME;
    if (std::filesystem::exists(app_entry) &&
        std::filesystem::is_regular_file(app_entry)) {
      pdc->add_include_dir(fpd);
      return run_local_tests_dir(fpd);
    }
  }

  // Check non-installed module
  {
    auto fpd = std::filesystem::path(dir);
    try {
      fpd = std::filesystem::canonical(fpd);
    } catch (...) {
    }
    auto test_dir = fpd / nibi::config::NIBI_MODULE_FILE_NAME;
    if (std::filesystem::exists(test_dir) &&
        std::filesystem::is_regular_file(test_dir)) {
      if (fpd.has_parent_path()) {
        pdc->add_include_dir(fpd.parent_path());
      }
      pdc->add_include_dir(fpd);
      return run_local_tests_dir(fpd);
    }
  }

  // Check installed modules
  {
    auto info =
        modules_c(pdc->get_source_manager(), pdc->get_interpreter()).get_module_info(dir);
    pdc->reset();

    if (info.test_files.has_value()) {
      return run_each_test(info.test_files.value());
    }
  }
}

int main(int argc, char **argv) {

#if CALCULATE_EXECUTION_TIME
  auto app_start = std::chrono::high_resolution_clock::now();
#endif

  std::vector<std::string> unmatched;
  std::vector<std::filesystem::path> include_dirs;
  std::vector<std::string> args = std::vector<std::string>(argv + 1, argv + argc);

  pdc = std::make_unique<program_data_controller_c>(args, include_dirs);

  for (std::size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-h" || args[i] == "--help") {
      show_help();
      return 0;
    }

    if (args[i] == "-v" || args[i] == "--version") {
      show_version();
      return 0;
    }

    if (args[i] == "-t" || args[i] == "--test") {
      if (i + 1 >= args.size()) {
        std::cout << "No module name specified to test" << std::endl;
        return 1;
      }
      pdc->add_include_dir(std::filesystem::current_path());
      run_tests(args[i + 1]);
      return 0;
    }

    if (args[i] == "-i" || args[i] == "--include") {
      if (i + 1 >= args.size()) {
        std::cout << "No include directory specified" << std::endl;
        return 1;
      }
      std::stringstream ss(args[i + 1]);
      std::string item;
      while (std::getline(ss, item, ':')) {
        pdc->add_include_dir(item);
      }
      ++i;
      continue;
    }

    if (args[i] == "-m" || args[i] == "--module") {
      if (i + 1 >= args.size()) {
        std::cout << "No module name specified" << std::endl;
        return 1;
      }
      pdc->add_include_dir(std::filesystem::current_path());
      show_module_info(args[i + 1]);
      return 0;
    }

    unmatched.push_back(args[i]);
  }

  if (unmatched.empty()) {
    std::cerr << "No file or directory specified" << std::endl;
    return 1;
  }

  if (unmatched.size() > 1) {
    std::cerr << "Unmatched arguments:" << std::endl;
    for (auto &arg : unmatched) {
      std::cout << "  " << arg << std::endl;
    }
    return 1;
  }

  bool run_as_dir = false;
  if (std::filesystem::is_directory(unmatched[0])) {
    run_as_dir = true;
  } else if (std::filesystem::is_regular_file(unmatched[0])) {
    run_as_dir = false;
  } else {
    std::cout << "Invalid file or directory: " << unmatched[0] << std::endl;
    return 1;
  }

  // Ensure all include dirs exist
  for (auto &include_dir : include_dirs) {
    if (!std::filesystem::exists(include_dir) ||
        !std::filesystem::is_directory(include_dir)) {
      return 1;
    }
  }

  std::filesystem::path entry_file_path(unmatched[0]);
  entry_file_path = std::filesystem::canonical(entry_file_path);

  if (!run_as_dir && entry_file_path.has_parent_path()) {
    entry_file_path = entry_file_path.parent_path();
  }

  pdc->add_include_dir(entry_file_path);

#if CALCULATE_EXECUTION_TIME
  auto start = std::chrono::high_resolution_clock::now();
#endif

  if (run_as_dir) {
    run_from_dir(unmatched[0]);
  } else {
    run_from_file(unmatched[0]);
  }

#if CALCULATE_EXECUTION_TIME
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Execution time: " << duration.count() << "ms" << std::endl;
#endif
  return 0;
}
