#include <iostream>
#include "list_set.hpp"


namespace base {

Mutex m;

ListBasedSet::ListBasedSet() : Head(NULL) {
}

ListBasedSet::~ListBasedSet() {
  struct Node* currentNode = Head;
  struct Node* tempNode;
  while (currentNode != NULL) {
    tempNode = currentNode;
    currentNode = currentNode->next;
    delete tempNode;
  }
}

bool ListBasedSet::insert(int value) {
  ScopedLock lock(&m);
  struct Node* newNode;
  struct Node* currentNode = Head;
  struct Node* prevNode;
  newNode = new struct Node();
  newNode->value = value;
  newNode->next = NULL;
  if (currentNode == NULL) {
    Head = newNode;
    return true;
  }
  if (currentNode->value > newNode->value) {
    Head = newNode;
    Head->next = currentNode;
    return true;
  }
  while(currentNode != NULL && currentNode->value < newNode->value) {
    prevNode = currentNode;
    currentNode = currentNode->next;
  }
  if ( currentNode == NULL) {
    prevNode->next = newNode;
    return true;
  }
  if ( currentNode->value == newNode->value) {
    delete newNode;
    return false;
  }
  
  prevNode->next = newNode;
  newNode->next = currentNode;
  return true;
  
}

bool ListBasedSet::remove(int value) {
  ScopedLock lock(&m);
  struct Node* currentNode = Head;
  struct Node* prevNode;
  if (currentNode == NULL) {
    return false;
  }
  while (currentNode != NULL && currentNode->value != value ) {
    prevNode = currentNode;
    currentNode = currentNode->next;
  }
  if (currentNode == Head) {
    Head = Head->next;
    delete currentNode;
    return true;
  }
  if (currentNode == NULL) {
    return false;
  }  
  prevNode->next = currentNode->next;
  delete currentNode;
  return true;
  
}


bool ListBasedSet::lookup(int value) const {
  ScopedLock lock(&m);
  struct Node* currentNode = Head;
  if (currentNode == NULL) {
    return false;
  }
  while (currentNode != NULL && currentNode->value < value) {
    currentNode = currentNode->next;
  }
  if (currentNode == NULL) {
    return false;
  }
  if (currentNode->value > value ) {
    return false;
  }
  return true;
  
}

void ListBasedSet::clear() {
  struct Node* currentNode = Head;
  struct Node* tempNode;
  while (currentNode != NULL) {
    tempNode = currentNode;
    currentNode = currentNode->next;
    delete tempNode;
  }
  Head = NULL;
}

bool ListBasedSet::checkIntegrity() const {
  ScopedLock lock(&m);
  struct Node* currentNode = Head;
  struct Node* prevNode;
  if (currentNode == NULL ) {
    return true;
  }
  if (currentNode->next == NULL ) {
    return true;
  }
  
  prevNode = currentNode;
  currentNode = currentNode->next;
  while( currentNode != NULL && prevNode->value < currentNode->value) {
    prevNode = currentNode;
    currentNode = currentNode->next;
  }
  if (currentNode == NULL) {
    return true;
  }
  if (prevNode->value == currentNode->value) {
    std::cout << "Equal elements appear!" << std::endl;
  }
  return false;
}



} // namespace base
