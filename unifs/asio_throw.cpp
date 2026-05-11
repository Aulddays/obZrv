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

#include <system_error>  // std::system_error

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
template void throw_exception<std::system_error>(
	const std::system_error &);

} // namespace detail
} // namespace asio
