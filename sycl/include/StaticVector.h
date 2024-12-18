#include <array>
#include <algorithm>
#include <initializer_list>

template <typename T, size_t Capacity>
class StaticVector {
private:
    std::array<T, Capacity> data;
    size_t current_size = 0;

public:
    // Constructors
    StaticVector() = default;
    StaticVector(size_t count) : current_size(std::min(count, Capacity)) {}
    StaticVector(size_t count, const T& value) : current_size(std::min(count, Capacity)) {
        std::fill(data.begin(), data.begin() + current_size, value);
    }
    template<typename InputIt>
    StaticVector(InputIt first, InputIt last) {
        while (first != last && current_size < Capacity) {
            data[current_size++] = *first++;
        }
    }
    StaticVector(const std::initializer_list<T>& init) : current_size(std::min(init.size(), Capacity)) {
        std::copy(init.begin(), init.begin() + current_size, data.begin());
    }
    StaticVector(const StaticVector& other) = default;
    StaticVector(StaticVector&& other) noexcept = default;

    // Assignment operators
    StaticVector& operator=(const StaticVector& other) = default;
    StaticVector& operator=(StaticVector&& other) noexcept = default;
    StaticVector& operator=(const std::initializer_list<T>& init) {
        current_size = std::min(init.size(), Capacity);
        std::copy(init.begin(), init.begin() + current_size, data.begin());
        return *this;
    }

    // Capacity
    size_t size() const { return current_size; }
    size_t max_size() const { return Capacity; }
    bool empty() const { return current_size == 0; }
    size_t capacity() const { return Capacity; }

    // Element access
    T& operator[](size_t pos) { return data[pos]; }
    const T& operator[](size_t pos) const { return data[pos]; }
    T& at(size_t pos) {
        if (pos >= current_size) {
          // No exceptions, return a reference to a static dummy value.
          static T dummy{};
          return dummy;
        }
        return data[pos];
    }
    const T& at(size_t pos) const {
         if (pos >= current_size) {
          static T dummy{};
          return dummy;
        }
        return data[pos];
    }
    T& front() { return data[0]; }
    const T& front() const { return data[0]; }
    T& back() { return data[current_size - 1]; }
    const T& back() const { return data[current_size - 1]; }
    T* data_ptr() { return data.data(); }
    const T* data_ptr() const { return data.data(); }

    // Iterators
    using iterator = T*;
    using const_iterator = const T*;
    iterator begin() { return data.begin(); }
    const_iterator begin() const { return data.begin(); }
    iterator end() { return data.begin() + current_size; }
    const_iterator end() const { return data.begin() + current_size; }
    const_iterator cbegin() const { return data.cbegin(); }
    const_iterator cend() const { return data.cbegin() + current_size; }

    // Modifiers
    void clear() { current_size = 0; }
    void push_back(const T& value) {
        if (current_size < Capacity) {
            data[current_size++] = value;
        }
    }
    void pop_back() {
        if (current_size > 0) {
            --current_size;
        }
    }
    template <class... Args>
    void emplace_back(Args&&... args) {
      if(current_size < Capacity) {
        data[current_size++] = T(std::forward<Args>(args)...);
      }
    }
    void resize(size_t n) {
        current_size = std::min(n, Capacity);
    }
    void resize(size_t n, const T& val) {
        if(n > current_size){
            size_t to_add = std::min(n-current_size, Capacity-current_size);
            std::fill(data.begin() + current_size, data.begin() + current_size + to_add, val);
            current_size += to_add;
        } else {
            current_size = n;
        }
    }

    iterator insert(const_iterator pos, const T& value) {
        size_t index = pos - begin();
        if (current_size < Capacity && index <= current_size) {
            for (size_t i = current_size; i > index; --i) {
                data[i] = data[i - 1];
            }
            data[index] = value;
            ++current_size;
            return begin() + index;
        }
        return end();
    }

    iterator insert(const_iterator pos, size_t count, const T& value) {
      size_t index = pos - begin();
      if (index <= current_size) {
        size_t num_to_insert = std::min(count, Capacity - current_size);
        if (num_to_insert > 0) {
          size_t num_to_move = current_size - index;
          
          // Move existing elements to make space
          for (size_t i = 0; i < num_to_move; ++i) {
            if (current_size + num_to_insert - 1 - i >= Capacity) break; // Prevent out-of-bounds write
            data[current_size + num_to_insert - 1 - i] = data[current_size - 1 - i];
          }
          
          // Fill the inserted range with the value
          std::fill(data.begin() + index, data.begin() + index + num_to_insert, value);
          
          current_size += num_to_insert;
          return begin() + index;
        }
      }
      return end();
    }


    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_t index = pos - begin();
        if (index <= current_size) {
            size_t num_to_insert = 0;
            InputIt temp_first = first;
            while(temp_first != last && current_size + num_to_insert < Capacity){
                ++num_to_insert;
                ++temp_first;
            }

            if (num_to_insert > 0) {
              size_t num_to_move = current_size - index;
              
              // Move existing elements
              for (size_t i = 0; i < num_to_move; ++i) {
                if(current_size + num_to_insert - 1 - i >= Capacity) break; // Prevent out-of-bounds write
                data[current_size + num_to_insert - 1 - i] = data[current_size - 1 - i];
              }

              // Copy elements from the input range
              std::copy(first, first + num_to_insert, data.begin() + index);
              
              current_size += num_to_insert;
              return begin() + index;
            }
        }
        return end();
    }

    iterator insert(const_iterator pos, const std::initializer_list<T>& ilist){
        return insert(pos, ilist.begin(), ilist.end());
    }
};