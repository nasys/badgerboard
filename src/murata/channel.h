#pragma once

#include <stddef.h>
#include <stdint.h>

class Channel
{
public:
	Channel() = default;

	virtual void open() = 0;
	virtual void transmit(uint8_t* buffer, size_t size) const = 0;
	virtual size_t receive(uint8_t* buffer, size_t size) const = 0;

	virtual ~Channel() = default;
};
