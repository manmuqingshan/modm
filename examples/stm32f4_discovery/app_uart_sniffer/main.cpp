/*
 * Copyright (c) 2015, Kevin Läufer
 * Copyright (c) 2015, 2017, Sascha Schade
 * Copyright (c) 2015-2018, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/board.hpp>
#include <modm/debug/logger.hpp>
#include <modm/processing/timer.hpp>
#include <inttypes.h>

using Usart1 = BufferedUart<UsartHal1>;
using Usart2 = BufferedUart<UsartHal2>;
using Usart3 = BufferedUart<UsartHal3>;
modm::IODeviceWrapper< Usart2, modm::IOBuffer::BlockIfFull > loggerDevice;

// Set all four logger streams to use the UART
modm::log::Logger modm::log::debug(loggerDevice);
modm::log::Logger modm::log::info(loggerDevice);
modm::log::Logger modm::log::warning(loggerDevice);
modm::log::Logger modm::log::error(loggerDevice);

// Set the log level
#undef	MODM_LOG_LEVEL
#define	MODM_LOG_LEVEL modm::log::DEBUG



enum class Direction {
	Init,
	Host2Node,
	Node2Host,
};

Direction direction = Direction::Init;

void
setDirection(Direction dir)
{
	if (direction == dir) {
		// no change
	} else {
		direction = dir;
		static uint16_t counter = 0;
		static modm::Duration lastTimestamp = modm::Clock::now().time_since_epoch();

		modm::Duration timestamp = modm::Clock::now().time_since_epoch();

		MODM_LOG_INFO.printf("\e[39m\n%04" PRId16 " %02" PRId32 ":%03" PRId32 " +%01" PRId32 ":%03" PRId32 " ",
				counter,
				timestamp.count() / 1000,
				timestamp.count() % 1000,
				(timestamp.count() - lastTimestamp.count()) / 1000,
				(timestamp.count() - lastTimestamp.count()) % 1000);

		switch (direction)
		{
		case Direction::Init:
			// Error
			break;
		case Direction::Host2Node:
			MODM_LOG_INFO.printf("\e[91m");
			break;
		case Direction::Node2Host:
			MODM_LOG_INFO.printf("\e[92m");
			break;
		}

		lastTimestamp = timestamp;
		++counter;
	}
}

// ----------------------------------------------------------------------------
/**
 *
 */
int
main()
{
	Board::initialize();

	// Enable USART 2: To / from PC
	Usart2::connect<GpioOutputA2::Tx, GpioInputA3::Rx>();
	Usart2::initialize<Board::SystemClock, 115200_Bd>();

	// Enable USART 1 Host To Node
	Usart1::connect<GpioInputA10::Rx>();
	Usart1::initialize<Board::SystemClock, 115200_Bd>();

	// Enable USART 3 Node to Host
	Usart3::connect<GpioInputD9::Rx>();
	Usart3::initialize<Board::SystemClock, 115200_Bd>();

	MODM_LOG_INFO.printf("\e[H\e[J\e[39m");
	MODM_LOG_INFO.printf("Welcome to MODM Bidirectional UART Sniffer.\n\n");
	MODM_LOG_INFO.printf("\e[91mRed PD9    \e[92mGreen PA10\n\n\e[39m");
	MODM_LOG_INFO.printf("ctr   time  relati data\n");
	MODM_LOG_INFO.printf("==== ====== ====== ===== ...\n");

	while (true)
	{
		uint8_t c;
		while (Usart3::read(c)) {
			setDirection(Direction::Node2Host);
			MODM_LOG_INFO.printf("%02x ", c);
			Board::LedRed::toggle();
		}
		while (Usart1::read(c)) {
			setDirection(Direction::Host2Node);
			MODM_LOG_INFO.printf("%02x ", c);
			Board::LedGreen::toggle();
		}

		modm::delay(100us);
	}

	return 0;
}
