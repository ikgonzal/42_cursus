#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define WRONG_ARG_MSG "Wrong number of arguments\n"
#define FATAL_ERROR_MSG "Fatal error\n"
#define IP_ADDRESS 2130706433 // 127.0.0.1
#define BACKLOG_SIZE 128 // Maximum number of clients to listen simultaneously (the surpluss will join a queue)

// File descriptor sets for reading, writting and master
fd_set  fds, rfds, wfds;
int     fdmax = 0; // Maximum file descriptor number
int     id[10000]; // Array to store client IDs
int     clients = 0; // Number of clients
char*   msg[10000]; // Array to store messages from clients
char    rbuf[1024]; // Buffer to read client messages
char    wbuf[42]; // Buffer to write server messages

// Function to send a message to all clients except the sender
void notify(int from, char *msg)
{
    for (int fd = 0; fd <= fdmax; fd++)
        if(FD_ISSET(fd, &wfds) && fd != from)
            send(fd, msg, strlen(msg), 0);
}

// Function to add a new client
void add_client(int fd)
{
    fdmax = fd > fdmax ? fd : fdmax; // Update maximum file descriptor number
    id[fd] = clients++; // Assign an ID to the new client
    msg[fd] = NULL; // Initialize the client's messages to NULL
    FD_SET(fd, &fds); // Add the new client to the master set
    sprintf(wbuf, "server: client %d just arrived\n", id[fd]); // Prepare the server message
    notify(fd, wbuf); // Notify all clients about the new client

}

// Function to remove a client
void remove_client(int fd)
{
    sprintf(wbuf, "server: client %d just left\n,", id[fd]); // Prepare the server message
    notify(fd, wbuf); // Notify all the clients about the client leaving
    free(msg[fd]); // Free the memory allocated for the client's message
    msg[fd] = NULL; // Set the client's message to NULL
    FD_CLR(fd, &fds); // Remove the client from the master set
    close(fd); // Close the client's socket
}

// Function to concatenate two strings
char* str_join(char *buf, char *add)
{
    // Calculate the length of the new string
    size_t len = (buf ? strlen(buf) : 0) + strlen(add) + 1;

    // Allocate memory for the new string
    char *newbuf = malloc(len * sizeof(*newbuf));
    if (!newbuf)
        return NULL;

    // Copy the original string and the additional string into the new string
    if (buf)
        strcpy(newbuf, buf);
    else
        newbuf[0] = '\0';
    strcat(newbuf, add);

    // Free the original string
    free(buf);

    return newbuf;
}

// Function to extract a message from a buffer
int extract_message(char **buf, char **msg)
{
    char *newbuf;
    int i;

    *msg = 0;
    if (*buf = 0)
        return (0);
    i = 0;
    while((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                reutnr (-1);
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        write(2, WRONG_ARG_MSG, strlen(WRONG_ARG_MSG));
        exit(1);
    }

    //Initialize the master set of fds
    FD_ZERO(&fds);

    // Create a socket
    fdmax = socket(AF_INET, SOCK_STREAM, 0);
    if (fdmax < 0)
    {
        write (2, FATAL_ERROR_MSG, strlen(FATAL_ERROR_MSG));
        exit(1);
    }
    FD_SET(fdmax, &fds); // Add the socket to the master set
    int sockfd = fdmax; // Store the socket descriptor

    // Define the server address
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(IP_ADDRESS); // 127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1])); // port to bind to in the IP address specified

    // Bind the socket to the server address and start listening for connections
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) || listen(sockfd, BACKLOG_SIZE))
    {
        write(2, FATAL_ERROR_MSG, strlen(FATAL_ERROR_MSG));
        exit(1);
    }

    // Main loop
    while (1)
    {
        // Copy the master set to the read and write sets
        rfds = wfds = fds;

        // Wait for an event on one of the socket
        if (select(fdmax + 1, &rfds, &wfds, NULL, NULL) < 0)
        {
            write(2, FATAL_ERROR_MSG, strlen(FATAL_ERROR_MSG));
            exit(1);
        }

        // Check all sockets for events
        for (int fd = 0; fd <= fdmax; fd++)
        {
            // Skip if there are no events on the socket
            if (!FD_ISSET(fd, &rfds))
                continue;

            // If there is a new connection
            if (fd == sockfd)
            {
                socklen_t len = sizeof(servaddr);
                int client = accept(sockfd, (struct sockaddr *)&servaddr, &len);
                if (client >= 0)
                {
                    add_client(client); // Add the new client
                    break;
                }
            }
            // If there is data from a client
            else
            {
                int ret = recv(fd, rbuf, 1024, 0);
                if (ret <= 0)
                {
                    remove_client(fd);
                    break;
                }
                rbuf[ret] = '\0';
                msg[fd] = str_join(msg[fd], rbuf); // Append the received data to the client's message
                
                char *msg;
                // Extract all messages from the client's data
                while(extract_message(&(msg[fd]), &msg))
                {
                    sprintf(wbuf, "client %d: ", id[fd]); // Prepare the server message
                    notify(fd, wbuf);
                    notify(fd, msg);
                    free(msg);
                    msg = NULL;
                }

            }
        }
    }
    return (0);
}