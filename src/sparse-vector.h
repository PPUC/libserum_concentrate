#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

enum class SparseOptimization
{
	DENSE, // Always use full vectors (no InnerSparseVector)
	SPARSE // Use InnerSparseVector when beneficial
};

template <typename T>
class SparseVector
{
	static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
				  "SparseVector only supports trivial types like uint8_t or uint16_t");

	struct SparseElement
	{
		std::vector<T> dense_data;
		std::unordered_map<uint32_t, T> sparse_data;
		T noDataValue;
		bool use_sparse = false;

		SparseElement(T noData, size_t size, bool sparse_mode)
			: noDataValue(noData), use_sparse(sparse_mode)
		{
			if (!use_sparse)
			{
				dense_data.resize(size, noDataValue);
			}
		}

		T *data()
		{
			if (use_sparse)
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

		void set(size_t index, T value)
		{
			if (use_sparse)
			{
				if (value == noDataValue)
				{
					sparse_data.erase(index);
				}
				else
				{
					sparse_data[index] = value;
				}
			}
			else
			{
				dense_data[index] = value;
			}
		}

		bool empty() const
		{
			return use_sparse ? sparse_data.empty() : std::all_of(dense_data.begin(), dense_data.end(), [this](T val)
																  { return val == noDataValue; });
		}

	private:
		static thread_local std::vector<T> temp_buffer;
	};

protected:
	std::vector<std::vector<T>> index;
	std::unordered_map<uint32_t, SparseElement> data;
	std::vector<T> noData;
	bool use_index = true;
	SparseOptimization optimization = SparseOptimization::SPARSE;

public:
	SparseVector(T noDataSignature, SparseOptimization opt = SparseOptimization::SPARSE)
		: optimization(opt)
	{
		noData.resize(1, noDataSignature);
	}

	void set_optimization(SparseOptimization opt)
	{
		optimization = opt;
	}

	T *operator[](const uint32_t elementId)
	{
		if (use_index)
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
		if (use_index)
			return elementId < index.size() && !index[elementId].empty() &&
				   index[elementId][0] != noData[0];

		auto it = data.find(elementId);
		return it != data.end() && !it->second.empty();
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
			index.resize(std::max<size_t>(index.size(), elementId + 1));
			index[elementId].assign(values, values + elementSize);
		}
		else if (parent == nullptr || parent->hasData(elementId))
		{
			// Check if all elements are noData
			if (memcmp(values, noData.data(), elementSize * sizeof(T)) == 0)
			{
				data.erase(elementId);
				return;
			}

			bool use_sparse = (optimization == SparseOptimization::SPARSE);
			auto &elem = data.try_emplace(
								 elementId, noData[0], elementSize, use_sparse)
							 .first->second;

			for (size_t i = 0; i < elementSize; ++i)
			{
				if (values[i] != noData[0])
				{
					elem.set(i, values[i]);
				}
			}

			// If empty after setting (shouldn't happen due to check above)
			if (elem.empty())
			{
				data.erase(elementId);
			}
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
thread_local std::vector<T> SparseVector<T>::SparseElement::temp_buffer;