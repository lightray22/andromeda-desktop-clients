
// https://vorbrodt.blog/2019/03/23/base64-encoding/
// Licensed by Martin under BSD-0

/**
Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef LIBA2_BASE64_H_
#define LIBA2_BASE64_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace base64
{
	inline static const char kEncodeLookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

			temp  = static_cast<uint32_t>(*it++) << 16;
			temp += static_cast<uint32_t>(*it++) << 8;
			temp += static_cast<uint32_t>(*it++);
			encoded.append(1, kEncodeLookup[(temp & 0x00FC0000) >> 18]);
			encoded.append(1, kEncodeLookup[(temp & 0x0003F000) >> 12]);
			encoded.append(1, kEncodeLookup[(temp & 0x00000FC0) >> 6 ]);
			encoded.append(1, kEncodeLookup[(temp & 0x0000003F)      ]);
		}

		switch(input.size() % 3)
		{
		case 1:
			temp = static_cast<uint32_t>(*it++) << 16;
			encoded.append(1, kEncodeLookup[(temp & 0x00FC0000) >> 18]);
			encoded.append(1, kEncodeLookup[(temp & 0x0003F000) >> 12]);
			encoded.append(2, kPadCharacter);
			break;
		case 2:
			temp  = static_cast<uint32_t>(*it++) << 16;
			temp += static_cast<uint32_t>(*it++) << 8;
			encoded.append(1, kEncodeLookup[(temp & 0x00FC0000) >> 18]);
			encoded.append(1, kEncodeLookup[(temp & 0x0003F000) >> 12]);
			encoded.append(1, kEncodeLookup[(temp & 0x00000FC0) >> 6 ]);
			encoded.append(1, kPadCharacter);
			break;
		}

		return encoded;
	}
}

#endif // LIBA2_BASE64_H_
