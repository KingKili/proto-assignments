#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include <time.h>
#include "Filesystem.h"
#include "Logger.h"
#include "FileServerClient.h"
#include "WorkingSet.h"
#include "ClientsToDo.h"

class FileServerUser
{
public:
  const std::string USERNAME;
  const std::string PASSWORD;
  FilesystemServer *fS;

  FileServerUser(std::string username, std::string password);
  FileServerUser();
  ~FileServerUser();

  FileServerClient *getClient(unsigned int clientId);
  void addClient(FileServerClient *client);
  void removeClient(FileServerClient *client);
  void clanupClients();
  bool isEmpty();
  ClientToDo *getClientToDo(unsigned int clientId);

private:
  std::unordered_map<unsigned int, FileServerClient *> clients;
  std::mutex *clientsMutex;
  ClientsToDo clientsToDo;

  void deleteAllClients();
};