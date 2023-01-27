/* version.h
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

#ifndef TINYALSA_VERSION_H
#define TINYALSA_VERSION_H

/* Macros for expanding the version numbers into string literals */
#define TINYALSA_VERSION_STR_EX(number) #number
#define TINYALSA_VERSION_STR(number) TINYALSA_VERSION_STR_EX (number)

#define TINYALSA_VERSION_MAJOR 2

#define TINYALSA_VERSION_MINOR 0

#define TINYALSA_VERSION_PATCH 0

/* The final version number is constructed based on minor, major and patch */
#define TINYALSA_VERSION \
    ((unsigned long) \
    ((TINYALSA_VERSION_MAJOR << 16)   | \
     (TINYALSA_VERSION_MINOR << 8 )   | \
     (TINYALSA_VERSION_PATCH      )))

/* The version string is constructed by concatenating individual ver. strings */
#define TINYALSA_VERSION_STRING \
    TINYALSA_VERSION_STR (TINYALSA_VERSION_MAJOR) \
    "." \
    TINYALSA_VERSION_STR (TINYALSA_VERSION_MINOR) \
    "." \
    TINYALSA_VERSION_STR (TINYALSA_VERSION_PATCH)

#endif /* TINYALSA_VERSION_H */
