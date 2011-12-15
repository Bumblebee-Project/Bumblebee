/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 *
 * This file is part of Bumblebee.
 *
 * Bumblebee is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bumblebee is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bumblebee. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * bbswitch-related functions for Bumblebee
 */
#pragma once

/// Returns 0 if card is off, 1 if card is on, -1 if bbswitch not active.
/// In other words: 0 means off, anything else means on.
int bbswitch_status();

/// Turns card on if not already on.
void bbswitch_on();

/// Turns card off if not already on.
void bbswitch_off();
