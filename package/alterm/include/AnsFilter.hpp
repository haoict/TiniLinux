#pragma once
#include <string>


std::string strip_ansi_sequences(const std::string& input);
std::string strip_osc_sequences(const std::string& input);