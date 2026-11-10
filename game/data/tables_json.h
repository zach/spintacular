// Reads the balance tables from JSON (assets/data/tables.json).
#pragma once

#include <string>

#include "rules/tables.h"

namespace spin {

// Throws std::runtime_error with the reason when the text isn't a valid table.
Tables tables_from_json(const std::string& text);
Tables load_tables(const std::string& path);

// Sets one number in a tables JSON text by path, such as "round.tank=14", and returns the text.
std::string override_tables_json(const std::string& text, const std::string& assignment);

std::string read_file(const std::string& path);
bool write_file(const std::string& path, const std::string& text);

} // namespace spin
