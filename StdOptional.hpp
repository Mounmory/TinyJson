/**
 * @file StdOptional.hpp
 * @brief 
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date 
 *
 * 
 */

#ifndef MMR_UTIL_STD_OPTIONAL_HPP
#define MMR_UTIL_STD_OPTIONAL_HPP
 // 版本检测宏
#if __has_include(<optional>)
// 如果支持 C++17 标准库 optional，直接使用标准库
#include <optional>
#else

#include <utility>
#include <stdexcept>
#include <type_traits>
namespace std {

// 自定义 nullopt_t 类型
	struct nullopt_t {
		explicit constexpr nullopt_t(int) {}
	};
	constexpr nullopt_t nullopt{ 0 };

	// 自定义 in_place_t 类型
	struct in_place_t {
		explicit in_place_t() = default;
	};
	constexpr in_place_t in_place{};

	// 自定义 bad_optional_access 异常
	class bad_optional_access : public std::exception {
	public:
		const char* what() const noexcept override {
			return "bad optional access";
		}
	};

	// 自定义 optional 实现
	template <typename T>
	class optional {
		static_assert(!std::is_reference<T>::value,
			"optional cannot be used with reference types");
		static_assert(!std::is_same<T, nullopt_t>::value,
			"optional cannot be used with nullopt_t");
		static_assert(!std::is_same<T, in_place_t>::value,
			"optional cannot be used with in_place_t");

		// 存储类型，使用对齐存储
		typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
		bool has_value_;

	public:
		// 构造/析构
		constexpr optional() noexcept : has_value_(false) {}
		constexpr optional(nullopt_t) noexcept : has_value_(false) {}

		// 从值构造
		template <typename U = T>
		optional(U&& value) : has_value_(true) {
			new (&storage) T(std::forward<U>(value));
		}

		// 从其他 optional 构造
		template <typename U>
		optional(const optional<U>& other) : has_value_(other.has_value_) {
			if (other.has_value_) {
				new (&storage) T(*other);
			}
		}

		template <typename U>
		optional(optional<U>&& other) : has_value_(other.has_value_) {
			if (other.has_value_) {
				new (&storage) T(std::move(*other));
			}
		}

		optional(const optional& other) : has_value_(other.has_value_) {
			if (other.has_value_) {
				new (&storage) T(*other);
			}
		}

