#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <basic_data.hpp>


bool configuration::data::DataManager::AddConfig(const std::string& conf) {
  return json_scan(conf);
}

bool configuration::data::DataManager::IsValidString(std::string s) {
  return !rapidjson::Document().Parse(s.c_str()).HasParseError();
}

bool configuration::data::DataManager::json_scan(const std::string&) { return false; }



