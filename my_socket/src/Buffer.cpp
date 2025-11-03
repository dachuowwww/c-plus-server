#include "Buffer.h"
#include <iostream>

Buffer::Buffer() : buffer_("") {}

Buffer::~Buffer() {}

void Buffer::Append(const char *data, ssize_t len) { buffer_.append(data, len); }

ssize_t Buffer::GetSize() { return buffer_.size(); }

const char *Buffer::ReadAll() { return buffer_.c_str(); }

void Buffer::Clear() { buffer_.clear(); }

void Buffer::GetLine() { std::getline(std::cin, buffer_); }
