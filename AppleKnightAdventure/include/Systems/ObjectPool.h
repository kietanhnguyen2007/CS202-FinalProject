#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <vector>
#include <cstddef>

template<typename T>
class ObjectPool {
protected:
    std::vector<T> m_pool;
    std::vector<size_t> m_available;

public:
    explicit ObjectPool(size_t initialSize = 64) {
        if (initialSize > 0) {
            m_pool.resize(initialSize);
            for (size_t i = 0; i < initialSize; ++i) {
                m_available.push_back(initialSize - 1 - i);
            }
        }
    }

    T* Acquire() {
        if (m_available.empty()) {
            m_pool.emplace_back();
            return &m_pool.back();
        }
        size_t index = m_available.back();
        m_available.pop_back();
        T* obj = &m_pool[index];
        *obj = T();
        return obj;
    }

    bool Release(T* obj) {
        ptrdiff_t index = obj - m_pool.data();
        if (index >= 0 && index < static_cast<ptrdiff_t>(m_pool.size())) {
            m_available.push_back(static_cast<size_t>(index));
            return true;
        }
        return false;
    }

    void ReleaseAll() {
        m_available.clear();
        for (size_t i = 0; i < m_pool.size(); ++i) {
            m_available.push_back(m_pool.size() - 1 - i);
        }
    }

    void Reserve(size_t size) {
        size_t old = m_pool.size();
        if (size > old) {
            m_pool.resize(size);
            for (size_t i = size; i > old; --i) {
                m_available.push_back(i - 1);
            }
        }
    }

    size_t GetActiveCount() const {
        return m_pool.size() - m_available.size();
    }

    size_t GetPoolSize() const {
        return m_pool.size();
    }

    bool IsEmpty() const {
        return m_available.size() == m_pool.size();
    }
};

#endif
