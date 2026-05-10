// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020-2026 Aulddays (https://dev.aulddays.com/). All rights reserved.
//
// This file is part of obZrv.
// 
// obZrv is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// obZrv is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with obZrv. If not, see <https://www.gnu.org/licenses/>.

#include "pch.h"
// Stub for ASIO_NO_EXCEPTIONS: asio replaces throw with a call to this
// function.  We simply abort -- these code paths are only reached for
// logic errors (bad_executor, service_already_exists) that should never
// occur in normal operation.
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef ASIO_NO_EXCEPTIONS
#define ASIO_NO_EXCEPTIONS
#endif
#include <asio/asio.hpp>
#include <cstdlib>
#include <new>       // std::bad_alloc

namespace asio {
namespace detail {

template <typename Exception>
void throw_exception(const Exception &)
{
	abort();
}

// Explicit instantiations for the exception types asio uses.
template void throw_exception<asio::execution::bad_executor>(
	const asio::execution::bad_executor &);
template void throw_exception<asio::invalid_service_owner>(
	const asio::invalid_service_owner &);
template void throw_exception<asio::service_already_exists>(
	const asio::service_already_exists &);
template void throw_exception<std::out_of_range>(
	const std::out_of_range &);
template void throw_exception<std::bad_alloc>(
	const std::bad_alloc &);

} // namespace detail
} // namespace asio
