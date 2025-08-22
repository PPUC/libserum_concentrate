#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include <algorithm>
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"

class LZ4Stream
{
public:
	static constexpr size_t CHUNK_SIZE = 64 * 1024;

	LZ4Stream(FILE *file, bool writing, int compressionLevel = 12) // Default is max compression
		: file_(file), writing_(writing), compressionLevel_(compressionLevel)
	{
		if (writing)
		{
			ctx_hc_ = LZ4_createStreamHC();
			LZ4_resetStreamHC(ctx_hc_, compressionLevel_);
			outBuf_.resize(LZ4_COMPRESSBOUND(CHUNK_SIZE));
		}
		else
		{
			dctx_ = LZ4_createStreamDecode();
			inBuf_.resize(CHUNK_SIZE);
		}
	}

	~LZ4Stream()
	{
		if (writing_)
		{
			flush();
			LZ4_freeStreamHC(ctx_hc_);
		}
		else
		{
			LZ4_freeStreamDecode(dctx_);
		}
	}

	bool write(const void *data, size_t size)
	{
		const char *src = static_cast<const char *>(data);
		size_t remaining = size;

		while (remaining > 0)
		{
			size_t chunk = std::min(remaining, CHUNK_SIZE);
			int compressedSize = LZ4_compress_HC_continue(
				ctx_hc_, src, (char *)outBuf_.data(),
				chunk, outBuf_.size());

			if (compressedSize <= 0)
				return false;

			// Write compressed chunk size and data
			uint32_t chunkSize = compressedSize;
			if (fwrite(&chunkSize, sizeof(chunkSize), 1, file_) != 1)
				return false;
			if (fwrite(outBuf_.data(), 1, compressedSize, file_) != compressedSize)
				return false;

			src += chunk;
			remaining -= chunk;
		}
		return true;
	}

	bool read(void *data, size_t size)
	{
		char *dst = static_cast<char *>(data);
		size_t remaining = size;

		while (remaining > 0)
		{
			uint32_t compressedSize;
			if (fread(&compressedSize, sizeof(compressedSize), 1, file_) != 1)
				return false;

			if (fread(inBuf_.data(), 1, compressedSize, file_) != compressedSize)
				return false;

			size_t chunk = std::min(remaining, CHUNK_SIZE);
			int decompressedSize = LZ4_decompress_safe_continue(
				dctx_, (char *)inBuf_.data(), dst,
				compressedSize, chunk);

			if (decompressedSize <= 0)
				return false;

			dst += decompressedSize;
			remaining -= decompressedSize;
		}
		return true;
	}

	void flush()
	{
		if (writing_)
		{
			// Final compression if needed
		}
	}

private:
	FILE *file_;
	bool writing_;
	int compressionLevel_;
	LZ4_streamHC_t *ctx_hc_ = nullptr;
	LZ4_streamDecode_t *dctx_ = nullptr;
	std::vector<uint8_t> inBuf_;
	std::vector<uint8_t> outBuf_;
};
