/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "commands.h"
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include "config.h"
#include "lib/console/console.h"
#include "lib/uart/uart.h"
#include "lib/max7456/max7456.h"
#include "settings.h"
#include "osd/panel.h"
#include "telemetry/telemetry.h"

namespace console
{

const char _str_done [] PROGMEM = "Done.\r\n";

uint8_t read ()
{
	while (true)
	{
		uint16_t c = CONSOLE_UART::receive ();
		if (!(c & 0xff00)) return c;
	}
}

namespace commands
{

namespace font
{

	const char command [] PROGMEM = "font";
	const char help [] PROGMEM = "Upload and download mcm-file";

	void draw ()
	{
		uint8_t left = max7456::hcenter - 8;
		max7456::clear ();
		for (uint8_t h = 0; h < 0x10; h ++)
			for (uint8_t l = 0; l < 0x10; l ++)
				max7456::put (left + l, h, (h << 4) | l);
	}

	void print_byte (uint8_t b)
	{
		for (uint8_t i = 0; i < 8; i ++)
			CONSOLE_UART::send ((b >> i) & 1 ? '1' : '0');
		eol ();
	}

	void download ()
	{
		draw ();
		CONSOLE_UART::send_string_p (PSTR ("MAX7456\r\n"));
		for (uint16_t c = 0; c < 0x100; c ++)
		{
			uint8_t data [54];
			max7456::download_char (c, data);
			for (uint8_t i = 0; i < 54; i ++)
				print_byte (data [i]);
			for (uint8_t i = 0; i < 10; i ++)
				print_byte (0);
		}
	}

	uint8_t read_byte ()
	{
		uint8_t res = 0;
		for (uint8_t i = 0; i < 8; i ++)
			res = (res << 1) | ((console::read () - '0') & 1);
		// \r\n
		console::read ();
		console::read ();
		return res;
	}

	void upload ()
	{
		draw ();
		CONSOLE_UART::send_string_p (PSTR ("Send MCM-file\r\n"));
		for (uint8_t i = 0; i < 9; i ++)
			read ();
		for (uint16_t c = 0; c < 0x100; c ++)
		{
			uint8_t data [54];
			for (uint8_t i = 0; i < 54; i ++)
				data [i] = read_byte ();
			for (uint8_t i = 0; i < 10; i ++)
				read_byte ();
			max7456::upload_char (c, data);
		}
		CONSOLE_UART::send_string_p (_str_done);
	}

	void exec ()
	{
		const char *arg = console::argument (1);
		if (arg)
		{
			switch (*arg) {
				case 'u':
				case 'U':
					upload ();
					return;
				case 'd':
				case 'D':
					download ();
			}		return;
		}
		CONSOLE_UART::send_string_p (PSTR ("Args: u - upload, d - download"));
	}

} // namespace font


namespace reset
{

	const char command [] PROGMEM = "reset";
	const char help [] PROGMEM = "Reset settings to defaults";

	void exec ()
	{
		CONSOLE_UART::send_string_p (PSTR ("Reset to defaults... "));
		settings::reset ();
		CONSOLE_UART::send_string_p (_str_done);
	}

}  // namespace reset

namespace eeprom
{

	const char command [] PROGMEM = "eeprom";
	const char help [] PROGMEM = "Read/write EEPROM";

	void dump ()
	{
		for (uint8_t row = 0; row < EEPROM_SIZE / 16; row ++)
		{
			fprintf_P (&CONSOLE_UART::stream, PSTR ("%04x: "), row << 4);
			for (uint8_t byte = 0; byte < 16; byte ++)
				fprintf_P (&CONSOLE_UART::stream, PSTR ("%02x "), eeprom_read_byte ((uint8_t *) ((row << 4) | byte)));
			console::eol ();
		}
	}

	void read ()
	{
		for (uint16_t addr = 0; addr < EEPROM_SIZE; addr ++)
			CONSOLE_UART::send (eeprom_read_byte ((uint8_t *) addr));
	}

