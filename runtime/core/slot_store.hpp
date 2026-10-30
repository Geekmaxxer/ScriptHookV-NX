#pragma once
#include "runtime_config.hpp"
#include <stddef.h>
#include <new>

namespace ScriptHook {
// Append-only metadata pages preserve published owner tokens and suspended
// context addresses. Pages remain resident until process exit.
template<class T, size_t Initial, size_t PageSlots = 16> struct SlotStore {
    struct Page { Page* next; T values[PageSlots]; };
    T base[Initial];
    Page* head;
    Page* tail;
    size_t extra;
    mutable Page* cached;
    mutable size_t cached_start;

    size_t Capacity() const { return Initial + extra; }
    bool Ensure(size_t index) {
        if (index < Capacity()) return true;
        if (index != Capacity() || extra > SIZE_MAX - Initial - PageSlots) return false;
        void* storage = RuntimeConfig::AllocateMetadata(sizeof(Page));
        if (!storage) return false;
        auto* page = ::new(storage) Page{};
        if (tail) tail->next = page; else head = page;
        tail = page; extra += PageSlots;
        return true;
    }
    const T& operator[](size_t index) const {
        if (index < Initial) return base[index];
        if (!cached || index < cached_start) { cached = head; cached_start = Initial; }
        while (index - cached_start >= PageSlots) { cached = cached->next; cached_start += PageSlots; }
        return cached->values[index - cached_start];
    }
    T& operator[](size_t index) { return const_cast<T&>(static_cast<const SlotStore&>(*this)[index]); }
    template<class Store, class Value> struct Iterator {
        Store* store;
        size_t index;
        Value& operator*() const { return (*store)[index]; }
        Iterator& operator++() { ++index; return *this; }
        bool operator!=(const Iterator& other) const { return index != other.index; }
    };
    auto begin() { return Iterator<SlotStore, T>{this, 0}; }
    auto end() { return Iterator<SlotStore, T>{this, Capacity()}; }
    auto begin() const { return Iterator<const SlotStore, const T>{this, 0}; }
    auto end() const { return Iterator<const SlotStore, const T>{this, Capacity()}; }
};
}
