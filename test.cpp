#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

int main(int argc, char *argv[])
{
    std::string s="8\r\ntesttest\r\n";
    int size=std::stoi(s);
    printf("%d\n",size);
}

// void f()
// {
//     // assume socket is already connected
//     char buffer[1024]; // buffer for storing chunks
//     int bytesReceived; // number of bytes received
//     int chunkSize;     // size of current chunk
//     std::string data;  // string for storing complete data

//     while (true)
//     {
//         // receive chunk size
//         bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
//         if (bytesReceived <= 0)
//             break;                     // error or end of transmission
//         buffer[bytesReceived] = '\0';  // null-terminate buffer
//         chunkSize = std::stoi(buffer); // convert buffer to integer
//         if (chunkSize == 0)
//             break; // empty chunk means end of data

//         // receive chunk data
//         bytesReceived = recv(socket, buffer, min(chunkSize, sizeof(buffer)), 0);
//         if (bytesReceived <= 0)
//             break;                    // error or end of transmission
//         buffer[bytesReceived] = '\0'; // null-terminate buffer
//         data += buffer;               // append buffer to data string
//     }
// }