#include "../include/forkpty.hpp"

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "../include/Alterm.hpp"
#include "../include/AnsFilter.hpp"

std::vector<std::string> lines;

// start shell with subprocess (child process) and connect to it via pty
// (master_fd)

int start_shell_with_pty(int& master_fd) {
    pid_t pid = forkpty(&master_fd, nullptr, nullptr,
                        nullptr);  // master_fd is the file descriptor of the
                                   // pty, using forkpty to create the pty.

    if (pid == -1) {
        std::cerr << "forkpty failed" << std::endl;
        return -1;
    }

    if (pid == 0) {
        // child process
        // IF YOU WANT TO DISABLE THE ECHO MODE, REMOVE THE COMMENT BELOW

        // struct termios tty;
        // tcgetattr(STDIN_FILENO, &tty);
        // tty.c_lflag &= ~ECHO;  // Disable echo mode
        // tcsetattr(STDIN_FILENO, TCSANOW, &tty);  // set the terminal
        // attributes to the same as the parent process.

        // Set up terminal environment for full color support
        setenv("TERM", "xterm-256color", 1);  // Full color terminal support
        setenv("COLORTERM", "truecolor", 1);  // Enable true color support
        setenv("CLICOLOR", "1", 1);           // Force color output for some programs
        setenv("CLICOLOR_FORCE", "1", 1);     // Force color even if not a tty

        // Start bash as a login shell to load .bash_profile, .bashrc, etc.
        execlp("bash", "bash", "-l", NULL);
        // execlp("bash", "bash", "--norc", "--noprofile", NULL);

        perror("execlp failed");
        exit(1);
    }

    return pid;
}

bool read_pty(int pty_fd, alterm* term_ptr, std::vector<std::string>& lines) {
    char buffer[BUFSIZ];  // buffer to store the read data, and it char[256]
                          // because the function read() deal with only char
                          // array.
    int n = read(pty_fd, buffer,
                 sizeof(buffer) - 1);  // we subtract 1 because we want to add the
                                       // null terminator at the end of the buffer.
    if (n > 0) {
        buffer[n] = '\0';  // \0 represents the null terminator.
        std::string raw = buffer;

        std::string cleaned = strip_osc_sequences(raw);  // Keep ANSI for rendering

        // Handle carriage returns and line feeds properly
        size_t i = 0;
        while (i < cleaned.size()) {
            if (cleaned[i] == '\r') {
                // Carriage return - go to beginning of current line
                if (i + 1 < cleaned.size() && cleaned[i + 1] == '\n') {
                    // \r\n - move to next line (normal line ending)
                    i += 2;
                    lines.push_back("");  // Add new empty line
                } else {
                    // Just \r - overwrite current line
                    i++;
                    if (!lines.empty()) {
                        lines.back().clear();  // Clear the current line for overwriting
                    }
                }
            } else if (cleaned[i] == '\n') {
                // Line feed without carriage return
                i++;
                lines.push_back("");  // Add new empty line
            } else {
                // Regular character - add to current line
                if (lines.empty()) {
                    lines.push_back("");
                }
                lines.back() += cleaned[i];
                i++;
            }
        }

        term_ptr->trim_lines(1000, term_ptr);

        return true;
    }

    return false;
}
