#ifndef SETTINGS_HPP
#define SETTINGS_HPP
#include <string>
#include <map>
#include <vector>

class SettingsManager{

    private:
    std::map<std::string, std::string> Settings;

    public:
    SettingsManager();

    std::vector<std::string> apply_command(const std::string& Cmd);
    void load_from_file(const std::string& Path);
    void save_to_file(const std::string& Path);
    const std::string& get(const std::string& Key) const;
    void set(const std::string& Key, const std::string& Value);
    std::vector<std::string> render_all_settings() const;

};
#endif 