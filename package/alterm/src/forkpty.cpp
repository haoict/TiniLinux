#include "../include/forkpty.hpp"

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "../include/Alterm.hpp"
#include "../include/AnsFilter.hpp"

#define BUFSIZE 2048

static char shell[] = "/bin/sh";

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
        // execlp("bash", "bash", "-l", NULL);

        char** args;
        char* envshell = getenv("SHELL");
        envshell = envshell ? envshell : shell;
        args = (char*[]){envshell, "-i", NULL};
        execlp(envshell, args[0], args[1], NULL);

        perror("execlp failed");
        exit(1);
    }

    return pid;
}

bool read_pty(int pty_fd, alterm* term_ptr, std::vector<std::string>& lines) {
    char buffer[BUFSIZE];
    int n = read(pty_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        std::string raw = buffer;

        // Process each character individually like the reference implementation
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            term_ptr->process_char(c);
        }

        // Only process for normal line mode if we're NOT in alternate screen
        if (!term_ptr->alternate_screen) {
            // Normal line-based mode for shell
            std::string cleaned = strip_osc_sequences(raw);

            // Handle carriage returns and line feeds properly
            size_t i = 0;
            while (i < cleaned.size()) {
                if (cleaned[i] == '\r') {
                    // Carriage return - go to beginning of current line
                    if (i + 1 < cleaned.size() && cleaned[i + 1] == '\n') {
                        // \r\n - move to next line (normal line ending)
                        i += 2;
                        lines.push_back("");
                    } else {
                        // Just \r - overwrite current line
                        i++;
                        if (!lines.empty()) {
                            lines.back().clear();
                        }
                    }
                } else if (cleaned[i] == '\n') {
                    // Line feed without carriage return
                    i++;
                    lines.push_back("");
                } else if (cleaned[i] == '\x08' || cleaned[i] == '\x7F') {
                    // Backspace character - remove last character from current line
                    i++;
                    if (!lines.empty() && !lines.back().empty()) {
                        lines.back().pop_back();
                    }
                } else if (cleaned[i] >= 32) {
                    // Printable character - add to current line
                    if (lines.empty()) {
                        lines.push_back("");
                    }
                    if (lines.back().size() >= BUFSIZE) {
                        lines.push_back("");
                    }
                    lines.back() += cleaned[i];
                    i++;
                } else {
                    // Skip other control characters
                    i++;
                }
            }

            int max_lines = term_ptr->settings_manager ? term_ptr->settings_manager->get_max_lines() : 64;
            term_ptr->trim_lines(max_lines, term_ptr);
        }

        return true;
    }

    return false;
}

// Function to detect if output contains a shell prompt (indicating command completion)
bool looks_like_shell_prompt(const std::string& line) {
    if (line.empty()) return false;

    // Common shell prompt patterns
    // Look for patterns like: user@host:path$ or user@host:path# or just $ or #
    if (line.find('$') != std::string::npos && line.back() == ' ') return true;
    if (line.find('#') != std::string::npos && line.back() == ' ') return true;

    // Look for patterns ending with typical prompt characters
    if (line.size() > 2) {
        char last_char = line[line.size() - 1];
        char second_last = line[line.size() - 2];

        if (last_char == ' ' && (second_last == '$' || second_last == '#' || second_last == '>')) {
            return true;
        }
    }

    return false;
}
