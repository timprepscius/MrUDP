/**
 * legal header
 *
 * ============================================================================
 *
 * @author	Timothy Prepscius
 * @created 10/12/01
 */

#pragma once

namespace timprepscius::core {

inline
void small_copy_v3(char *d, const char *s, size_t size)
{
	switch(size)
	{
		case 8:
			*d++ = *s++;
			*d++ = *s++;
			*d++ = *s++;
			*d++ = *s++;
		case 4:
			*d++ = *s++;
			*d++ = *s++;
		case 2:
			*d++ = *s++;
		case 1:
			*d++ = *s++;
			return;
		default:
			;
	}
	
	while (size > 0)
	{
		switch(size)
		{
			case 4:
				*d++ = *s++;
			case 3:
				*d++ = *s++;
			case 2:
				*d++ = *s++;
			case 1:
				*d++ = *s++;
				return;
			default:
				*d++ = *s++;
				*d++ = *s++;
				*d++ = *s++;
				*d++ = *s++;
				size -= 4;
		}
	}
}

inline
void small_copy(char *d, const char *s, size_t size)
{
	small_copy_v3(d, s, size);
}

inline
bool maybe_small_copy(char *d, const char *s, size_t size)
{
	if (size > 32)
		return false;

	small_copy(d, s, size);
	return true;
}

} // namespace
