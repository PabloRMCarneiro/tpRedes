#include "common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_TEMPERATURE 10

int equipmentId = 0;
int serverSocket;
int equipmentsInstalled[MAX_EQUIPMENT];

void initializeEquipments()
{
    for (int i = 0; i < MAX_EQUIPMENT; i++)
    {
        equipmentsInstalled[i] = 0;
    }
}

typedef struct control
{
    char message[BUFSZ];
    int send_server;
} control;

control commandInterpreter(char *command)
{
    control serverCommand;
    if (strcmp(command, "close connection") == 0)
    {
        serverCommand.send_server = 1;
        sprintf(serverCommand.message, "%d %d", REQ_REM, equipmentId);
    }
    else if (strcmp(command, "list equipment") == 0)
    {
        serverCommand.send_server = 0;
        for (int i = 0; i < MAX_EQUIPMENT; i++)
        {
            if (equipmentsInstalled[i] && equipmentId != (i + 1))
                printf("%02d ", i + 1);
        }
        printf("\n");
    }
    else if (strncmp(command, "request info from ", strlen("request info from ")) == 0)
    {
        strtok(command, " ");
        strtok(NULL, " ");
        strtok(NULL, " ");
        char *value = strtok(NULL, " ");
        int destineId = atoi(value);
        serverCommand.send_server = 1;
        sprintf(serverCommand.message, "%d %d %d", REQ_INF, equipmentId, destineId);
    }
    else
    {
        serverCommand.send_server = 0;
        sprintf(serverCommand.message, "Mensagem não identificada: %s\n", command);
    }

    return serverCommand;
}

float getTemperature()
{
    return (float)rand() / (float)(RAND_MAX / MAX_TEMPERATURE);
}

void requestedInformation(int sourceId, int destineId)
{
    char msg[BUFSZ];
    memset(msg, 0, BUFSZ);
    printf("Equipment %d requested information\n", sourceId);
    sprintf(msg, "%02d %02d %02d %.2f", RES_INF, destineId, sourceId, getTemperature());
    sendMessage(serverSocket, msg);
}

void handleResponse(char *response)
{
    char *splittedResponse = strtok(response, " ");
    int messageId = atoi(splittedResponse);
    int sourceId = 0, destineId = 0;

    switch (messageId)
    {
    case REQ_REM:
        splittedResponse = strtok(NULL, " ");
        int removedId = atoi(splittedResponse);
        equipmentsInstalled[removedId - 1] = 0;
        printf("Equipment %02d removed\n", removedId);
        return;
    case RES_ADD:
        splittedResponse = strtok(NULL, " ");
        int receivedId = atoi(splittedResponse);
        equipmentsInstalled[receivedId - 1] = 1;
        if (equipmentId == 0)
        {
            equipmentId = receivedId;
            printf("New ID: %0d\n", equipmentId);
        }
        else
        {
            printf("Equipment %02d added\n", receivedId);
        }
        return;
    case RES_LIST:
        splittedResponse = strtok(NULL, " ");
        while (splittedResponse != NULL)
        {
            equipmentsInstalled[atoi(splittedResponse) - 1] = 1;
            splittedResponse = strtok(NULL, " ");
        }
        return;
    case REQ_INF:
        splittedResponse = strtok(NULL, " ");
        sourceId = atoi(splittedResponse);
        splittedResponse = strtok(NULL, " ");
        destineId = atoi(splittedResponse);
        requestedInformation(sourceId, destineId);
        return;
    case RES_INF:
        splittedResponse = strtok(NULL, " ");
        sourceId = atoi(splittedResponse);
        splittedResponse = strtok(NULL, " ");
        destineId = atoi(splittedResponse);
        splittedResponse = strtok(NULL, " ");
        float temperature = atof(splittedResponse);
        printf("Value from %d: %.2f\n", sourceId, temperature);
        return;
    case ERROR:
        splittedResponse = strtok(NULL, " ");
        int errorCode = atoi(splittedResponse);
        printf("%s\n", ERROR_MESSAGES[errorCode - 1]);
        if (errorCode == EQUIPMENT_LIMIT_EXCEEDED)
        {
            close(serverSocket);
            exit(EXIT_SUCCESS);
        }
        return;
    case OK:
        splittedResponse = strtok(NULL, " ");
        int okMessage = atoi(splittedResponse) - 1;
        printf("%s\n", OK_MESSAGES[okMessage]);

        close(serverSocket);
        exit(EXIT_SUCCESS);
    default:
        break;
    }

    printf("Unknown response: %s\n", response);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    struct sockaddr_storage storage;
    addrparse(argv[1], argv[2], &storage);

    serverSocket = socket(storage.ss_family, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(serverSocket, addr, sizeof(storage)))
    {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    initializeEquipments();

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "%d", REQ_ADD);
    sendMessage(serverSocket, buf);

    fd_set read_fds, master_fds;
    int fdmax;
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
    FD_SET(serverSocket, &master_fds);
    FD_SET(STDIN_FILENO, &master_fds);
    fdmax = (serverSocket > STDIN_FILENO) ? serverSocket : STDIN_FILENO;

    while (1)
    {
        read_fds = master_fds;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            logexit("select");
        }

        if (FD_ISSET(serverSocket, &read_fds))
        {
            char buf[BUFSZ];
            memset(buf, 0, BUFSZ);
            int nbytes = recv(serverSocket, buf, sizeof(buf), 0);
            buf[strcspn(buf, "\n")] = 0;

            if (nbytes <= 0)
            {
                if (nbytes != 0)
                {
                    perror("recv");
                }

                close(serverSocket);
                exit(EXIT_FAILURE);
            }
            else
            {
                handleResponse(buf);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            char buf[BUFSZ];
            memset(buf, 0, BUFSZ);
            fgets(buf, BUFSZ - 1, stdin);
            buf[strcspn(buf, "\n")] = 0;
            control serverCommand = commandInterpreter(buf);
            if (serverCommand.send_server)
            {
                sendMessage(serverSocket, serverCommand.message);
            }
        }
    }

    close(serverSocket);
    exit(EXIT_SUCCESS);
}
