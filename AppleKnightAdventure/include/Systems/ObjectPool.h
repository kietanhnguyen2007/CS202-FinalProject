#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <vector>
#include <cstddef>
#include <memory>

template<typename T>
class ObjectPool {
protected:
    std::vector<std::unique_ptr<T>> m_pool;
    std::vector<size_t> m_available;

public:
    explicit ObjectPool(size_t initialSize = 64) {
        if (initialSize > 0) {
            for (size_t i = 0; i < initialSize; ++i) {
                m_pool.push_back(std::make_unique<T>());
                m_available.push_back(initialSize - 1 - i);
            }
        }
    }

    T* Acquire() {
        if (m_available.empty()) {
            m_pool.push_back(std::make_unique<T>());
            return m_pool.back().get();
        }
        size_t index = m_available.back();
        m_available.pop_back();
        T* obj = m_pool[index].get();
        *obj = T();
        return obj;
    }

    bool Release(T* obj) {
        for (size_t i = 0; i < m_pool.size(); ++i) {
            if (m_pool[i].get() == obj) {
                m_available.push_back(i);
                return true;
            }
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
            for (size_t i = old; i < size; ++i) {
                m_pool.push_back(std::make_unique<T>());
            }
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
