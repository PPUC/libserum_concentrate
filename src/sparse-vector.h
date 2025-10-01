#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>

#include "LZ4Stream.h"
template <typename T>
class SparseVector
{
	static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
				  "SparseVector only supports trivial types like uint8_t or uint16_t");

protected:
	std::vector<std::vector<T>> index;
	std::unordered_map<uint32_t, std::vector<uint8_t>> data; // Changed to uint8_t for compressed data
	std::vector<T> noData;
	size_t elementSize = 0; // Size of each element in bytes
	std::vector<T> decompBuffer;
	bool useIndex;
	bool useCompression;
	mutable uint32_t lastAccessedId = UINT32_MAX;
	mutable std::vector<T> lastDecompressed;

public:
	SparseVector(T noDataSignature, bool index, bool compress = false)
		: useIndex(index), useCompression(compress), lastAccessedId(UINT32_MAX)
	{
		noData.resize(1, noDataSignature);
	}

	SparseVector(T noDataSignature)
		: useIndex(false), useCompression(false)
	{
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

			if (useCompression)
			{
				// Cache-Hit
				if (elementId == lastAccessedId)
				{
					return lastDecompressed.data();
				}

				const auto &compressed = it->second;

				// ensure decompBuffer is large enough
				if (lastDecompressed.size() < elementSize)
				{
					lastDecompressed.resize(elementSize);
				}

				int decompressedSize = LZ4_decompress_safe(
					reinterpret_cast<const char *>(compressed.data()),
					reinterpret_cast<char *>(lastDecompressed.data()),
					static_cast<int>(compressed.size()),
					static_cast<int>(elementSize * sizeof(T)));

				if (decompressedSize < 0)
					return noData.data();

				// Cache-Update
				lastAccessedId = elementId;
				return lastDecompressed.data();
			}

			return reinterpret_cast<T *>(it->second.data());
		}
	}

	bool hasData(uint32_t elementId) const
	{
		if (useIndex)
			return elementId < index.size() && !index[elementId].empty() && index[elementId][0] != noData[0];
		return data.find(elementId) != data.end();
	}

	template <typename U = T>
	void set(uint32_t elementId, const T *values, size_t size, SparseVector<U> *parent = nullptr)
	{
		if (useIndex)
		{
			throw std::runtime_error("set() must not be used for index");
		}

		elementSize = size;

		if (decompBuffer.size() < (elementSize * sizeof(T)))
		{
			decompBuffer.resize(elementSize * sizeof(T));
		}

		if (noData.size() < elementSize)
		{
			noData.resize(elementSize, noData[0]);
		}

		if (parent == nullptr || parent->hasData(elementId))
		{
			if (memcmp(values, noData.data(), elementSize * sizeof(T)) != 0)
			{
				if (useCompression)
				{
					const size_t maxCompressedSize = LZ4_compressBound(static_cast<int>(elementSize * sizeof(T)));
					std::vector<uint8_t> compBuffer(maxCompressedSize);

					int compressedSize = LZ4_compress_HC(
						reinterpret_cast<const char *>(values),
						reinterpret_cast<char *>(compBuffer.data()),
						static_cast<int>(elementSize * sizeof(T)),
						static_cast<int>(maxCompressedSize),
						12 // max compression level
					);

					if (compressedSize > 0)
					{
						data[elementId].assign(
							compBuffer.begin(),
							compBuffer.begin() + compressedSize);
					}
				}
				else
				{
					// Without compression, store directly.
					const uint8_t *byteValues = reinterpret_cast<const uint8_t *>(values);
					data[elementId].assign(byteValues, byteValues + elementSize * sizeof(T));
				}
			}
		}
	}

	template <typename U = T>
	void readFromCRomFile(size_t elementSize, uint32_t numElements, FILE *stream, SparseVector<U> *parent = nullptr)
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
		lastAccessedId = UINT32_MAX;
		lastDecompressed.clear();
	}

	void saveToStream(LZ4Stream &stream) const
	{
		// Flags
		uint8_t flags = (useIndex ? 1 : 0) | (useCompression ? 2 : 0);
		stream.write(&flags, sizeof(flags));

		// Element size and default value
		stream.write(&elementSize, sizeof(elementSize));
		size_t noDataSize = noData.size();
		stream.write(&noDataSize, sizeof(noDataSize));
		stream.write(noData.data(), sizeof(T) * noDataSize);

		if (useIndex)
		{
			size_t indexSize = index.size();
			stream.write(&indexSize, sizeof(indexSize));
			for (const auto &vec : index)
			{
				size_t vecSize = vec.size();
				stream.write(&vecSize, sizeof(vecSize));
				if (vecSize > 0)
				{
					stream.write(vec.data(), sizeof(T) * vecSize);
				}
			}
		}
		else
		{
			uint32_t count = data.size();
			stream.write(&count, sizeof(count));
			for (const auto &entry : data)
			{
				stream.write(&entry.first, sizeof(entry.first));
				size_t dataSize = entry.second.size();
				stream.write(&dataSize, sizeof(dataSize));
				stream.write(entry.second.data(), dataSize);
			}
		}
	}

	void loadFromStream(LZ4Stream &stream)
	{
		clear();

		// Flags
		uint8_t flags;
		stream.read(&flags, sizeof(flags));
		useIndex = (flags & 1) != 0;
		useCompression = (flags & 2) != 0;

		// Element size and default value
		stream.read(&elementSize, sizeof(elementSize));
		size_t noDataSize;
		stream.read(&noDataSize, sizeof(noDataSize));
		noData.resize(noDataSize);
		stream.read(noData.data(), sizeof(T) * noDataSize);

		if (useIndex)
		{
			size_t indexSize;
			stream.read(&indexSize, sizeof(indexSize));
			index.resize(indexSize);
			for (size_t i = 0; i < indexSize; i++)
			{
				size_t vecSize;
				stream.read(&vecSize, sizeof(vecSize));
				if (vecSize > 0)
				{
					index[i].resize(vecSize);
					stream.read(index[i].data(), sizeof(T) * vecSize);
				}
			}
		}
		else
		{
			uint32_t count;
			stream.read(&count, sizeof(count));
			for (uint32_t i = 0; i < count; i++)
			{
				uint32_t key;
				stream.read(&key, sizeof(key));
				size_t dataSize;
				stream.read(&dataSize, sizeof(dataSize));

				std::vector<uint8_t> dataBuf(dataSize);
				stream.read(dataBuf.data(), dataSize);
				data[key] = std::move(dataBuf);
			}
		}

		lastAccessedId = UINT32_MAX;
		lastDecompressed.clear();
	}

	template <typename U = T>
	void setParent(SparseVector<U> *parent)
	{
		if (!parent || useIndex)
		{
			return; // Parent cannot be set for index-based vectors or if no parent is provided
		}

		std::unordered_map<uint32_t, std::vector<uint8_t>> filteredData;

		for (const auto &entry : data)
		{
			uint32_t elementId = entry.first;

			if (parent->hasData(elementId))
			{
				filteredData[elementId] = std::move(data[elementId]);
			}
		}

		data = std::move(filteredData);

		// Clear cache
		lastAccessedId = UINT32_MAX;
		lastDecompressed.clear();
	}
};
