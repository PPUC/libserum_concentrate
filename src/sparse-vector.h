#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <algorithm>

template <typename T>
class SparseVector
{
	static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
				  "SparseVector only supports trivial types");

	class InnerVector
	{
		std::unordered_map<uint32_t, T> sparse_data;
		std::vector<T> dense_data;
		T noDataValue;
		bool sparse_mode;

		// Thread-local reconstruction buffer
		static thread_local std::vector<T> temp_buffer;

	public:
		InnerVector(T noData, size_t size, bool sparse)
			: noDataValue(noData), sparse_mode(sparse)
		{
			if (!sparse_mode)
			{
				dense_data.resize(size, noDataValue);
			}
		}

		T &operator[](size_t idx)
		{
			if (sparse_mode)
			{
				temp_buffer.resize(dense_data.size(), noDataValue);
				for (const auto &entry : sparse_data)
				{
					if (entry.first < temp_buffer.size())
					{
						temp_buffer[entry.first] = entry.second;
					}
				}
				return temp_buffer[idx];
			}
			return dense_data[idx];
		}

		T *data()
		{
			if (sparse_mode)
			{
				temp_buffer.resize(dense_data.size(), noDataValue);
				for (const auto &entry : sparse_data)
				{
					if (entry.first < temp_buffer.size())
					{
						temp_buffer[entry.first] = entry.second;
					}
				}
				return temp_buffer.data();
			}
			return dense_data.data();
		}

		void set(size_t idx, T value)
		{
			if (sparse_mode)
			{
				if (value == noDataValue)
				{
					sparse_data.erase(idx);
				}
				else
				{
					sparse_data[idx] = value;
				}
			}
			else
			{
				dense_data[idx] = value;
			}
		}

		bool empty() const
		{
			return sparse_mode ? sparse_data.empty() : (dense_data.empty() || std::all_of(dense_data.begin(), dense_data.end(), [this](T v)
																						  { return v == noDataValue; }));
		}
	};

	std::vector<std::vector<T>> index;
	std::unordered_map<uint32_t, InnerVector> data;
	std::vector<T> noData;
	bool use_index = true;
	bool sparse_optimization = true;

public:
	SparseVector(T noDataSignature, bool optimization = false)
		: sparse_optimization(optimization)
	{
		noData.resize(1, noDataSignature);
	}

	// Unified operator[] that returns a proxy object
	class Proxy
	{
		T *data;
		size_t size;
		T noDataValue;

	public:
		Proxy(T *d, size_t s, T nd) : data(d), size(s), noDataValue(nd) {}

		// Conversion to T* for [x] usage
		operator T *() { return data; }

		// [x][y] access
		T &operator[](size_t idx)
		{
			return (idx < size) ? data[idx] : noDataValue;
		}
	};

	// Single operator[] that handles both cases
	Proxy operator[](uint32_t elementId)
	{
		if (use_index)
		{
			if (elementId >= index.size())
				return Proxy(noData.data(), noData.size(), noData[0]);
			return Proxy(index[elementId].data(), index[elementId].size(), noData[0]);
		}
		auto it = data.find(elementId);
		if (it == data.end())
			return Proxy(noData.data(), noData.size(), noData[0]);
		return Proxy(it->second.data(), noData.size(), noData[0]);
	}

	// Public hasData method
	bool hasData(uint32_t elementId) const
	{
		if (use_index)
		{
			return elementId < index.size() && !index[elementId].empty();
		}
		return data.find(elementId) != data.end();
	}

	template <typename U = T>
	void set(uint32_t elementId, const T *values, size_t elementSize,
			 SparseVector<U> *parent = nullptr)
	{
		if (elementSize > 1 || parent != nullptr)
		{
			use_index = false;
		}

		if (noData.size() < elementSize)
		{
			noData.resize(elementSize, noData[0]);
		}

		if (use_index)
		{
			index.resize(std::max(index.size(), size_t(elementId + 1)));
			index[elementId].assign(values, values + elementSize);
			return;
		}

		if (parent && !parent->hasData(elementId))
		{
			return;
		}

		// Fast check for all-noData case
		if (memcmp(values, noData.data(), elementSize * sizeof(T)) == 0)
		{
			data.erase(elementId);
			return;
		}

		// Efficient insertion/update
		auto result = data.try_emplace(
			elementId,
			noData[0],			// noDataValue
			elementSize,		// size
			sparse_optimization // sparse mode
		);

		InnerVector &vec = result.first->second;
		for (size_t i = 0; i < elementSize; ++i)
		{
			vec.set(i, values[i]);
		}

		if (vec.empty())
		{
			data.erase(elementId);
		}
	}

	template <typename U = T>
	void my_fread(size_t elementSize, uint32_t numElements, FILE *stream,
				  SparseVector<U> *parent = nullptr)
	{
		std::vector<T> tmp(elementSize);

		for (uint32_t i = 0; i < numElements; ++i)
		{
			if (fread(tmp.data(), elementSize * sizeof(T), 1, stream) != 1)
			{
				fprintf(stderr, "File read error\n");
				exit(1);
			}
			set(i, tmp.data(), elementSize, parent);
		}
	}

	void clearIndex() { index.clear(); }

	void clear()
	{
		index.clear();
		data.clear();
		noData.clear();
	}
};

template <typename T>
thread_local std::vector<T> SparseVector<T>::InnerVector::temp_buffer;
