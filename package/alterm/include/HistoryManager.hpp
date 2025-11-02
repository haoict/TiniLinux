#ifndef __HISTORY_MANAGER_HPP__
#define __HISTORY_MANAGER_HPP__

#include <string>
#include <vector>
#include <fstream>


void load_history(std::vector<std::string>& History, const std::string& FilePath);

void append_history(const std::string& Command, const std::string& FilePaths);

void save_trimmed_history(const std::vector<std::string>& History, const std::string& FilePath, size_t MaxSize);

#endif 