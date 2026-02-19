#include "Common.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

std::string send_command(const std::string& cmd) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return "ERR|Socket Init";

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(fd);
        return "ERR|Connection Refused (Is daemon running?)";
    }

    write(fd, cmd.c_str(), cmd.length());

    char buffer[1024] = {0};
    int rc = read(fd, buffer, sizeof(buffer));
    close(fd);

    if (rc > 0) return std::string(buffer, rc);
    return "ERR|Empty Response";
}