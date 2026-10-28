#pragma once

#include <exception>
#include <map>
#include <string>

std::string stripHtml(const std::string& html);
std::string truncateUtf8(const std::string& s, size_t maxChars);
std::string describeException(std::exception_ptr e);
std::string trim(const std::string& s);
std::string percentEncode(const std::string& s);

std::map<std::string, std::string> loadEnv(const std::string& path);
std::string envOr(const std::map<std::string, std::string>& file, const std::string& key,
                  const std::string& fallback = "");
