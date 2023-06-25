
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
		auto it = input.begin();

		// groups of 3 in -> groups of 4 out
		for(std::size_t i = 0; i < input.size() / 3; ++i)
		{
			const uint32_t temp {
				// the cast to unsigned char THEN uint32 is vitally important
				// otherwise on a signed char system 0xd0 turns into 0xffffffd0 !
				static_cast<uint32_t>(static_cast<unsigned char>(*it++)) << 16UL |
				static_cast<uint32_t>(static_cast<unsigned char>(*it++)) << 8UL |
				static_cast<uint32_t>(static_cast<unsigned char>(*it++)) };
			encoded += kEncodeLookup[(temp & 0x00FC0000UL) >> 18UL];
			encoded += kEncodeLookup[(temp & 0x0003F000UL) >> 12UL];
			encoded += kEncodeLookup[(temp & 0x00000FC0UL) >> 6UL ];
			encoded += kEncodeLookup[(temp & 0x0000003FUL)        ];
		}

		// string may end with a partial group
		switch(input.size() % 3)
		{
		case 2:	{
			const uint32_t temp {
				static_cast<uint32_t>(static_cast<unsigned char>(*it++)) << 16UL |
				static_cast<uint32_t>(static_cast<unsigned char>(*it++)) << 8UL };
			encoded += kEncodeLookup[(temp & 0x00FC0000UL) >> 18UL];
			encoded += kEncodeLookup[(temp & 0x0003F000UL) >> 12UL];
			encoded += kEncodeLookup[(temp & 0x00000FC0UL) >> 6UL ];
			encoded.append(1, kPadCharacter);
			break;
		}
		case 1: {
			const uint32_t temp { 
				static_cast<uint32_t>(static_cast<unsigned char>(*it++)) << 16UL };
			encoded += kEncodeLookup[(temp & 0x00FC0000UL) >> 18UL];
			encoded += kEncodeLookup[(temp & 0x0003F000UL) >> 12UL];
			encoded.append(2, kPadCharacter);
			break;
		}
		case 0: break; // do nothing
		}

		return encoded;
	}
} // namespace base64

#endif // LIBA2_BASE64_H_
