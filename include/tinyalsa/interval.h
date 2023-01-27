/* interval.h
 *
 * Copyright 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#ifndef TINYALSA_INTERVAL_H
#define TINYALSA_INTERVAL_H

#include <stdlib.h>
#include <unistd.h>

/** A closed range signed interval. */

struct tinyalsa_signed_interval {
	/** The maximum value of the interval */
	ssize_t max;
	/** The minimum value of the interval */
	ssize_t min;
};

/** A closed range unsigned interval. */

struct tinyalsa_unsigned_interval {
	/** The maximum value of the interval */
	size_t max;
	/** The minimum value of the interval */
	size_t min;
};

#endif /* TINYALSA_INTERVAL_H */
