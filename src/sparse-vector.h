#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
class SparseVector
{
	static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
				  "SparseVector only supports trivial types like uint8_t or uint16_t");

protected:
	std::vector<std::vector<T>> index;
	std::unordered_map<uint32_t, std::vector<T>> data;
	std::vector<T> noData;
	bool useIndex;

public:
	SparseVector(T noDataSignature, bool index)
		: useIndex(index)
	{
		noData.resize(1, noDataSignature);
	}

	SparseVector(T noDataSignature)
	{
		useIndex = false;
		noData.resize(1, noDataSignature);
	}

	T *operator[](const uint32_t elementId)
	{
		if (useIndex)
		{
			if (elementId >= index.size())
				return noData.data();
			return index[elementId].data();
		}
		else
		{
			auto it = data.find(elementId);
			if (it == data.end())
				return noData.data();
			return it->second.data();
		}
	}

	bool hasData(uint32_t elementId) const
	{
		if (useIndex)
			return elementId < index.size() && !index[elementId].empty() && index[elementId][0] != noData[0];
		return data.find(elementId) != data.end();
	}

	template <typename U = T>
	void set(uint32_t elementId, const T *values, size_t elementSize, SparseVector<U> *parent = nullptr)
	{
		if (useIndex)
		{
			fprintf(stderr, "set() must not be used for index\n");
			exit(2);
		}

		if (noData.size() < elementSize)
		{
			noData.resize(elementSize, noData[0]);
		}

		if (parent == nullptr || parent->hasData(elementId))
		{
			if (memcmp(values, noData.data(), elementSize * sizeof(T)) != 0)
			{
				// asign() takes a pointers to the first and the last element of the array
				data[elementId].assign(values, values + elementSize);
			}
		}
	}

	template <typename U = T>
	void my_fread(size_t elementSize, uint32_t numElements, FILE *stream, SparseVector<U> *parent = nullptr)
	{
		if (useIndex)
		{
			index.resize(numElements);
			for (uint32_t i = 0; i < numElements; ++i)
			{
				index[i].resize(elementSize);
				if (fread(index[i].data(), sizeof(T), elementSize, stream) != elementSize)
				{
					fprintf(stderr, "File read error\n");
					exit(1);
				}
			}
		}
		else
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
	}

	void reserve(size_t elementSize)
	{
		if (noData.size() < elementSize)
		{
			noData.resize(elementSize, noData[0]);
		}
	}

	void clearIndex()
	{
		index.clear();
	}

	void clear()
	{
		index.clear();
		data.clear();
		noData.resize(1);
	}
};
