#pragma once

#include <cstddef>      
#include <new>          
#include <stdexcept>    
#include <type_traits>  
#include <utility>      
#include <iterator>     
#include <memory>       

template <typename T, std::size_t max_size>
class tight_array {
public:

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    constexpr tight_array() noexcept = default;

    ~tight_array() {
        clear();
    }

    tight_array(const tight_array& other) {

        for (size_type i = 0; i < other.m_size; ++i) {
            new (data() + i) T(other.data()[i]);
        }
        m_size = other.m_size;
    }

    tight_array& operator=(const tight_array& other) {
        if (this != &other) {

            clear();

            for (size_type i = 0; i < other.m_size; ++i) {
                new (data() + i) T(other.data()[i]);
            }
            m_size = other.m_size;
        }
        return *this;
    }

    tight_array(tight_array&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        for (size_type i = 0; i < other.m_size; ++i) {
            new (data() + i) T(std::move(other.data()[i]));
        }
        m_size = other.m_size;
        other.clear();
    }

    tight_array& operator=(tight_array&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            clear();

            for (size_type i = 0; i < other.m_size; ++i) {
                new (data() + i) T(std::move(other.data()[i]));
            }
            m_size = other.m_size;
            other.clear();
        }
        return *this;
    }

    iterator       begin() noexcept { return data(); }
    const_iterator begin() const noexcept { return data(); }
    const_iterator cbegin() const noexcept { return data(); }

    iterator       end() noexcept { return data() + m_size; }
    const_iterator end() const noexcept { return data() + m_size; }
    const_iterator cend() const noexcept { return data() + m_size; }

    reverse_iterator       rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator       rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
    constexpr size_type size() const noexcept { return m_size; }
    constexpr size_type capacity() const noexcept { return max_size; }

    reference at(size_type pos) {
        if (pos >= m_size) throw std::out_of_range("at(): index out of range");
        return data()[pos];
    }
    const_reference at(size_type pos) const {
        if (pos >= m_size) throw std::out_of_range("at(): index out of range");
        return data()[pos];
    }

    reference       operator[](size_type pos) noexcept { return data()[pos]; }
    const_reference operator[](size_type pos) const noexcept { return data()[pos]; }

    reference       front() { return data()[0]; }
    const_reference front() const { return data()[0]; }

    reference       back() { return data()[m_size - 1]; }
    const_reference back() const { return data()[m_size - 1]; }

    reference       newest() { return back(); }
    const_reference newest() const { return back(); }
    reference       oldest() { return front(); }
    const_reference oldest() const { return front(); }

    pointer       data() noexcept { return std::launder(reinterpret_cast<pointer>(m_stack_block)); }
    const_pointer data() const noexcept { return std::launder(reinterpret_cast<const_pointer>(m_stack_block)); }


    template<typename... Args>
    reference emplace_back(Args&&... args) {
        if (m_size >= max_size) throw std::overflow_error("emplace_back(): container is full");
        new (data() + m_size) T(std::forward<Args>(args)...);
        return data()[m_size++];
    }

    void push_back(const T& value) {
        if (m_size >= max_size) throw std::overflow_error("push_back(): container is full");
        new (data() + m_size) T(value);
        ++m_size;
    }

    void push_back(T&& value) {
        if (m_size >= max_size) throw std::overflow_error("push_back(): container is full");
        new (data() + m_size) T(std::move(value));
        ++m_size;
    }

    void pop_back() {
        if (empty()) return;
        --m_size;
        (data() + m_size)->~T();
    }

    iterator erase(const_iterator pos) {
        if (pos < cbegin() || pos >= cend()) {
            throw std::out_of_range("erase(): iterator out of range");
        }

        const auto index = std::distance(cbegin(), pos);
        iterator current = begin() + index;

        for (iterator it = current; it != end() - 1; ++it) {
            *it = std::move(*(it + 1));
        }

        pop_back();
        return current;
    }

    bool erase_unordered(size_type index) {
        if (index >= m_size) return false;
        if (index < m_size - 1) {
            at(index) = std::move(back());
        }
        pop_back();
        return true;
    }

    void pop_front() {
        if (empty()) return;
        erase(cbegin());
    }

    void clear() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_type i = 0; i < m_size; ++i) {
                (data() + i)->~T();
            }
        }
        m_size = 0;
    }

    void resize(size_type new_size) {

        if (new_size > max_size) {
            throw std::overflow_error("resize(): new_size exceeds container capacity");
        }

        if (new_size > m_size) {
            for (size_type i = m_size; i < new_size; ++i) {
                new (data() + i) T();
            }
        }
        else if (new_size < m_size) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_type i = new_size; i < m_size; ++i) {
                    (data() + i)->~T();
                }
            }
        }

        m_size = new_size;
    }

private:
    alignas(T) std::byte m_stack_block[sizeof(T) * max_size]{};
    size_type m_size = 0;
};