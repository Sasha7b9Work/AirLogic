
#include "mfprotopipe.h"
#include "mfproto.h"

mfProtoPipe::mfProtoPipe(const char *_name,
                         std::vector<mfProtoPhyPipe *> initMembers) {
  strncpy(name, _name, 31);
  instances.push_back(this);
  members.reserve(initMembers.size());
  members.clear();
  // pipeMutex.store(false);
  for (uint32_t i = 0; i < initMembers.size(); i++)
    insertMember(initMembers[i]);
}
mfProtoPipe::~mfProtoPipe() {
  instances.erase(std::find(instances.begin(), instances.end(), this));
}
void mfProtoPipe::pushIncomingToBus(mfProtoPhyPipe *sender, uint8_t *buffer,
                                    uint32_t len) {
  //  if (mfProto::getMutex(pipeMutex)) {
  for (uint32_t i = 0; i < members.size(); i++) {
    if (members[i] != sender) {
      members[i]->receiveHandler(buffer, len);
    }
  }
  mfProto::freeMutex(pipeMutex);
  //  }
}
void mfProtoPipe::insertMember(mfProtoPhyPipe *member) {
  members.push_back(member);
  member->writeToPipe =
      std::bind(&mfProtoPipe::pushIncomingToBus, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);
}
void mfProtoPipe::removeMember(mfProtoPhyPipe *member) {
  uint32_t index = members.size();
  for (uint32_t i = 0; i < members.size(); i++)
    if (members[i] == member) {
      index = i;
      break;
    }
  if (index < members.size()) {
    member->writeToPipe = nullptr;
    members.erase(members.begin() + index);
  }
}
