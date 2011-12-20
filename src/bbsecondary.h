/// \file bbsecondary.h Contains definitions for enabling and disabling the secondary GPU.

/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
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
#pragma once

/// Start the X server by fork-exec, turn card on if needed.
void start_secondary(void);

/// Kill the second X server if any, turn card off if requested.
void stop_secondary(void);

/// Checks what methods are available and what drivers are installed.
void check_secondary(void);

/// Returns 0 if card is off, 1 if card is on, -1 if not-switchable.
int status_secondary(void);
