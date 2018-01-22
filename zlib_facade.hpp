#pragma once

#include <cstdio>
#include <string>

namespace zl {
class zlib_facade {
public:
    zlib_facade(): chunk_sz(16384) {};
    zlib_facade(int chunk_sz): chunk_sz(chunk_sz) {};
    zlib_facade(const zlib_facade&) = default;
    zlib_facade(zlib_facade&&) = default;
    ~zlib_facade() = default;

    zlib_facade& operator=(const zlib_facade&) = default;
    zlib_facade& operator=(zlib_facade&&) = default;

    /**
     * Open the file fname, compress it and save result to fname.zz.
     *
     * @param fname The path to the file to compress.
     * @return zlib return code.
     */
    auto compress(const std::string& fname);

    /**
     * Open the zlib archive file fname, decompress it and save the result.
     *
     * @param fname The path to the file to decompress.
     * @return zlib return code.
     */
    auto decompress(const std::string& fname);

private:
    int deflate_file(FILE *source, FILE *dest, int level);
    int inflate_file(FILE *source, FILE *dest);
    
    int chunk_sz;
};
}
