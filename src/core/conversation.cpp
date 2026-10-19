#include "core/conversation.h"
#include <utility>

Conversation::Conversation()
    : data_(nullptr), size_(0), capacity_(0) {
}

Conversation::~Conversation() {
    delete[] data_;
}

Conversation::Conversation(const Conversation& other)
    : size_(other.size_), capacity_(other.size_) {
    if (capacity_ > 0) {
        data_ = new Message[capacity_];
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }
    else {
        data_ = nullptr;
    }
}

Conversation& Conversation::operator=(const Conversation& other) {
    if (this != &other) {
        Conversation temp(other);
        std::swap(data_, temp.data_);
        std::swap(size_, temp.size_);
        std::swap(capacity_, temp.capacity_);
    }
    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

Conversation& Conversation::operator=(Conversation&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

void Conversation::append(Message m) {
    if (size_ == capacity_) {
        std::size_t new_capacity = (capacity_ == 0) ? 2 : capacity_ * 2;
        Message* new_data = new Message[new_capacity];
        for (std::size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }

    if (m.role() == Role::System) {
        // Shift existing elements right to place System message at index 0
        for (std::size_t i = size_; i > 0; --i) {
            data_[i] = std::move(data_[i - 1]);
        }
        data_[0] = std::move(m);
    }
    else {
        // Normal append to the back
        data_[size_] = std::move(m);
    }
    ++size_;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Conversation::at: index out of range");
    }
    return data_[i];
}

const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    return data_ + size_;
}