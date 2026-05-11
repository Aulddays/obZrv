#pragma once
#include "unifs.hpp"
#include <memory>

#ifdef _WIN32
#	include "win_fs.hpp"
	typedef WinFs LocalFs;
#else
#	include "nix_fs.hpp"
	typedef NixFs LocalFs;
#endif
