#include "circular_buffer.hpp"

namespace base {

CircularBuffer::CircularBuffer(int slots) {
  if (slots > 0)
    bufferSlots = slots;
  else
    bufferSlots = 10;
  elements = new struct bufferElement[bufferSlots];
  for (int i=0; i<bufferSlots; i++) {
    elements[i].readstatus = isread;
  }
  readPtr = elements;
  writePtr = elements;
}

CircularBuffer::~CircularBuffer() {
  delete[] elements;
}

void CircularBuffer::write(int value) {
  struct bufferElement* tempPtr;
  tempPtr = (writePtr-elements == (bufferSlots-1))?elements:(writePtr+1);
  if (tempPtr->readstatus == unread && writePtr == readPtr) {
    readPtr = (readPtr-elements == (bufferSlots-1))?elements:(readPtr+1);
  }
  writePtr->value = value;
  writePtr->readstatus = unread;
  writePtr = tempPtr;
}

int CircularBuffer::read() {
  int temp;
  if (readPtr->readstatus == unread)
    temp = readPtr->value;
  else 
    temp = -1;
  readPtr->readstatus = isread;
  readPtr = ((readPtr-elements == (bufferSlots-1))?elements:(readPtr+1));
  return temp;
}

void CircularBuffer::clear() {
  for (int i=0; i<bufferSlots; i++) {
    elements[i].readstatus = isread;
  }
  readPtr = writePtr;
}

}  // namespace base
