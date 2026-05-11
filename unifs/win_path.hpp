#pragma once
#ifdef _WIN32

#include <string>
#include <cctype>

// Convert a Linux-style path to a Windows native path.
//
// Rules:
//   "/"          -> ""            (root - caller handles as drive-list mode)
//   "/c"         -> "C:\"
//   "/c/foo/bar" -> "C:\foo\bar"
//   anything else (no leading '/', or no drive letter) -> returned unchanged
//
// All forward slashes in the rest of the path are replaced with backslashes.
inline std::string linux_path_to_win(const char* path) {
	if (!path || path[0] == '\0') return "";

	// Not a Unix-style absolute path -- pass through unchanged.
	if (path[0] != '/') return path;

	// Root "/" -- signal to caller by returning empty string.
	if (path[1] == '\0') return "";

	// Expect a single drive letter immediately after '/'.
	char drive = path[1];
	if (!isalpha((unsigned char)drive)) return path;  // e.g. "//server/share" - pass through

	// After the drive letter must be '\0' or '/' (not e.g. "/cc/foo").
	if (path[2] != '\0' && path[2] != '/') return path;

	std::string result;
	result += (char)toupper((unsigned char)drive);
	result += ':';

	if (path[2] == '\0') {
		// "/c" -> "C:\"
		result += '\\';
	} else {
		// "/c/foo/bar" -> "C:\foo\bar"
		const char* rest = path + 2;  // starts with '/'
		while (*rest) {
			result += (*rest == '/') ? '\\' : *rest;
			++rest;
		}
	}
	return result;
}

#endif // _WIN32
