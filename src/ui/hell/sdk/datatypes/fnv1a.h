#pragma once
#include <cstddef>
#include <string>

typedef uint64_t fnv1a_t;

namespace fnv1a
{
	constexpr uint32_t val_32_const = 0x811c9dc5;
	constexpr uint32_t prime_32_const = 0x1000193;
	constexpr fnv1a_t val_64_const = 0xcbf29ce484222325;
	constexpr fnv1a_t prime_64_const = 0x100000001b3;

	inline constexpr uint32_t hash_32( const char* const str, const uint32_t value = val_32_const ) noexcept
	{
		return (str[0] == '\0') ? value : hash_32( &str[1], (value ^ uint32_t( (uint8_t)str[0] )) * prime_32_const );
	}

	inline constexpr fnv1a_t hash_64( const char* const str, const fnv1a_t value = val_64_const ) noexcept
	{
		return (str[0] == '\0') ? value : hash_64( &str[1], (value ^ fnv1a_t( (uint8_t)str[0] )) * prime_64_const );
	}
}

#define fnv_hash(str) fnv1a::hash_64(str)