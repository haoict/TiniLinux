#include "../include/HistoryManager.hpp"


void load_history(std::vector<std::string>& History, const std::string& FilePath){
    std::ifstream file(FilePath);
    std::string line;
    while (std::getline(file, line)) {
        if(!line.empty())
            History.push_back(line);
    }
    file.close();
}


void append_history(const std::string &Command, const std::string &FilePath){
    std::ofstream file(FilePath, std::ios::app);
    if(file.is_open()){
        file << Command << "\n";
    }
}


void save_trimmed_history(const std::vector<std::string> &History, const std::string &FilePath, size_t MaxSize){
    std::ofstream file(FilePath);
    if(!file) return;

    size_t start = History.size() > MaxSize ? History.size() - MaxSize : 0;
    for(size_t i = start; i < History.size(); ++i){
        file << History[i] << "\n";
    }
    file.close();
}