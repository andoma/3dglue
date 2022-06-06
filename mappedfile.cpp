#include "mappedfile.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>

#include <system_error>

namespace g3d {

MappedFile::MappedFile(const char *path) : m_data(NULL), m_size(0)
{
    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
    struct stat st;
    if(fstat(fd, &st) == -1) {
        int e = errno;
        close(fd);
        throw std::system_error(e, std::system_category());
    }

    void *p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    int e = errno;
    close(fd);

    if(p == MAP_FAILED) {
        throw std::system_error(e, std::system_category());
    }

    m_size = st.st_size;
    m_data = (uint8_t *)p;
}

MappedFile::~MappedFile()
{
    if(m_size)
        munmap(m_data, m_size);
}
}  // namespace g3d