	void write ()
	{
		for (uint16_t addr = 0; addr < EEPROM_SIZE; addr ++)
			eeprom_update_byte ((uint8_t *) addr, console::read ());
	}

	void exec ()
	{
		const char *arg = console::argument (1);
		if (arg)
		{
			switch (*arg)
			{
				case 'd':
				case 'D':
					dump ();
					return;
				case 'r':
				case 'R':
					read ();
					return;
				case 'w':
				case 'W':
					write ();
					return;
			}
		}
		CONSOLE_UART::send_string_p (PSTR ("Args: d - dump, r - read, w - write"));
	}

}  // namespace eeprom


namespace opt
{

	const char command [] PROGMEM = "opt";
	const char help [] PROGMEM = "Read/write OSD options";

	const char __unknown [] PROGMEM = "Unknown option";

	const char __uint [] PROGMEM = "%u";
	const char __float [] PROGMEM = "%0.4f";

	const char __t_bool [] PROGMEM = "bool";
	const char __t_byte [] PROGMEM = "byte";
	const char __t_word [] PROGMEM = "word";
	const char __t_dword [] PROGMEM = "dword";
	const char __t_float [] PROGMEM = "float";
	const char __t_str [] PROGMEM = "str";

	const char * const types [] PROGMEM = {
		__t_bool, __t_byte, __t_word, __t_dword, __t_float, __t_str
	};

	void display (const settings::option_t *option)
	{
		if (!option)
		{
			CONSOLE_UART::send_string_p (__unknown);
			return;
		}

		const char *name_p = (const char *) pgm_read_ptr (&option->name_p);
		void *addr = pgm_read_ptr (&option->addr);
		uint8_t type = pgm_read_byte (&option->type);
		uint8_t size = pgm_read_byte (&option->size);

		fprintf_P (&CONSOLE_UART::stream, PSTR ("%03p\t(%S:%u@)\t%S\t= "), addr, (const char *) pgm_read_ptr (&types [type]), size, name_p);

		char buf [16];

		switch (type)
		{
			case settings::ot_bool:
				fprintf_P (&CONSOLE_UART::stream, __uint, settings::read_bool_option (option));
				break;
			case settings::ot_uint8:
				fprintf_P (&CONSOLE_UART::stream, __uint, settings::read_uint8_option (option));
				break;
			case settings::ot_uint16:
				fprintf_P (&CONSOLE_UART::stream, __uint, settings::read_uint16_option (option));
				break;
			case settings::ot_uint32:
				ltoa (settings::read_uint32_option (option), buf, 10);
				CONSOLE_UART::send_string (buf);
				break;
			case settings::ot_float:
				fprintf_P (&CONSOLE_UART::stream, __float, settings::read_float_option (option));
				break;
			case settings::ot_str:
				settings::read_str_option (option, buf);
				CONSOLE_UART::send_string (buf);
				break;
		}
	}

	void list ()
	{
		for (uint8_t s = 0; s < settings::sections_count; s ++)
		{
			const settings::option_t *opts = settings::sections [s].options;
			for (uint8_t i  = 0; i < settings::sections [s].size; i ++)
			{
				display (opts + i);
				console::eol ();
			}
		}
	}

	void get ()
	{
		const char *name = console::argument (2);

		if (!name)
		{
			CONSOLE_UART::send_string_p (PSTR ("Args: <name>"));
			return;
		}

		display (settings::get_option (name));
	}

	void set ()
	{
		const char *name_arg = console::argument (2);
		const char *value = console::argument (3);

		if (!name_arg || !value)
		{
			CONSOLE_UART::send_string_p (PSTR ("Args: <name> <value>"));
			return;
		}

		char name [SETTINGS_MAX_NAME_LEN];
		console::read_argument (name_arg, name);

		const settings::option_t *option = settings::get_option (name);

		if (!option)
		{
			CONSOLE_UART::send_string_p (__unknown);
			return;
		}

		uint8_t type = pgm_read_byte (&option->type);

		switch (type)
		{
			case settings::ot_bool:
				settings::write_bool_option (name, atoi (value));
				break;
			case settings::ot_uint8:
				settings::write_uint8_option (name, atoi (value));
				break;
			case settings::ot_uint16:
				settings::write_uint16_option (name, atol (value));
				break;
			case settings::ot_uint32:
				settings::write_uint32_option (name, atol (value));
				break;
			case settings::ot_float:
				settings::write_float_option (name, atof (value));
				break;
			case settings::ot_str:
				settings::write_str_option (name, value);
				break;
		}

		display (option);
	}

