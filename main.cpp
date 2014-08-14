#include "encoder/EncoderParameter.h"
#include "encoder/EncoderHandler.h"

int main(int argc, char *argv[])
{
  //CUDA --resolution XxY --output ""
  if (argc < 5) {
    EncoderHandler::printHelp();
    return -1;
  }
  EncoderParameter encParameter;
  if (!encParameter.setEncoderParameter(argc, argv)) {
    EncoderHandler::printHelp();
    return -1;
  }
  EncoderHandler handler;
  handler.configureEncoder(&encParameter);
  return handler.startEncoding();
}
