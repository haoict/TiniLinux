#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include "AnsFilter.hpp"

class alterm;


int start_shell_with_pty(int& master_fd);

bool read_pty(int pty_fd, alterm* term_ptr, std::vector<std::string>& lines);