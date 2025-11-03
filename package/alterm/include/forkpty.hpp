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