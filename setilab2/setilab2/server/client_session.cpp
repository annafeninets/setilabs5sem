#include "client_session.hpp"

#include "../common/connection_error.hpp"
#include "../common/display_units.hpp"
#include "../common/logger.hpp"
#include "../common/platform.hpp"
#include "../common/protocol_limits.hpp"
#include "../common/utf8.hpp"
#include "speed_tracker.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <vector>

namespace {
constexpr size_t kChunkSize = 1 << 16; // 64 KiB
}

static std::string shortenFileName(const std::string& name, size_t maxBytes) {
    constexpr size_t kMaxKeptExtensionBytes = 32;
    std::string ext = std::filesystem::path(name).extension().string();
    if (ext.size() > kMaxKeptExtensionBytes || ext.size() >= maxBytes) {
        ext.clear();
    }
    std::string stem = name.substr(0, name.size() - ext.size());
    return truncateUtf8(stem, maxBytes - ext.size()) + ext;
}

ClientSession::ClientSession(TcpConnection connection, std::filesystem::path uploadsDir)
    : connection_(std::move(connection)), uploadsDir_(std::move(uploadsDir)) {}

std::filesystem::path ClientSession::resolveOutputPath(const std::string& clientFileName) const {
    std::string base = std::filesystem::path(clientFileName).filename().string();
    if (base.empty() || base == "." || base == "..") {
        base = "upload_" +
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    const size_t maxNameBytes = maxFileNameBytesInDir(uploadsDir_.string());
    if (base.size() > maxNameBytes) {
        Logger::log("[", clientLabel_, "] file name is ", base.size(),
                    " bytes, longer than the file system limit (", maxNameBytes,
                    "); shortening it");
        base = shortenFileName(base, maxNameBytes);
    }

    std::filesystem::path candidate = uploadsDir_ / base;

    auto canonicalUploads = std::filesystem::weakly_canonical(uploadsDir_);
    auto canonicalParent = std::filesystem::weakly_canonical(candidate.parent_path());
    if (canonicalParent != canonicalUploads) {
        throw std::runtime_error("refusing to write outside uploads directory");
    }
    return candidate;
}

void ClientSession::receiveAndStoreFile() {
    std::filesystem::create_directories(uploadsDir_);

    std::string clientFileName = connection_.recvString(kMaxFileNameBytes);

    if (!isValidUtf8(clientFileName)) {
        throw ConnectionError("client sent a file name that is not valid UTF-8");
    }
    if (clientFileName.find('\0') != std::string::npos) {
        throw ConnectionError("client sent a file name containing a NUL byte");
    }

    totalBytes_ = connection_.recvUint64();
    if (totalBytes_ > kMaxFileSizeBytes) {
        throw ConnectionError("client declared a file size above the 1 TiB protocol limit");
    }

    std::filesystem::path outputPath = resolveOutputPath(clientFileName);
    std::ofstream outFile(outputPath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        throw std::runtime_error("cannot create output file: " + outputPath.string());
    }

    double totalMB = static_cast<double>(totalBytes_) / kBytesPerMegabyte;
    Logger::log("[", clientLabel_, "] receiving '", clientFileName, "' (", std::fixed,
                std::setprecision(2), totalMB, " MB) -> ", outputPath.string());

    SpeedTracker tracker(clientLabel_, receivedBytes_, totalBytes_);
    tracker.start();

    std::vector<char> buffer(kChunkSize);
    uint64_t remaining = totalBytes_;
    while (remaining > 0) {
        size_t toRead = static_cast<size_t>(std::min<uint64_t>(kChunkSize, remaining));
        size_t n = connection_.recvSome(buffer.data(), toRead);
        if (n == 0) {
            tracker.stop();
            throw ConnectionError("client disconnected before sending the full file");
        }
        outFile.write(buffer.data(), static_cast<std::streamsize>(n));
        receivedBytes_.fetch_add(n, std::memory_order_relaxed);
        remaining -= n;
    }
    outFile.close();
    tracker.stop();
    bool ok = std::filesystem::exists(outputPath) &&
              std::filesystem::file_size(outputPath) == totalBytes_;
    connection_.sendString(ok ? kStatusSuccess : kStatusFail);
    Logger::log("[", clientLabel_, "] ", (ok ? "SUCCESS" : "FAIL"));
}

void ClientSession::run() {
    clientLabel_ = connection_.describeRemote();
    Logger::log("[", clientLabel_, "] connected");

    try {
        connection_.setTimeout(kIoTimeoutSeconds);
    } catch (const std::exception& e) {
        Logger::log("[", clientLabel_, "] error: ", e.what());
        return;
    }

    try {
        receiveAndStoreFile();
    } catch (const std::exception& e) {
        Logger::log("[", clientLabel_, "] error: ", e.what());
    }

    Logger::log("[", clientLabel_, "] disconnected");
}
