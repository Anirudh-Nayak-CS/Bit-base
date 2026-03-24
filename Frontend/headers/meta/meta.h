#include "common.h"
#include <string>

class MetaCommandHandler
{
public:
  Metastatus handleMetaCommands(const std::string &meta_command);
};