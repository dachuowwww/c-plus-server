#include "Buffer.h"

void Buffer::Append(const char *data, ssize_t len) { buffer_.append(data, len); }

ssize_t Buffer::GetSize() const { return buffer_.size(); }

const char *Buffer::GetData() const { return buffer_.c_str(); }

void Buffer::Input(const char *data) { buffer_ = data; }

void Buffer::Clear() { buffer_.clear(); }
