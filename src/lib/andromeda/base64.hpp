
// https://vorbrodt.blog/2019/03/23/base64-encoding/
// Licensed by Martin under BSD-0

/**
Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef LIBA2_BASE64_H_
#define LIBA2_BASE64_H_

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace base64
{
	inline static const char kEncodeLookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // NOLINT(*-avoid-c-arrays)
	inline static const char kPadCharacter = '=';

	using byte = std::uint8_t;

	inline std::string encode(const std::string& input)
	{
		std::string encoded;
		encoded.reserve(((input.size() / 3) + (input.size() % 3 > 0)) * 4);

		std::uint32_t temp{};
		auto it = input.begin();

		for(std::size_t i = 0; i < input.size() / 3; ++i)
		{
			temp  = static_cast<uint32_t>(*it++) << 16UL;
			temp += static_cast<uint32_t>(*it++) << 8UL;
			temp += static_cast<uint32_t>(*it++);
			encoded.append(1, kEncodeLookup[(temp & 0x00FC0000UL) >> 18UL]);
			encoded.append(1, kEncodeLookup[(temp & 0x0003F000UL) >> 12UL]);
			encoded.append(1, kEncodeLookup[(temp & 0x00000FC0UL) >> 6UL ]);
			encoded.append(1, kEncodeLookup[(temp & 0x0000003FUL)      ]);
		}

		switch(input.size() % 3)
		{
		case 0: break; // do nothing
		case 1:
			temp = static_cast<uint32_t>(*it++) << 16UL;
			encoded.append(1, kEncodeLookup[(temp & 0x00FC0000UL) >> 18UL]);
			encoded.append(1, kEncodeLookup[(temp & 0x0003F000UL) >> 12UL]);
			encoded.append(2, kPadCharacter);
			break;
		case 2:
			temp  = static_cast<uint32_t>(*it++) << 16UL;
			temp += static_cast<uint32_t>(*it++) << 8UL;
			encoded.append(1, kEncodeLookup[(temp & 0x00FC0000UL) >> 18UL]);
			encoded.append(1, kEncodeLookup[(temp & 0x0003F000UL) >> 12UL]);
			encoded.append(1, kEncodeLookup[(temp & 0x00000FC0UL) >> 6UL ]);
			encoded.append(1, kPadCharacter);
			break;
		}

		return encoded;
	}
} // namespace base64

#endif // LIBA2_BASE64_H_
