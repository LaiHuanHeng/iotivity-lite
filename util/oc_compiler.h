/****************************************************************************
 *
 * Copyright 2021 Daniel Adam All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#ifndef OC_COMPILER_H
#define OC_COMPILER_H

#if defined(__MINGW32__) && (!defined(__GNUC__) || __GNUC__ < 9)
#error "Unsupported compiler on MinGW platform"
#endif /* __MINGW32__ && (!__GNUC__ || __GNUC__ < 9) */

#if defined(__clang__) || defined(__GNUC__)
#define OC_NO_DISCARD_RETURN __attribute__((warn_unused_result))
#else
#define OC_NO_DISCARD_RETURN
#endif

#if defined(__clang__) || defined(__GNUC__)
#define OC_NO_RETURN __attribute__((noreturn))
#else
#define OC_NO_RETURN
#endif

#if defined(__clang__) || defined(__GNUC__)
#define OC_FALLTHROUGH __attribute__((fallthrough))
#else
#define OC_FALLTHROUGH
#endif

#endif // OC_COMPILER_H