		optional(optional&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
			: has_value_(other.has_value_) {
			if (other.has_value_) {
				new (&storage) T(std::move(*other));
			}
		}

		template <typename... Args>
		explicit optional(in_place_t, Args&&... args)
			: has_value_(true) {
			new (&storage) T(std::forward<Args>(args)...);
		}

		template <typename U, typename... Args,
			typename = typename std::enable_if<
			std::is_constructible<T, std::initializer_list<U>&, Args...>::value>::type>
			explicit optional(in_place_t, std::initializer_list<U> il, Args&&... args)
			: has_value_(true) {
			new (&storage) T(il, std::forward<Args>(args)...);
		}

		~optional() {
			if (has_value_) {
				reinterpret_cast<T*>(&storage)->~T();
			}
		}

		// 赋值操作
		optional& operator=(nullopt_t) noexcept {
			if (has_value_) {
				reinterpret_cast<T*>(&storage)->~T();
				has_value_ = false;
			}
			return *this;
		}

		optional& operator=(const optional& other) {
			if (this != &other) {
				if (other.has_value_) {
					if (has_value_) {
						**this = *other;
					}
					else {
						new (&storage) T(*other);
						has_value_ = true;
					}
				}
				else {
					if (has_value_) {
						reinterpret_cast<T*>(&storage)->~T();
						has_value_ = false;
					}
				}
			}
			return *this;
		}

		optional& operator=(optional&& other) noexcept(
			std::is_nothrow_move_assignable<T>::value&&
			std::is_nothrow_move_constructible<T>::value) {
			if (this != &other) {
				if (other.has_value_) {
					if (has_value_) {
						**this = std::move(*other);
					}
					else {
						new (&storage) T(std::move(*other));
						has_value_ = true;
					}
				}
				else {
					if (has_value_) {
						reinterpret_cast<T*>(&storage)->~T();
						has_value_ = false;
					}
				}
			}
			return *this;
		}

		template <typename U = T>
		typename std::enable_if<!std::is_same<
			typename std::decay<U>::type, optional>::value, optional&>::type
			operator=(U&& value) {
			if (has_value_) {
				**this = std::forward<U>(value);
			}
			else {
				new (&storage) T(std::forward<U>(value));
				has_value_ = true;
			}
			return *this;
		}

		// 访问器
		constexpr explicit operator bool() const noexcept { return has_value_; }
		constexpr bool has_value() const noexcept { return has_value_; }

		T& operator*() & {
			if (!has_value_) throw bad_optional_access();
			return *reinterpret_cast<T*>(&storage);
		}

		const T& operator*() const & {
			if (!has_value_) throw bad_optional_access();
			return *reinterpret_cast<const T*>(&storage);
		}

		T&& operator*() && {
			if (!has_value_) throw bad_optional_access();
			return std::move(*reinterpret_cast<T*>(&storage));
		}

		const T&& operator*() const && {
			if (!has_value_) throw bad_optional_access();
			return std::move(*reinterpret_cast<const T*>(&storage));
		}

		T* operator->() {
			if (!has_value_) throw bad_optional_access();
			return reinterpret_cast<T*>(&storage);
		}

		const T* operator->() const {
			if (!has_value_) throw bad_optional_access();
			return reinterpret_cast<const T*>(&storage);
		}

		T& value() & {
			if (!has_value_) throw bad_optional_access();
			return *reinterpret_cast<T*>(&storage);
		}

		const T& value() const & {
			if (!has_value_) throw bad_optional_access();
			return *reinterpret_cast<const T*>(&storage);
		}

		T&& value() && {
			if (!has_value_) throw bad_optional_access();
			return std::move(*reinterpret_cast<T*>(&storage));
		}

		const T&& value() const && {
			if (!has_value_) throw bad_optional_access();
			return std::move(*reinterpret_cast<const T*>(&storage));
		}

		template <typename U>
		T value_or(U&& default_value) const & {
			return has_value_ ? **this : static_cast<T>(std::forward<U>(default_value));
		}

		template <typename U>
		T value_or(U&& default_value) && {
			return has_value_ ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
		}

		// 修改器
		void swap(optional& other) noexcept(
			std::is_nothrow_move_constructible<T>::value&& noexcept(
				std::swap(std::declval<T&>(), std::declval<T&>()))) {
			if (has_value_ && other.has_value_) {
				using std::swap;
				swap(**this, *other);
			}
			else if (has_value_) {
				new (&other.storage) T(std::move(**this));
				reinterpret_cast<T*>(&storage)->~T();
				has_value_ = false;
				other.has_value_ = true;
			}
			else if (other.has_value_) {
				new (&storage) T(std::move(*other));
				reinterpret_cast<T*>(&other.storage)->~T();
				has_value_ = true;
				other.has_value_ = false;
			}
		}

		void reset() noexcept {
			if (has_value_) {
				reinterpret_cast<T*>(&storage)->~T();
				has_value_ = false;
			}
		}

		template <typename... Args>
		void emplace(Args&&... args) {
			if (has_value_) {
				reset();
			}
			new (&storage) T(std::forward<Args>(args)...);
			has_value_ = true;
		}

		template <typename U, typename... Args>
		void emplace(std::initializer_list<U> il, Args&&... args) {
			if (has_value_) {
				reset();
			}
			new (&storage) T(il, std::forward<Args>(args)...);
			has_value_ = true;
		}
	};

	// 比较操作
	template <typename T>
	bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
		if (lhs.has_value() != rhs.has_value()) return false;
		if (!lhs.has_value()) return true;
		return *lhs == *rhs;
	}

	template <typename T>
	bool operator!=(const optional<T>& lhs, const optional<T>& rhs) {
		return !(lhs == rhs);
	}

	template <typename T>
	bool operator<(const optional<T>& lhs, const optional<T>& rhs) {
		if (!rhs.has_value()) return false;
		if (!lhs.has_value()) return true;
		return *lhs < *rhs;
	}

	template <typename T>
	bool operator<=(const optional<T>& lhs, const optional<T>& rhs) {
		if (!lhs.has_value()) return true;
		if (!rhs.has_value()) return false;
		return *lhs <= *rhs;
	}

