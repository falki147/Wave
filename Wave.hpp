#pragma once

#include <fstream>
#include <vector>
#include <algorithm>

namespace Wave {
	struct Header {
		enum {
			RIFF = 0x46464952,
			WAVE = 0x45564157
		};

		unsigned int m_magicNumber;
		unsigned int m_size;
		unsigned int m_type;
	};

	struct SubChunk {
		enum {
			Format = 0x20746D66,
			Data = 0x61746164
		};

		unsigned int m_type;
		unsigned int m_size;
	};

	struct Format {
		enum {
			PCM = 0x0001
		};

		unsigned short m_format;
		unsigned short m_channels;
		unsigned int m_sampleRate;
		unsigned int m_bytesPerSec;
		unsigned short m_byteAlign;
		unsigned short m_bitsPerSample;
	};

	struct Info {
		unsigned short m_channels;
		unsigned int m_sampleRate;
	};

	template<typename T> struct Conversion {
		enum {
			isImplemented = false
		};

		static T from8Bit(unsigned char) { return T(); }
		static T from16Bit(short) { return T(); }
		static unsigned char to8Bit(T in) { return char(); }
		static short to16Bit(T in) { return short(); }
	};

	template<> struct Conversion<double> {
		enum {
			isImplemented = true
		};

		static double from8Bit(unsigned char in) { return (in / (std::numeric_limits<unsigned char>::max() / 2.0)) - 1; }
		static double from16Bit(short in) { return in / (double) std::numeric_limits<short>::max(); }
		static unsigned char to8Bit(double in) { return (unsigned char) ((in + 1) / (2 * std::numeric_limits<unsigned char>::max())); }
		static short to16Bit(double in) { return (short) (in * std::numeric_limits<short>::max()); }
	};

	template<> struct Conversion<float> {
		enum {
			isImplemented = true
		};

		static float from8Bit(unsigned char in) { return (in / (std::numeric_limits<unsigned char>::max() / 2.0f)) - 1; }
		static float from16Bit(short in) { return in / (float) std::numeric_limits<short>::max(); }
		static unsigned char to8Bit(float in) { return (unsigned char) ((in + 1) / (2 * std::numeric_limits<unsigned char>::max())); }
		static short to16Bit(float in) { return (short) (in * std::numeric_limits<short>::max()); }
	};

	Format loadInfo(std::istream& stream);
	unsigned int findChunk(std::istream& stream, unsigned int id);
	void writeInfo(std::ostream& stream, unsigned int samples, Info info, unsigned int bits);

	template<typename T> std::vector<T> load(std::istream& stream, Info& info, unsigned int maxChannels = 0) {
		static_assert(Conversion<T>::isImplemented, "Conversion is not implemented for type T");

		// Load format struct from file (also check headers)
		auto fmt = loadInfo(stream);

		stream.seekg(sizeof(Header));

		// Find data section in file and calculate samples from chunk size
		auto size = findChunk(stream, SubChunk::Data);
		auto samples = size / fmt.m_byteAlign;

		// Read data from file
		std::vector<char> data(size);

		if (stream.read(data.data(), data.size()).bad())
			throw std::runtime_error("failed to read");

		auto cur = data.begin();

		// Recalculate end instead of using size
		auto end = data.begin() + samples * fmt.m_byteAlign;

		auto channels = maxChannels ? std::min<unsigned int>(maxChannels, fmt.m_channels) : fmt.m_channels;

		std::vector<T> out;
		out.reserve(channels * samples);

		// Read data from memory
		switch (fmt.m_bitsPerSample) {
		case 8:
			while (cur < end) {
				for (unsigned int i = 0; i < channels; ++i)
					out.push_back(Conversion<T>::from8Bit(*(unsigned char*) &*(cur + i)));

				cur += fmt.m_byteAlign;
			}
			break;
		case 16:
			while (cur < end) {
				for (unsigned int i = 0; i < channels; ++i)
					out.push_back(Conversion<T>::from16Bit(*(short*) &*(cur + i)));

				cur += fmt.m_byteAlign;
			}
			break;
		}

		info.m_channels = channels;
		info.m_sampleRate = fmt.m_sampleRate;

		return out;
	}

	template<typename T> void save(std::ostream& stream, const std::vector<T>& samples, Info info, unsigned int bits) {
		static_assert(Conversion<T>::isImplemented, "Conversion is not implemented for type T");

		if (bits != 8 && bits != 16)
			throw std::runtime_error("invalid bits value");

		writeInfo(stream, samples.size(), info, bits);

		switch (bits) {
		case 8:
			for (auto val : samples) {
				auto tmp = Conversion<T>::to8Bit(val);

				if (!stream.write((const char*) &tmp, sizeof(tmp)))
					throw std::runtime_error("failed to write");
			}
			break;
		case 16:
			for (auto val : samples) {
				auto tmp = Conversion<T>::to16Bit(val);

				if (!stream.write((const char*) &tmp, sizeof(tmp)))
					throw std::runtime_error("failed to write");
			}
			break;
		}
	}
}
