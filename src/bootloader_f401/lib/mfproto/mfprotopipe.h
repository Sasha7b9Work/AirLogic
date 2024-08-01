#ifndef MF_PROTO_PIPE_H
#define MF_PROTO_PIPE_H

#include "mfprotophypipe.h"

#ifdef STM32_CORE
#include <wiring.h>
#else
#include <ctime>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include <functional>
#include <vector>
class mfProtoPipe {
private:
  char *name = (char *)nullptr; // object name, filled by MFPROTO define, can be
                                // used for debug purposes
  std::vector<mfProtoPhyPipe *> members;
  std::function<void(mfProtoPhyPipe *, uint8_t *, uint32_t)> _writeToPipe;
  atomic_bool pipeMutex = ATOMIC_FLAG_INIT;

public:
#undef __inst__
#define __inst__ std::vector<mfProtoPipe *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;

  mfProtoPipe(const char *_name, std::vector<mfProtoPhyPipe *> initMembers);
  void insertMember(mfProtoPhyPipe *member);
  void removeMember(mfProtoPhyPipe *member);
  void pushIncomingToBus(mfProtoPhyPipe *sender, uint8_t *buffer, uint32_t len);
  ~mfProtoPipe();
};

#define MFPROTOPIPE(a, ...) mfProtoPipe(a)(#a, __VA_ARGS__)
#endif