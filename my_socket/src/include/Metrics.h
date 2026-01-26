#pragma once
#include <atomic>
#include <cstdint>

struct MetricsSnapshot {
  uint64_t accepts_ = 0;
  uint64_t closes_ = 0;
  uint64_t requests_ = 0;
  uint64_t parse_errors_ = 0;
  uint64_t read_bytes_ = 0;
  uint64_t write_bytes_ = 0;
  uint64_t read_eagain_ = 0;
  uint64_t write_eagain_ = 0;
  uint64_t read_errors_ = 0;
  uint64_t write_errors_ = 0;
  uint64_t sendfile_bytes_ = 0;
  uint64_t sendfile_eagain_ = 0;
  uint64_t sendfile_errors_ = 0;
  int64_t active_ = 0;
};

class Metrics {
 public:
  static void OnAccept();
  static void OnClose();
  static void OnRequest();
  static void OnParseError();
  static void AddReadBytes(uint64_t n);
  static void AddWriteBytes(uint64_t n);
  static void OnReadEagain();
  static void OnWriteEagain();
  static void OnReadError();
  static void OnWriteError();
  static void AddSendfileBytes(uint64_t n);
  static void OnSendfileEagain();
  static void OnSendfileError();

  static MetricsSnapshot Snapshot();
  static void LogSnapshot();
};
