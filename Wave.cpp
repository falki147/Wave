#include "Wave.hpp"

namespace Wave {
	Format loadInfo(std::istream& stream) {
		Header h;

		if (stream.read((char*) &h, sizeof(h)).bad())
			throw std::runtime_error("failed to read");

		// Check magic number and type
		if (h.m_magicNumber != Header::RIFF || h.m_type != Header::WAVE)
			throw std::runtime_error("invalid id");

		if (findChunk(stream, SubChunk::Format) != sizeof(Format))
			throw std::runtime_error("chunk has a different size than expected");

		Format fmt;

		// Read format
		if (stream.read((char*) &fmt, sizeof(fmt)).bad())
			throw std::runtime_error("failed to read");

		if (fmt.m_format != Format::PCM)
			throw std::runtime_error("invalid format");

		if (fmt.m_bitsPerSample != 8 && fmt.m_bitsPerSample != 16)
			throw std::runtime_error("invalid bits per sample");

		return fmt;
	}

	unsigned int findChunk(std::istream& stream, unsigned int id) {
		SubChunk c;

		for (;;) {
			if (stream.read((char*) &c, sizeof(c)).bad())
				throw std::runtime_error("failed to read");

			if (c.m_type == id)
				break;

			stream.seekg(c.m_size, std::ios::cur);
		}

		return c.m_size;
	}

	void writeInfo(std::ostream& stream, unsigned int samples, Info info, unsigned int bits) {
		auto blockAlign = info.m_channels * ((bits + 7) / 8);
		auto fileSize = sizeof(Header) + 2 * sizeof(SubChunk) + sizeof(Format) + samples * blockAlign / info.m_channels - 8;

		Header h {
			Header::RIFF,
			(unsigned int) fileSize,
			Header::WAVE
		};

		if (!stream.write((const char*) &h, sizeof(h)))
			throw std::runtime_error("failed to write");

		SubChunk c {
			SubChunk::Format,
			sizeof(Format)
		};

		if (!stream.write((const char*) &c, sizeof(c)))
			throw std::runtime_error("failed to write");

		Format fmt {
			Format::PCM,
			(unsigned short) info.m_channels,
			info.m_sampleRate,
			blockAlign * info.m_sampleRate,
			(unsigned short) blockAlign,
			(unsigned short) bits
		};

		if (!stream.write((const char*) &fmt, sizeof(fmt)))
			throw std::runtime_error("failed to write");

		c = {
			SubChunk::Data,
			samples * blockAlign / info.m_channels
		};

		if (!stream.write((const char*) &c, sizeof(c)))
			throw std::runtime_error("failed to write");
	}
}
