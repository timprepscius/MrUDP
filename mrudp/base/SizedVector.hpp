#pragma once

namespace timprepscius::core {

template<typename T>
using Vector = std::vector<T>;

template<typename T>
struct SizedVector
{
	using V = Vector<T>;
	using iterator = typename V::iterator;
	using const_iterator = typename V::const_iterator;
	using value_type = typename V::value_type;
	using reference = typename V::reference;
	
	V v;
	size_t size_ = 0;
	
	auto reserved() const { return v.size(); }
	
	void reserve(size_t size)
	{
		if (size > v.size())
			v.resize(size);
	}
	
	auto capacity () const {
		return v.size();
	}
	
	auto resize (size_t size)
	{
		reserve(size);
		return size_ = size;
	}
	
	auto &front () { return v.front(); }
	auto &front () const { return v.front(); }

	auto &back () { return at(size_-1); }
	auto &back () const { return at(size_-1); }

	auto begin () { return v.begin(); }
	auto end() { return v.begin() + size(); };
	auto begin () const { return v.begin(); }
	auto end() const { return v.begin() + size(); };

	auto &at(size_t i)
	{
		debug_assert(i < size_);
		return v[i];
	}
	
	auto &at(size_t i) const
	{
		debug_assert(i < size_);
		return v[i];
	}

	auto &operator[](size_t i) { return at(i); }
	auto &operator[](size_t i) const { return at(i); }
	
	auto *data() { return v.data(); }
	auto *data() const { return v.data(); }
	
	auto empty() const { return size_ == 0; }
	auto size() const { return size_; }
	
	auto push_back(const T &t)
	{
		debug_assert(size_ < v.size());
		
		if (size_ < v.size())
		{
			size_++;
			back() = t;
		}
	}
	
	auto pop_back(const T &t)
	{
		debug_assert(size_ > 0);
		if (size_ > 0)
			size_--;
	}
	
	// can only be used to remove a portion at the end (as of now)
	template<typename A, typename B>
	auto erase(A &&a, B &&b)
	{
		debug_assert(b == end());
		auto distance = std::distance(a, b);
		size_ -= distance;
	}
	
} ;

} // namespace
