#include "Buffer.h"
#include <iostream>
void Buffer::Append(const char *data, ssize_t len) {
  buffer_.append(data, len);
  //std::cout << "append data to buffer: " << data << std::endl;
}

ssize_t Buffer::GetSize() const { return buffer_.size(); }

const char *Buffer::GetData() const { return buffer_.c_str(); }

void Buffer::Clear() { buffer_.clear(); }

void Buffer::SetData(const char *data) {
  buffer_ = data;
  //std::cout << "set data to buffer: " << data << std::endl;
}

void Buffer::SetKeyBoardInput() {
  std::cout << "Please input message to send to server :" << std::endl;
  std::getline(std::cin, buffer_);
  // std::cout << "input message: " << buffer_ << std::endl;
}
