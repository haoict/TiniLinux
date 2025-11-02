#include "../include/SettingsManager.hpp"
#include <cctype>
#include <iterator>
#include <sstream>
#include <fstream>
#include <iostream>


SettingsManager::SettingsManager() {
    this->Settings["font_size"] = "14";
    this->Settings["bg_opacity"] = "50";
    this->Settings["bg_color_r"] = "0";
    this->Settings["bg_color_g"] = "0";
    this->Settings["bg_color_b"] = "0";
    this->Settings["font_color_r"] = "255";
    this->Settings["font_color_g"] = "255";
    this->Settings["font_color_b"] = "255";
    this->Settings["font_family"] = "/usr/share/fonts/SourceCodePro-Regular.ttf";
    this->Settings["max_history"] = "100";
    this->Settings["auto_scroll"] = "true";
}

std::string trim(const std::string& str){
    auto start = str.begin();
    while(start != str.end() && std::isspace(*start)) ++start;

    auto end = str.end();
    do{
        --end;
    }while(std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}


void SettingsManager::set(const std::string& key, const std::string& value){
    this->Settings[key] = value;
}


const std::string& SettingsManager::get(const std::string& key) const{
    static const std::string empty = "";
    auto it = this->Settings.find(key);
    return it != this->Settings.end() ? it->second : empty;
}

std::vector<std::string> SettingsManager::render_all_settings()const{
    std::vector<std::string> result;
    for(const auto& [key, value] : this->Settings){
        result.push_back(key + " = " + value );
    }
    return result;
}

void SettingsManager::save_to_file(const std::string& Path){
    std::ofstream Out(Path);
    if(!Out.is_open()) return;
    for(const auto& [key, value] : this->Settings){
        if(!key.empty() && !value.empty())
            Out << key << "=" << value << std::endl;
    }
    Out.close();
}


void SettingsManager::load_from_file(const std::string& Path){
    std::ifstream In(Path);
    if(!In.is_open()) return;

    std::string Line;
    while(std::getline(In, Line)){
        size_t eq_pos = Line.find('=');
        if(eq_pos == std::string::npos) continue;

        std::string key = trim(Line.substr(0, eq_pos));
        std::string value = trim(Line.substr(eq_pos + 1));

        if(!key.empty() && !value.empty())
            Settings[key] = value;
    }
    
    In.close();
}


std::vector<std::string> SettingsManager::apply_command(const std::string& Cmd){
    std::vector<std::string> result;
    std::istringstream iss(Cmd);
    std::string word;
    iss >> word;
    if(word == "set"){
        std::string key, value;
        iss >> key >> value;
        if(!key.empty() && !value.empty()){
            set(key, value);
            result.push_back("[settings] update: " + key + " = " + value);;
        }else{
            result.push_back("[settings] usage: set <key> <value>");
        }
    }

    else if(word == "save"){
        save_to_file(".alterm_settings");
        result.push_back("[settings] saved to .alterm_settings");
    }
    else if(word == "help"){
        result.push_back("[settings] usage: set <key> <value>        -Set a settings");
        result.push_back("[settings] usage: save                     -Save to file");
        result.push_back("[settings] usage: exit                     -Exit settings");
    }
    else{
        result.push_back("[settings] unknown command");
    }
    return result;
}