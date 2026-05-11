#pragma once
// Protocol definition for the UniFs client-server wire format.
//
// Frame layout (all integers little-endian):
//   [len:u32][req_id:u16][cmd_or_status:u8][payload...]
//
// len     : total frame length including the 7-byte header
// req_id  : assigned by the client; 0x0000 is reserved for HEARTBEAT
// cmd     : command code in a request frame
// status  : 0x00 = success, non-zero = error code in a response frame

#include <stdint.h>
#include <string.h>  // memcpy

// ---------------------------------------------------------------------------
// Endianness
// ---------------------------------------------------------------------------

#if defined(_MSC_VER)
  // MSVC only targets little-endian platforms (x86, x64, ARM).
  #include <stdlib.h>
  #define UNIFS_BSWAP16(x) _byteswap_ushort(x)
  #define UNIFS_BSWAP32(x) _byteswap_ulong(x)
  #define UNIFS_BSWAP64(x) _byteswap_uint64(x)
  #define UNIFS_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  #define UNIFS_BSWAP16(x) __builtin_bswap16(x)
  #define UNIFS_BSWAP32(x) __builtin_bswap32(x)
  #define UNIFS_BSWAP64(x) __builtin_bswap64(x)
  #define UNIFS_LITTLE_ENDIAN 1
#else
  #define UNIFS_BSWAP16(x) __builtin_bswap16(x)
  #define UNIFS_BSWAP32(x) __builtin_bswap32(x)
  #define UNIFS_BSWAP64(x) __builtin_bswap64(x)
#endif

#ifdef UNIFS_LITTLE_ENDIAN
  #define htop16(x) ((uint16_t)(x))
  #define htop32(x) ((uint32_t)(x))
  #define htop64(x) ((uint64_t)(x))
  #define ptoh16(x) ((uint16_t)(x))
  #define ptoh32(x) ((uint32_t)(x))
  #define ptoh64(x) ((uint64_t)(x))
#else
  #define htop16(x) UNIFS_BSWAP16(x)
  #define htop32(x) UNIFS_BSWAP32(x)
  #define htop64(x) UNIFS_BSWAP64(x)
  #define ptoh16(x) UNIFS_BSWAP16(x)
  #define ptoh32(x) UNIFS_BSWAP32(x)
  #define ptoh64(x) UNIFS_BSWAP64(x)
#endif

// ---------------------------------------------------------------------------
// Frame constants
// ---------------------------------------------------------------------------

static const uint32_t FRAME_HEADER_SIZE     = 7;        // len(4)+req_id(2)+cmd(1)
static const uint32_t MAX_PAYLOAD_SIZE      = 1048576;  // 1 MB cap per frame
static const int      HEARTBEAT_INTERVAL_SEC = 60;      // client heartbeat period

// ---------------------------------------------------------------------------
// Command codes  (client -> server)
// ---------------------------------------------------------------------------

// Batch directory read: returns all entries in a single response.
//   request : path_len:u16 + path:N
//   response: entry_count:u32 +
//             [ type:u8 + ctime:u32 + mtime:u32 + size:u64 +
//               name_len:u16 + name:N ] × entry_count
static const uint8_t CMD_FS_READDIR = 0x01;

// Remove a file by path (directories are rejected).
//   request : path_len:u16 + path:N
static const uint8_t CMD_FS_REMOVE  = 0x06;

static const uint8_t CMD_FILE_OPEN  = 0x11;  // flags:u8 + path_len:u16 + path:N
static const uint8_t CMD_FILE_SEEK  = 0x12;  // file_handle:u32 + offset:i64 + whence:u8
static const uint8_t CMD_FILE_TELL  = 0x13;  // file_handle:u32
static const uint8_t CMD_FILE_READ  = 0x14;  // file_handle:u32 + size:u32
static const uint8_t CMD_FILE_WRITE = 0x15;  // file_handle:u32 + data_len:u32 + data:N
static const uint8_t CMD_FILE_CLOSE = 0x16;  // file_handle:u32

static const uint8_t CMD_HEARTBEAT  = 0xFF;  // no payload

// ---------------------------------------------------------------------------
// Status codes  (server -> client, in response frame's cmd/status byte)
// ---------------------------------------------------------------------------

static const uint8_t STATUS_OK            = 0x00;
static const uint8_t STATUS_ERR_GENERIC   = 0x01;
static const uint8_t STATUS_ERR_NOT_FOUND = 0x02;
static const uint8_t STATUS_ERR_PERM      = 0x03;
static const uint8_t STATUS_ERR_BAD_HANDLE= 0x04;
static const uint8_t STATUS_ERR_BAD_ARG   = 0x05;

// ---------------------------------------------------------------------------
// FILE_OPEN flag values
// ---------------------------------------------------------------------------

static const uint8_t FILE_FLAG_READ   = 0x01;  // open for reading
static const uint8_t FILE_FLAG_WRITE  = 0x02;  // open for writing (truncate)
static const uint8_t FILE_FLAG_RDWR   = 0x03;  // open for reading and writing
static const uint8_t FILE_FLAG_APPEND = 0x04;  // open for appending

// ---------------------------------------------------------------------------
// Integer read/write helpers (handle alignment and byte-order in one place)
// ---------------------------------------------------------------------------

inline uint16_t proto_read_u16(const uint8_t* p) {
	uint16_t v; memcpy(&v, p, 2); return ptoh16(v);
}
inline uint32_t proto_read_u32(const uint8_t* p) {
	uint32_t v; memcpy(&v, p, 4); return ptoh32(v);
}
inline uint64_t proto_read_u64(const uint8_t* p) {
	uint64_t v; memcpy(&v, p, 8); return ptoh64(v);
}
inline int64_t proto_read_i64(const uint8_t* p) {
	uint64_t v; memcpy(&v, p, 8); return (int64_t)ptoh64(v);
}

inline void proto_write_u16(uint8_t* p, uint16_t v) {
	v = htop16(v); memcpy(p, &v, 2);
}
inline void proto_write_u32(uint8_t* p, uint32_t v) {
	v = htop32(v); memcpy(p, &v, 4);
}
inline void proto_write_u64(uint8_t* p, uint64_t v) {
	v = htop64(v); memcpy(p, &v, 8);
}
inline void proto_write_i64(uint8_t* p, int64_t v) {
	uint64_t u = htop64((uint64_t)v); memcpy(p, &u, 8);
}
