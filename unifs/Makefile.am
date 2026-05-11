noinst_LIBRARIES = libunifs.a

libunifs_a_SOURCES = \
    local_fs.cpp    \
    nix_fs.cpp      \
    win_fs.cpp      \
    local_file.cpp  \
    remote_conn.cpp \
    remote_fs.cpp   \
    remote_file.cpp \
    asio_throw.cpp

libunifs_a_CXXFLAGS = $(AM_CXXFLAGS) \
    -std=c++11 \
    -I$(top_srcdir) \
    -DASIO_STANDALONE \
    -DASIO_NO_EXCEPTIONS
