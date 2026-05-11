#include "local_file.hpp"
#include <stdio.h>

// Large-file seek/tell: MSVC uses _fseeki64/_ftelli64; others use fseeko/ftello.
#ifdef _MSC_VER
  #define fs_fseek(fp, off, whence) _fseeki64((fp), (off), (whence))
  #define fs_ftell(fp)              _ftelli64(fp)
#else
  #define fs_fseek(fp, off, whence) fseeko((fp), (off64_t)(off), (whence))
  #define fs_ftell(fp)              ftello(fp)
#endif

int LocalFile::seek(int64_t offset, int whence) {
	if (!fp_) return -1;
	return (fs_fseek(fp_, offset, whence) == 0) ? 0 : -1;
}

int64_t LocalFile::tell() {
	if (!fp_) return -1;
	return (int64_t)fs_ftell(fp_);
}

size_t LocalFile::read(void* buf, size_t size) {
	if (!fp_) return 0;
	return fread(buf, 1, size, fp_);
}

size_t LocalFile::write(const void* buf, size_t size) {
	if (!fp_) return 0;
	return fwrite(buf, 1, size, fp_);
}

int LocalFile::close() {
	if (fp_) {
		int ret = fclose(fp_);
		fp_ = nullptr;
		return (ret == 0) ? 0 : -1;
	}
	return 0;
}