	template <typename T>
	bool operator>(const optional<T>& lhs, const optional<T>& rhs) {
		return rhs < lhs;
	}

	template <typename T>
	bool operator>=(const optional<T>& lhs, const optional<T>& rhs) {
		return rhs <= lhs;
	}

	template <typename T>
	bool operator==(const optional<T>& opt, nullopt_t) noexcept {
		return !opt.has_value();
	}

	template <typename T>
	bool operator==(nullopt_t, const optional<T>& opt) noexcept {
		return !opt.has_value();
	}

	template <typename T>
	bool operator!=(const optional<T>& opt, nullopt_t) noexcept {
		return opt.has_value();
	}

	template <typename T>
	bool operator!=(nullopt_t, const optional<T>& opt) noexcept {
		return opt.has_value();
	}

	template <typename T>
	bool operator<(const optional<T>&, nullopt_t) noexcept {
		return false;
	}

	template <typename T>
	bool operator<(nullopt_t, const optional<T>& opt) noexcept {
		return opt.has_value();
	}

	template <typename T>
	bool operator<=(const optional<T>& opt, nullopt_t) noexcept {
		return !opt.has_value();
	}

	template <typename T>
	bool operator<=(nullopt_t, const optional<T>&) noexcept {
		return true;
	}

	template <typename T>
	bool operator>(const optional<T>& opt, nullopt_t) noexcept {
		return opt.has_value();
	}

	template <typename T>
	bool operator>(nullopt_t, const optional<T>&) noexcept {
		return false;
	}

	template <typename T>
	bool operator>=(const optional<T>&, nullopt_t) noexcept {
		return true;
	}

	template <typename T>
	bool operator>=(nullopt_t, const optional<T>& opt) noexcept {
		return !opt.has_value();
	}

	// 与值比较
	template <typename T>
	bool operator==(const optional<T>& opt, const T& value) {
		return opt.has_value() ? *opt == value : false;
	}

	template <typename T>
	bool operator==(const T& value, const optional<T>& opt) {
		return opt.has_value() ? value == *opt : false;
	}

	template <typename T>
	bool operator!=(const optional<T>& opt, const T& value) {
		return opt.has_value() ? *opt != value : true;
	}

	template <typename T>
	bool operator!=(const T& value, const optional<T>& opt) {
		return opt.has_value() ? value != *opt : true;
	}

	template <typename T>
	bool operator<(const optional<T>& opt, const T& value) {
		return opt.has_value() ? *opt < value : true;
	}

	template <typename T>
	bool operator<(const T& value, const optional<T>& opt) {
		return opt.has_value() ? value < *opt : false;
	}

	template <typename T>
	bool operator<=(const optional<T>& opt, const T& value) {
		return opt.has_value() ? *opt <= value : true;
	}

	template <typename T>
	bool operator<=(const T& value, const optional<T>& opt) {
		return opt.has_value() ? value <= *opt : false;
	}

	template <typename T>
	bool operator>(const optional<T>& opt, const T& value) {
		return opt.has_value() ? *opt > value : false;
	}

	template <typename T>
	bool operator>(const T& value, const optional<T>& opt) {
		return opt.has_value() ? value > *opt : true;
	}

	template <typename T>
	bool operator>=(const optional<T>& opt, const T& value) {
		return opt.has_value() ? *opt >= value : false;
	}

	template <typename T>
	bool operator>=(const T& value, const optional<T>& opt) {
		return opt.has_value() ? value >= *opt : true;
	}

	// make_optional
	template <typename T>
	optional<typename std::decay<T>::type> make_optional(T&& value) {
		return optional<typename std::decay<T>::type>(std::forward<T>(value));
	}

	template <typename T, typename... Args>
	optional<T> make_optional(Args&&... args) {
		return optional<T>(in_place, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	optional<T> make_optional(std::initializer_list<U> il, Args&&... args) {
		return optional<T>(in_place, il, std::forward<Args>(args)...);
	}

	// swap
	template <typename T>
	void swap(optional<T>& lhs, optional<T>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
		lhs.swap(rhs);
	}

} // namespace std
#endif //__cplusplus

#endif //MMR_UTIL_STD_OPTIONAL_HPP





