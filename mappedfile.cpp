#include "mappedfile.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>

namespace g3d {

MappedFile::MappedFile(const char *path) : m_data(NULL), m_size(0)
{
    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Unable to open %s -- %m\n", path);
        return;
    }
    struct stat st;
    if(fstat(fd, &st) == -1) {
        fprintf(stderr, "Unable to stat %s -- %m\n", path);
        close(fd);
        return;
    }

    void *p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if(p == MAP_FAILED) {
        fprintf(stderr, "Unable to mmap %s -- %m\n", path);
        close(fd);
        return;
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
