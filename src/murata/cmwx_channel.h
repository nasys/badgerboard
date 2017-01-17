#pragma once

#include "channel.h"

class CmwxChannel final : public Channel
{
public:
	CmwxChannel() = default;

	void open() override;
	void transmit(uint8_t* buffer, size_t size) const override;
	size_t receive(uint8_t* buffer, size_t size) const override;

	~CmwxChannel() = default;
};
