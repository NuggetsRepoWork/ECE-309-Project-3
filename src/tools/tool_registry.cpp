// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.
// Every function below is a stub so the starter compiles. Replace each body.
#include "tools/tool_registry.h"

#include <cstdint>
#include <utility>

namespace {
    constexpr std::size_t kInitialBucketCount = 16;
}

ToolRegistry::ToolRegistry() : buckets_(kInitialBucketCount) {}

ToolRegistry::~ToolRegistry() = default;

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    // 64-bit FNV-1a. Hash the full string; bucket reduction happens separately.
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char c : key) {
        value ^= static_cast<std::uint64_t>(c);
        value *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(value);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) {
        return false;
    }

    std::string key = tool->name();
    std::size_t index = hash(key) % buckets_.size();

    // Reject duplicates before resizing; the original registration wins.
    for (const Entry& entry : buckets_[index]) {
        if (entry.name == key) {
            return false;
        }
    }

    // Resize before insertion so the public load factor never crosses 0.75.
    const double next_load = static_cast<double>(size_ + 1) /
        static_cast<double>(buckets_.size());
    if (next_load > kMaxLoadFactor) {
        rehash(buckets_.size() * 2);
        index = hash(key) % buckets_.size();
    }

    buckets_[index].push_back(Entry{ std::move(key), std::move(tool) });
    ++size_;
    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    if (buckets_.empty()) {
        return nullptr;
    }

    const std::size_t index = hash(name) % buckets_.size();
    for (const Entry& entry : buckets_[index]) {
        if (entry.name == name) {
            return entry.tool.get();
        }
    }
    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept {
    return size_;
}

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const auto& bucket : buckets_) {
        for (const Entry& entry : bucket) {
            fn(*entry.tool);
        }
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept {
    return buckets_.size();
}

double ToolRegistry::load_factor() const noexcept {
    if (buckets_.empty()) {
        return 0.0;
    }
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const auto& bucket : buckets_) {
        if (bucket.size() > longest) {
            longest = bucket.size();
        }
    }
    return longest;
}

void ToolRegistry::rehash(std::size_t new_bucket_count) {
    std::vector<std::vector<Entry>> new_buckets(new_bucket_count);

    for (auto& bucket : buckets_) {
        for (auto& entry : bucket) {
            const std::size_t index = hash(entry.name) % new_bucket_count;
            new_buckets[index].push_back(std::move(entry));
        }
    }

    buckets_.swap(new_buckets);
}
