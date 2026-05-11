#pragma once
#include <stdint.h>
#include <string>
#include <memory>

// Forward declarations for DirEntry friends (platform-specific iterator impls).
// Having an unused forward declaration on the wrong platform is harmless.
class NixDirIterImpl;
class WinDirIterImpl;

// ---------------------------------------------------------------------------
// DirEntry
// ---------------------------------------------------------------------------

class DirEntry {
public:
	enum Type : uint8_t { FILE = 0, DIR = 1, SYMLINK = 2, OTHER = 3 };

	DirEntry(const DirEntry&)            = delete;
	DirEntry& operator=(const DirEntry&) = delete;

	const char* name()  const { return name_.c_str(); }
	Type        type()  const { return type_; }
	uint64_t    size()  const { return size_; }
	uint32_t    ctime() const { return ctime_; }
	uint32_t    mtime() const { return mtime_; }

	std::unique_ptr<DirEntry> copy() const {
		return std::unique_ptr<DirEntry>(new DirEntry(name_, type_, size_, ctime_, mtime_));
	}

private:
	DirEntry() : type_(OTHER), size_(0), ctime_(0), mtime_(0) {}
	DirEntry(const std::string& n, Type t, uint64_t s, uint32_t ct, uint32_t mt)
		: name_(n), type_(t), size_(s), ctime_(ct), mtime_(mt) {}

	std::string name_;
	Type        type_;
	uint64_t    size_;
	uint32_t    ctime_;
	uint32_t    mtime_;

	friend class NixDirIterImpl;  // Linux dir iteration
	friend class WinDirIterImpl;  // Windows dir iteration
	friend class RemoteFs;        // constructs DirEntry for remote entries
};

// ---------------------------------------------------------------------------
// DirIter
// ---------------------------------------------------------------------------

class DirIterImpl {
public:
	virtual ~DirIterImpl() {}
	virtual const DirEntry* next() = 0;
protected:
	DirIterImpl() {}
};

class DirIter {
public:
	// Default-constructed DirIter is invalid; operator bool() returns false.
	DirIter() : impl_(nullptr) {}
	explicit DirIter(std::unique_ptr<DirIterImpl> impl) : impl_(std::move(impl)) {}

	// Returns false when the readdir() call that produced this iter failed.
	// A range-for on an invalid DirIter is a no-op (never enters the loop body).
	explicit operator bool() const { return impl_ != nullptr; }

	// Returns the next entry and advances the iterator.
	// Returns nullptr when all entries are exhausted.
	// The returned pointer is invalidated by the next call to next().
	const DirEntry* next() {
		if (!impl_) return nullptr;
		return impl_->next();
	}

	// Range-for support: for (const DirEntry& e : iter) { ... }
	// begin() advances to the first entry; operator++ advances to subsequent ones.
	struct Iter {
		DirIter*        owner;
		const DirEntry* ptr;
		const DirEntry& operator*()            const { return *ptr; }
		Iter&           operator++()                 { ptr = owner->next(); return *this; }
		bool            operator!=(const Iter& o) const { return ptr != o.ptr; }
	};
	Iter begin() { return {this, next()}; }
	Iter end()   { return {nullptr, nullptr}; }

	DirIter(DirIter&&) noexcept            = default;
	DirIter& operator=(DirIter&&) noexcept = default;
	DirIter(const DirIter&)                = delete;
	DirIter& operator=(const DirIter&)     = delete;

private:
	std::unique_ptr<DirIterImpl> impl_;
};

// ---------------------------------------------------------------------------
// UniFs
// ---------------------------------------------------------------------------

class UniFile;  // defined in unifile.hpp

class UniFs {
public:
	virtual ~UniFs() {}

	// Opens path for iteration; returns an invalid DirIter on failure.
	virtual DirIter                  readdir(const char* path)             = 0;

	// Opens a file; mode follows fopen conventions. Returns nullptr on failure.
	virtual std::unique_ptr<UniFile> openfile(const char* path,
	                                          const char* mode)            = 0;

	// Removes a file at path. Returns 0 on success, -1 on failure.
	virtual int                      removefile(const char* path)          = 0;

	UniFs(const UniFs&)            = delete;
	UniFs& operator=(const UniFs&) = delete;

protected:
	UniFs() {}
};
