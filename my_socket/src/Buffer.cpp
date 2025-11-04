#include "Buffer.h"
#include <iostream>

void Buffer::Append(const char *data, ssize_t len) { buffer_.append(data, len); }

ssize_t Buffer::GetSize() { return buffer_.size(); }

const char *Buffer::ReadAll() { return buffer_.c_str(); }

void Buffer::Clear() { buffer_.clear(); }

void Buffer::GetKeyboardLine() { std::getline(std::cin, buffer_); }

void Buffer::GetData(const char *data) { buffer_ = data; }
