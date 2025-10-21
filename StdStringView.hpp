/**
 * @file StdOptional.hpp
 * @brief
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date
 *
 *
 */

#ifndef MMR_UTIL_STD_STRING_VIEW_HPP
#define MMR_UTIL_STD_STRING_VIEW_HPP

#if __has_include(<string_view>)
#include <string_view>
#else

#include <string>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <ostream>
#include <cstring>
namespace std {
	class string_view {
	public:
		// Types
		typedef char value_type;
		typedef const char* pointer;
		typedef const char* const_pointer;
		typedef const char& reference;
		typedef const char& const_reference;
		typedef const char* const_iterator;
		typedef const_iterator iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef const_reverse_iterator reverse_iterator;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		static const size_type npos = static_cast<size_type>(-1);

		// Construction
		string_view() noexcept : data_(nullptr), size_(0) {}
		string_view(const char* str) : data_(str), size_(str ? strlen(str) : 0) {}
		string_view(const std::string& str) noexcept : data_(str.data()), size_(str.size()) {}
		string_view(const char* str, size_type len) : data_(str), size_(len) {}

		// Assignment
		string_view& operator=(const string_view&) noexcept = default;

		// Iterators
		const_iterator begin() const noexcept { return data_; }
		const_iterator end() const noexcept { return data_ + size_; }
		const_iterator cbegin() const noexcept { return begin(); }
		const_iterator cend() const noexcept { return end(); }
		reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
		reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
		const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		const_reverse_iterator crend() const noexcept { return rend(); }

		// Capacity
		size_type size() const noexcept { return size_; }
		size_type length() const noexcept { return size_; }
		size_type max_size() const noexcept { return size_; }
		bool empty() const noexcept { return size_ == 0; }

		// Element access
		const_reference operator[](size_type pos) const { return data_[pos]; }
		const_reference at(size_type pos) const {
			if (pos >= size_) throw std::out_of_range("string_view::at");
			return data_[pos];
		}
		const_reference front() const { return data_[0]; }
		const_reference back() const { return data_[size_ - 1]; }
		const_pointer data() const noexcept { return data_; }

		// Modifiers
		void remove_prefix(size_type n) {
			data_ += n;
			size_ -= n;
		}
		void remove_suffix(size_type n) {
			size_ -= n;
		}
		void swap(string_view& other) noexcept {
			std::swap(data_, other.data_);
			std::swap(size_, other.size_);
		}

		// Operations
		size_type copy(char* dest, size_type count, size_type pos = 0) const {
			if (pos > size_) throw std::out_of_range("string_view::copy");
			size_type rlen = std::min<size_type>(count, size_ - pos);
			std::copy(data_ + pos, data_ + pos + rlen, dest);
			return rlen;
		}

		string_view substr(size_type pos = 0, size_type count = npos) const {
			if (pos > size_) throw std::out_of_range("string_view::substr");
			size_type rlen = std::min<size_type>(count, size_ - pos);
			return string_view(data_ + pos, rlen);
		}

		int compare(string_view v) const noexcept {
			size_type rlen = std::min<size_type>(size_, v.size_);
			int result = std::char_traits<char>::compare(data_, v.data_, rlen);
			if (result == 0) {
				result = size_ == v.size_ ? 0 : (size_ < v.size_ ? -1 : 1);
			}
			return result;
		}

		int compare(size_type pos1, size_type count1, string_view v) const {
			return substr(pos1, count1).compare(v);
		}

		int compare(size_type pos1, size_type count1, string_view v,
			size_type pos2, size_type count2) const {
			return substr(pos1, count1).compare(v.substr(pos2, count2));
		}

		int compare(const char* s) const {
			return compare(string_view(s));
		}

		int compare(size_type pos1, size_type count1, const char* s) const {
			return substr(pos1, count1).compare(string_view(s));
		}

		int compare(size_type pos1, size_type count1, const char* s,
			size_type count2) const {
			return substr(pos1, count1).compare(string_view(s, count2));
		}

		//cpp 20支持
		//bool starts_with(string_view v) const noexcept {
		//	return size_ >= v.size_ && compare(0, v.size_, v) == 0;
		//}

		//bool starts_with(char c) const noexcept {
		//	return !empty() && front() == c;
		//}

		//bool starts_with(const char* s) const {
		//	return starts_with(string_view(s));
		//}

		//bool ends_with(string_view v) const noexcept {
		//	return size_ >= v.size_ && compare(size_ - v.size_, npos, v) == 0;
		//}

		//bool ends_with(char c) const noexcept {
		//	return !empty() && back() == c;
		//}

		//bool ends_with(const char* s) const {
		//	return ends_with(string_view(s));
		//}

		size_type find(string_view v, size_type pos = 0) const noexcept {
			if (pos > size_ || v.size_ > size_ - pos) return npos;
			if (v.empty()) return pos;
			const_iterator it = std::search(begin() + pos, end(), v.begin(), v.end());
			return it == end() ? npos : static_cast<size_type>(it - begin());
		}

		size_type find(char c, size_type pos = 0) const noexcept {
			if (pos >= size_) return npos;
			const_iterator it = std::find(begin() + pos, end(), c);
			return it == end() ? npos : static_cast<size_type>(it - begin());
		}

		size_type find(const char* s, size_type pos, size_type count) const {
			return find(string_view(s, count), pos);
		}

		size_type find(const char* s, size_type pos = 0) const {
			return find(string_view(s), pos);
		}

		// Other find functions (rfind, find_first_of, etc.) would go here
		// Omitted for brevity but should be implemented in a complete version

	private:
		const char* data_;
		size_type size_;
	};

	// Non-member functions
	inline bool operator==(string_view lhs, string_view rhs) noexcept {
		return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
	}

	inline bool operator!=(string_view lhs, string_view rhs) noexcept {
		return !(lhs == rhs);
	}

	inline bool operator<(string_view lhs, string_view rhs) noexcept {
		return lhs.compare(rhs) < 0;
	}

	inline bool operator<=(string_view lhs, string_view rhs) noexcept {
		return lhs.compare(rhs) <= 0;
	}

	inline bool operator>(string_view lhs, string_view rhs) noexcept {
		return lhs.compare(rhs) > 0;
	}

	inline bool operator>=(string_view lhs, string_view rhs) noexcept {
		return lhs.compare(rhs) >= 0;
	}

	inline std::ostream& operator<<(std::ostream& os, string_view v) {
		os.write(v.data(), v.size());
		return os;
	}
} // namespace std

#endif

#endif //MMR_UTIL_STD_STRING_VIEW_HPP