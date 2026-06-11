#ifndef OBSERVABLELIST_H
#define OBSERVABLELIST_H

#include <vector>
#include <functional>

template<typename T>
class ObservableList {
protected:
    std::vector<T> m_items;

public:
    using OnItemAdded = std::function<void(const T&)>;
    using OnItemRemoved = std::function<void(const T&)>;
    using OnCleared = std::function<void()>;

    OnItemAdded OnItemAddedCallback;
    OnItemRemoved OnItemRemovedCallback;
    OnCleared OnClearedCallback;

    void Add(const T& item) {
        m_items.push_back(item);
        if (OnItemAddedCallback) {
            OnItemAddedCallback(item);
        }
    }

    void RemoveAt(size_t index) {
        if (index < m_items.size()) {
            T item = m_items[index];
            m_items.erase(m_items.begin() + static_cast<ptrdiff_t>(index));
            if (OnItemRemovedCallback) {
                OnItemRemovedCallback(item);
            }
        }
    }

    bool Remove(const T& item) {
        for (size_t i = 0; i < m_items.size(); ++i) {
            if (m_items[i] == item) {
                RemoveAt(i);
                return true;
            }
        }
        return false;
    }

    void Clear() {
        m_items.clear();
        if (OnClearedCallback) {
            OnClearedCallback();
        }
    }

    T& operator[](size_t index) {
        return m_items[index];
    }

    const T& operator[](size_t index) const {
        return m_items[index];
    }

    T& At(size_t index) {
        return m_items.at(index);
    }

    const T& At(size_t index) const {
        return m_items.at(index);
    }

    size_t Size() const {
        return m_items.size();
    }

    bool IsEmpty() const {
        return m_items.empty();
    }

    bool Contains(const T& item) const {
        for (const auto& i : m_items) {
            if (i == item) return true;
        }
        return false;
    }

    auto begin() { return m_items.begin(); }
    auto end() { return m_items.end(); }
    auto begin() const { return m_items.begin(); }
    auto end() const { return m_items.end(); }
};

#endif
