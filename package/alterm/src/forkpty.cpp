#include "../include/forkpty.hpp"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include "../include/Alterm.hpp"

std::vector<std::string> lines;




// start shell with subprocess (child process) and connect to it via pty (master_fd)

int start_shell_with_pty(int &master_fd){
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);  // master_fd is the file descriptor of the pty, using forkpty to create the pty.

    if(pid == -1){
        std::cerr << "forkpty failed" << std::endl;
        return -1;
    }

    if(pid == 0){
        // child process
        //IF YOU WANT TO DISABLE THE ECHO MODE, REMOVE THE COMMENT BELOW
        
       // struct termios tty;
        //tcgetattr(STDIN_FILENO, &tty);
        //tty.c_lflag &= ~ECHO;  // Disable echo mode
        //tcsetattr(STDIN_FILENO, TCSANOW, &tty);  // set the terminal attributes to the same as the parent process. 

        execlp("bash", "bash", "--norc", "--noprofile", NULL);
        perror("execlp failed");
        exit(1);
    }

    return pid;
}


bool read_pty(int pty_fd, alterm* term_ptr, std::vector<std::string>& lines){

    char buffer[256];                                                   //buffer to store the read data, and it char[256] because the function read() deal with only char array.                       
    int n = read(pty_fd, buffer, sizeof(buffer) - 1);   //we subtract 1 because we want to add the null terminator at the end of the buffer.
     if(n > 0){
        buffer[n] = '\0';                                              // \0 represents the null terminator.
        std::string raw = buffer;

        std::string cleaned = strip_osc_sequences(strip_ansi_sequences(raw));

        size_t pos = 0;
        while((pos = cleaned.find("\n")) != std::string::npos){
            std::string line = cleaned.substr(0, pos);
            lines.push_back(line);                                //every line is stored in a vector of strings.
            cleaned.erase(0, pos + 1);
        }

        if(!cleaned.empty()){                                        //if the chunk is not empty, and their are no lines ending with \n, we add the chunk to the vector.
            lines.push_back(cleaned);
        }
        
        term_ptr->trim_lines(1000, term_ptr);
        

        return true;
    }

    return false;
}
        