	void exec ()
	{
		const char *arg = console::argument (1);
		if (arg)
		{
			switch (*arg)
			{
				case 'l':
				case 'L':
					list ();
					return;
				case 'g':
				case 'G':
					get ();
					return;
				case 's':
				case 'S':
					set ();
					return;
			}
		}
		CONSOLE_UART::send_string_p (PSTR ("Args: l - list, g - get, s - set"));
	}

}  // namespace opt

namespace info
{

	const char command [] PROGMEM = "info";
	const char help [] PROGMEM = "Firmware version, modules, panels";

	void exec ()
	{
		fprintf_P (&CONSOLE_UART::stream, PSTR ("VERSION: %04u\r\n"), VERSION);
		CONSOLE_UART::send_string_p (PSTR ("MODULES: "));
		for (uint8_t i = 0; i < telemetry::modules::count; i ++)
		{
			CONSOLE_UART::send_string_p (telemetry::modules::name_p (i));
			CONSOLE_UART::send (' ');
		}
		console::eol ();
		CONSOLE_UART::send_string_p (PSTR ("PANELS:\r\n"));
		for (uint8_t i = 0; i < osd::panel::count; i ++)
		{
			fprintf_P (&CONSOLE_UART::stream, PSTR ("%03u: "), i);
			CONSOLE_UART::send_string_p (osd::panel::name_p (i));
			console::eol ();
		}
	}

}  // namespace info

namespace help
{

	const char command [] PROGMEM = "help";
	const char help [] PROGMEM = "Commands list";

	void exec ()
	{
		for (uint8_t i = 0; i < commands::count; i ++)
		{
			CONSOLE_UART::send_string_p (get_cmd_p (i));
			CONSOLE_UART::send_string_p (PSTR (" - "));
			CONSOLE_UART::send_string_p (get_help_p (i));
			console::eol ();
		}
	}

}  // namespace help

namespace exit
{

	const char command [] PROGMEM = "exit";
	const char help [] PROGMEM = "Exit console";

	void exec ()
	{
		console::stop ();
	}

}  // namespace exit


namespace reboot
{

	const char command [] PROGMEM = "reboot";
	const char help [] PROGMEM = "Reboot OSD";

	void exec ()
	{
		wdt_enable (WDTO_250MS);
		while (true)
			;
	}

}  // namespace reboot


#define declare_cmd(NS) { NS :: command, NS :: help, NS :: exec }

const command_t values [] PROGMEM = {
	declare_cmd (font),
	declare_cmd (reset),
	declare_cmd (eeprom),
	declare_cmd (opt),
	declare_cmd (info),
	declare_cmd (help),
	declare_cmd (exit),
	declare_cmd (reboot),
};

const uint8_t count = sizeof (values) / sizeof (command_t);

bool exec (const char *cmd, uint8_t size)
{
	for (uint8_t i = 0; i < count; i ++)
		if (!strncasecmp_P (cmd, (const char *) pgm_read_ptr (&values [i].cmd_p), size))
		{
			((command_t::proc_t) pgm_read_ptr (&values [i].exec)) ();
			return true;
		}
	return false;
}

}  // namespace commands


void process (const char *cmd)
{
	const char *command = console::argument (0);
	uint8_t size = 0;
	while (command [size] && command [size] != ' ') size ++;

	if (!commands::exec (command, size))
		CONSOLE_UART::send_string_p (PSTR ("Invalid command"));
}

}
