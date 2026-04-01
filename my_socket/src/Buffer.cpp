#include "Buffer.h"
#include <cstring>
#include <iostream>
#include "Error.h"

Buffer::Buffer() : read_index_(KPREPENDINDEX), write_index_(KPREPENDINDEX) { buffer_.resize(KINITALSIZE); }

void Buffer::Append(const char *data) { Append(data, static_cast<int>(strlen(data))); }
void Buffer::Append(const std::string &data) { Append(data.data(), data.size()); }
void Buffer::Append(const char *data, ssize_t len) {
  EnsureWritableBytes(len);
  std::copy(data, data + len, BeginWrite());
  write_index_ += len;
}

int Buffer::ReadableBytes() const { return write_index_ - read_index_; }
int Buffer::WritableBytes() const { return static_cast<int>(buffer_.size()) - write_index_; }
int Buffer::PrepenableBytes() const { return read_index_; }
void Buffer::EnsureWritableBytes(int len) {
  if (WritableBytes() < len) {
    if (WritableBytes() + PrepenableBytes() < len + KPREPENDINDEX) {
      buffer_.resize(write_index_ + len);
    } else {
      std::copy(BeginRead(), BeginWrite(), Begin() + KPREPENDINDEX);
      write_index_ = read_index_ + ReadableBytes();
      read_index_ = KPREPENDINDEX;
    }
  }
}

char *Buffer::Begin() { return &*buffer_.begin(); }
const char *Buffer::Begin() const { return &*buffer_.begin(); }

char *Buffer::BeginRead() { return Begin() + read_index_; }
const char *Buffer::BeginRead() const { return Begin() + read_index_; }

char *Buffer::BeginWrite() { return Begin() + write_index_; }
const char *Buffer::BeginWrite() const { return Begin() + write_index_; }
char *Buffer::Peek() { return BeginRead(); }
const char *Buffer::Peek() const { return BeginRead(); }
std::string Buffer::PeekAsString(int len) const { return std::string(BeginRead(), BeginRead() + len); }
std::string Buffer::PeekAllString() const { return std::string(BeginRead(), BeginWrite()); }

void Buffer::Retreive(int len) {
  Errif(len > ReadableBytes(), "Retreive len is larger than ReadableBytes");
  if (len < ReadableBytes()) {
    read_index_ += len;
  } else {
    RetreiveAll();
  }
}
void Buffer::RetreiveAll() {
  write_index_ = KPREPENDINDEX;
  read_index_ = KPREPENDINDEX;
}
std::string Buffer::RetreiveAsString(int len) {
  Errif(len > ReadableBytes(), "RetreiveAsString len is larger than ReadableBytes");
  std::string result = PeekAsString(len);
  Retreive(len);
  return result;
}

std::string Buffer::RetreiveAllAsString() {
  std::string result = PeekAllString();
  RetreiveAll();
  return result;
}

void Buffer::RetrieveUtil(const char *end) {
  Errif(BeginWrite() < end, "RetrieveUtil len is larger than ReadableBytes");
  read_index_ += static_cast<int>(end - BeginRead());
}
std::string Buffer::RetrieveUtilAsString(const char *end) {
  Errif(BeginWrite() < end, "RetrieveUtilAsString len is larger than ReadableBytes");
  std::string ret = PeekAsString(static_cast<int>(end - BeginRead()));
  RetrieveUtil(end);
  return ret;
}

void Buffer::AppendKeyBoard() {
  RetreiveAll();
  std::cout << "Please input message to send to server :" << std::endl;
  std::string message;
  std::getline(std::cin, message);
  Append(message);
  // std::cout << "input message: " << buffer_ << std::endl;
}
