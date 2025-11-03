#include <fcntl.h>
#include <pty.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "AnsFilter.hpp"

class alterm;

int start_shell_with_pty(int& master_fd);

bool read_pty(int pty_fd, alterm* term_ptr, std::vector<std::string>& lines);

// Function to detect if output contains a shell prompt (indicating command completion)
bool looks_like_shell_prompt(const std::string& line);