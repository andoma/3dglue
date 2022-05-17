#include <stdint.h>
#include <stddef.h>

namespace g3d {

struct MappedFile {
    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;

    MappedFile(const char *path);
    ~MappedFile();

    const uint8_t *data() const { return m_data; };
    const size_t size() const { return m_size; };

private:
    uint8_t *m_data;
    size_t m_size;
};
}  // namespace g3d
