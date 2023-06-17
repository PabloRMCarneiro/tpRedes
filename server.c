#include "common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

typedef struct equipment
{
  int id;
  int installed;
  int socket;
} equipment;

typedef struct control
{
  char message[BUFSZ];
  int send_server;
} control;

int equipmentCount = 0;
equipment equipments[MAX_EQUIPMENT];

control commandInterpreter(char *command)
{
  // close connection -> fecha a conexão do proprio servidor
  // list equipment -> lista os equipamentos conectados

  control serverCommand;
  if(strcmp(command, "close connection") == 0)
  {
    serverCommand.send_server = 1;
    // verificar se tem conexão com algum peer na rede
    // se sim desconectar de todos
    // se não imprimir "No peer connected to close connection"
  }
  else if(strcmp(command, "list equipment") == 0)
  {
    for(int i = 0; i < MAX_EQUIPMENT; i++)
    {
      if(equipments[i].installed)
        printf("%02d ", i+1);
    }
  }
  else
  {
    serverCommand.send_server = 0;
    sprintf(serverCommand.message, "Mensagem não identificada: %s\n", command);
  }
  return serverCommand;
}

void initializeEquipments()
{
  for (int i = 0; i < MAX_EQUIPMENT; i++)
  {
    equipments[i].installed = 0;
    equipments[i].id = i + 1;
    equipments[i].socket = 0;
  }
}

typedef struct response
{
  char message[BUFSZ];
  int endConnection;
} response;

response exitHandler(char *message)
{
  response response;
  memset(response.message, 0, BUFSZ);
  response.endConnection = 1;
  return response;
}

response resolveHandler(char *message)
{
  response response;
  strcpy(response.message, message);
  response.endConnection = 0;
  return response;
}

response addNewEquipment(int clientSocket)
{
  int newId;
  for (int i = 0; i < MAX_EQUIPMENT; i++)
  {
    if (!equipments[i].installed)
    {
      newId = equipments[i].id;
      equipments[i].installed = 1;
      equipments[i].socket = clientSocket;
      equipmentCount++;
      break;
    }
  }

  printf("Equipment %d added\n", newId);

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  sprintf(msg, "%02d %02d\n", RES_ADD, newId);
  for (int i = 0; i < MAX_EQUIPMENT; i++)
  {
    if (equipments[i].installed)
    {
      sendMessage(equipments[i].socket, msg);
    }
  }

  usleep(1000); // Sleep between messages

  memset(msg, 0, BUFSZ);
  sprintf(msg, "%02d", RES_LIST);
  for (int i = 0; i < MAX_EQUIPMENT; i++)
  {
    if (equipments[i].installed)
    {
      char installedId[BUFSZ];
      sprintf(installedId, " %02d", equipments[i].id);
      strcat(msg, installedId);
    }
  }
  strcat(msg, "\n");
  sendMessage(equipments[newId - 1].socket, msg);

  return resolveHandler(msg);
}

response removeEquipment(int id)
{
  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  if (!equipments[id - 1].installed)
  {
    sprintf(msg, "%02d %02d", ERROR, EQUIPMENT_NOT_FOUND);
    sendMessage(equipments[id - 1].socket, msg);
    return exitHandler("Error");
  }

  equipments[id - 1].installed = 0;
  equipmentCount--;

  sprintf(msg, "%02d %02d %02d", OK, id, 1);
  sendMessage(equipments[id - 1].socket, msg);
  printf("Equipment %02d removed\n", id);

  for (int i = 0; i < MAX_EQUIPMENT; i++)
  {
    if (equipments[i].installed)
    {
      sprintf(msg, "%02d %02d", REQ_REM, id);
      sendMessage(equipments[i].socket, msg);
    }
  }

  return exitHandler("Success");
}

int isValidId(int id)
{
  if (id < 0)
    return 0;
  if (id > MAX_EQUIPMENT)
    return 0;
  return 1;
}

