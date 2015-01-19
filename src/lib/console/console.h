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
#ifndef LIB_CONSOLE_CONSOLE_H_
#define LIB_CONSOLE_CONSOLE_H_

#include <stddef.h>
#include <stdint.h>

namespace console
{

typedef void (* callback_t) (const char *command);

extern callback_t handler;

void eol ();
void show_prompt ();
const char *str_argument (uint8_t position, const char *def = NULL);
int32_t int_argument (uint8_t position);

extern bool running;

void run (callback_t h);
inline void stop ()
{
	running = false;
}

}

#endif /* LIB_CONSOLE_CONSOLE_H_ */
