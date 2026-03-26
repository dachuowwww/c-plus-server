#include "Metrics.h"
#include <chrono>
#include "Logger.h"

namespace {
std::atomic<uint64_t> g_accepts{0};
std::atomic<uint64_t> g_closes{0};
std::atomic<uint64_t> g_requests{0};
std::atomic<uint64_t> g_parse_errors{0};
std::atomic<uint64_t> g_read_bytes{0};
std::atomic<uint64_t> g_write_bytes{0};
std::atomic<uint64_t> g_read_eagain{0};
std::atomic<uint64_t> g_write_eagain{0};
std::atomic<uint64_t> g_read_errors{0};
std::atomic<uint64_t> g_write_errors{0};
std::atomic<uint64_t> g_sendfile_bytes{0};
std::atomic<uint64_t> g_sendfile_eagain{0};
std::atomic<uint64_t> g_sendfile_errors{0};
std::atomic<int64_t> g_active{0};
}  // namespace

void Metrics::OnAccept() {
  g_accepts.fetch_add(1, std::memory_order_relaxed);
  g_active.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::OnClose() {
  g_closes.fetch_add(1, std::memory_order_relaxed);
  g_active.fetch_sub(1, std::memory_order_relaxed);
}

void Metrics::OnRequest() { g_requests.fetch_add(1, std::memory_order_relaxed); }

void Metrics::OnParseError() { g_parse_errors.fetch_add(1, std::memory_order_relaxed); }

void Metrics::AddReadBytes(uint64_t n) { g_read_bytes.fetch_add(n, std::memory_order_relaxed); }

void Metrics::AddWriteBytes(uint64_t n) { g_write_bytes.fetch_add(n, std::memory_order_relaxed); }

void Metrics::OnReadEagain() { g_read_eagain.fetch_add(1, std::memory_order_relaxed); }

void Metrics::OnWriteEagain() { g_write_eagain.fetch_add(1, std::memory_order_relaxed); }

void Metrics::OnReadError() { g_read_errors.fetch_add(1, std::memory_order_relaxed); }

void Metrics::OnWriteError() { g_write_errors.fetch_add(1, std::memory_order_relaxed); }

void Metrics::AddSendfileBytes(uint64_t n) { g_sendfile_bytes.fetch_add(n, std::memory_order_relaxed); }

void Metrics::OnSendfileEagain() { g_sendfile_eagain.fetch_add(1, std::memory_order_relaxed); }

void Metrics::OnSendfileError() { g_sendfile_errors.fetch_add(1, std::memory_order_relaxed); }

MetricsSnapshot Metrics::Snapshot() {
  MetricsSnapshot snap;
  snap.accepts_ = g_accepts.load(std::memory_order_relaxed);
  snap.closes_ = g_closes.load(std::memory_order_relaxed);
  snap.requests_ = g_requests.load(std::memory_order_relaxed);
  snap.parse_errors_ = g_parse_errors.load(std::memory_order_relaxed);
  snap.read_bytes_ = g_read_bytes.load(std::memory_order_relaxed);
  snap.write_bytes_ = g_write_bytes.load(std::memory_order_relaxed);
  snap.read_eagain_ = g_read_eagain.load(std::memory_order_relaxed);
  snap.write_eagain_ = g_write_eagain.load(std::memory_order_relaxed);
  snap.read_errors_ = g_read_errors.load(std::memory_order_relaxed);
  snap.write_errors_ = g_write_errors.load(std::memory_order_relaxed);
  snap.sendfile_bytes_ = g_sendfile_bytes.load(std::memory_order_relaxed);
  snap.sendfile_eagain_ = g_sendfile_eagain.load(std::memory_order_relaxed);
  snap.sendfile_errors_ = g_sendfile_errors.load(std::memory_order_relaxed);
  snap.active_ = g_active.load(std::memory_order_relaxed);
  return snap;
}

void Metrics::LogSnapshot() {
  static MetricsSnapshot last = Snapshot();
  static auto last_time = std::chrono::steady_clock::now();

  auto now = std::chrono::steady_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - last_time).count();
  if (elapsed <= 0.0) {
    elapsed = 1.0;
  }

  MetricsSnapshot cur = Snapshot();

  double accepts_rate = static_cast<double>(cur.accepts_ - last.accepts_) / elapsed;
  double closes_rate = static_cast<double>(cur.closes_ - last.closes_) / elapsed;
  double requests_rate = static_cast<double>(cur.requests_ - last.requests_) / elapsed;
  double parse_errors_rate = static_cast<double>(cur.parse_errors_ - last.parse_errors_) / elapsed;
  double read_kbps = static_cast<double>(cur.read_bytes_ - last.read_bytes_) * 8.0 / elapsed / 1000.0;
  double write_kbps = static_cast<double>(cur.write_bytes_ - last.write_bytes_) * 8.0 / elapsed / 1000.0;
  double read_eagain_rate = static_cast<double>(cur.read_eagain_ - last.read_eagain_) / elapsed;
  double write_eagain_rate = static_cast<double>(cur.write_eagain_ - last.write_eagain_) / elapsed;
  double read_error_rate = static_cast<double>(cur.read_errors_ - last.read_errors_) / elapsed;
  double write_error_rate = static_cast<double>(cur.write_errors_ - last.write_errors_) / elapsed;
  double sendfile_kbps = static_cast<double>(cur.sendfile_bytes_ - last.sendfile_bytes_) * 8.0 / elapsed / 1000.0;
  double sendfile_eagain_rate = static_cast<double>(cur.sendfile_eagain_ - last.sendfile_eagain_) / elapsed;
  double sendfile_error_rate = static_cast<double>(cur.sendfile_errors_ - last.sendfile_errors_) / elapsed;

  LOG_INFO << "metrics active=" << cur.active_ << " accept/s=" << accepts_rate << " close/s=" << closes_rate
           << " req/s=" << requests_rate << " parse_err/s=" << parse_errors_rate << " in_kbps=" << read_kbps
           << " out_kbps=" << write_kbps << " rd_eagain/s=" << read_eagain_rate << " wr_eagain/s=" << write_eagain_rate
           << " rd_err/s=" << read_error_rate << " wr_err/s=" << write_error_rate << " sf_kbps=" << sendfile_kbps
           << " sf_eagain/s=" << sendfile_eagain_rate << " sf_err/s=" << sendfile_error_rate;

  last = cur;
  last_time = now;
}