response requestInformation(int sourceId, int destineId)
{
  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  if (!isValidId(sourceId) || !equipments[sourceId - 1].installed)
  {
    printf("Equipment %02d not found\n", sourceId);
    sprintf(msg, "%02d %02d", ERROR, EQUIPMENT_SOURCE_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  if (!isValidId(destineId) || !equipments[destineId - 1].installed)
  {
    printf("Equipment %02d not found\n", destineId);
    sprintf(msg, "%02d %02d", ERROR, EQUIPMENT_TARGET_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  sprintf(msg, "%02d %02d %02d", REQ_INF, sourceId, destineId);
  sendMessage(equipments[destineId - 1].socket, msg);
  return resolveHandler("Success");
}

response informationResult(int sourceId, int destineId, float temperature)
{
  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  if (!isValidId(sourceId) || !equipments[sourceId - 1].installed)
  {
    printf("Equipment %02d not found\n", sourceId);
    sprintf(msg, "%02d %02d", ERROR, EQUIPMENT_SOURCE_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  if (!isValidId(destineId) || !equipments[destineId - 1].installed)
  {
    printf("Equipment %02d not found\n", destineId);
    sprintf(msg, "%02d %02d", ERROR, EQUIPMENT_TARGET_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  sprintf(msg, "%02d %02d %02d %.2f", RES_INF, sourceId, destineId, temperature);
  sendMessage(equipments[destineId - 1].socket, msg);
  return resolveHandler("Success");
}

response handleCommands(char *buf, int clientSocket)
{
  char *splittedCommand = strtok(buf, " ");
  int commandId = atoi(splittedCommand);
  int sourceId = 0, destineId = 0;

  switch (commandId)
  {
  case REQ_ADD:
    return addNewEquipment(clientSocket);
  case REQ_REM:
    splittedCommand = strtok(NULL, " ");
    return removeEquipment(atoi(splittedCommand));
  case REQ_INF:
    splittedCommand = strtok(NULL, " ");
    sourceId = atoi(splittedCommand);
    splittedCommand = strtok(NULL, " ");
    destineId = atoi(splittedCommand);
    return requestInformation(sourceId, destineId);
  case RES_INF:
    splittedCommand = strtok(NULL, " ");
    sourceId = atoi(splittedCommand);
    splittedCommand = strtok(NULL, " ");
    destineId = atoi(splittedCommand);
    splittedCommand = strtok(NULL, " ");
    float temperature = atof(splittedCommand);
    return informationResult(sourceId, destineId, temperature);
  default:
    break;
  }

  printf("Unknown message: %s\n", buf);
  return exitHandler("ERROR");
}

int main(int argc, char *argv[])
{
  if( argc != 4)
  {
    printf("Usage: %s <ip> <PeerPort> <ClientPort>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct sockaddr_storage storage;
  server_sockaddr_init(argv[3], &storage);

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1)
  {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != bind(s, addr, sizeof(storage)))
  {
    logexit("bind");
  }

  if (0 != listen(s, 10))
  {
    logexit("listen");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  initializeEquipments();
  fd_set read_fds, master_fds;
  int fdmax;
  FD_ZERO(&master_fds);
  FD_ZERO(&read_fds);
  FD_SET(s, &master_fds);
  fdmax = s;

  while (1)
  {
    


    read_fds = master_fds;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
    {
      logexit("select");
    }

    for (int i = 0; i <= fdmax; i++)
    {
          


      if (FD_ISSET(i, &read_fds))
      {
       

        if (i == s)
        {
          struct sockaddr_storage cstorage;
          struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
          socklen_t caddrlen = sizeof(cstorage);

          int csock = accept(s, caddr, &caddrlen);
          if (csock == -1)
          {
            logexit("accept");
          }

          FD_SET(csock, &master_fds);
          if (csock > fdmax)
          {
            fdmax = csock;
          }

          char caddrstr[BUFSZ];
          addrtostr(caddr, caddrstr, BUFSZ);
          printf("New connection from %s on socket %d\n", caddrstr, csock);
        }
        else
        {          
          char buf[BUFSZ];
          memset(buf, 0, BUFSZ);
          int nbytes = recv(i, buf, sizeof(buf), 0);
          buf[strcspn(buf, "\n")] = 0;

          if (nbytes <= 0)
          {
            if (nbytes == 0)
            {
              printf("Socket %d hung up\n", i);
            }
            else
            {
              perror("recv");
            }

            close(i);
            FD_CLR(i, &master_fds);
          }
          else
          {
            response res = handleCommands(buf, i);
            if (res.endConnection)
            {
              close(i);
              FD_CLR(i, &master_fds);
            }
          }
        }
      }

      
    }
  }
  exit(EXIT_SUCCESS);
}
