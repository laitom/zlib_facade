#include <cassert>
#include <memory>

#include "zlib.h"
#include "zlib_facade.hpp"

auto zl::zlib_facade::compress(const std::string& fname) {
    auto temp = fname;
    temp.append(".zz");

    std::unique_ptr<FILE, decltype(&fclose)> ifile(fopen(fname.c_str(), "rb"), &fclose);
    std::unique_ptr<FILE, decltype(&fclose)> ofile(fopen(temp.c_str(), "wb"), &fclose);

    return deflate_file(ifile.get(), ofile.get(), Z_DEFAULT_COMPRESSION);
}

auto zl::zlib_facade::decompress(const std::string& fname) {
    auto temp = fname;
    temp.erase(temp.length()-3);

    std::unique_ptr<FILE, decltype(&fclose)> ifile(fopen(fname.c_str(), "rb"), &fclose);
    std::unique_ptr<FILE, decltype(&fclose)> ofile(fopen(temp.c_str(), "wb"), &fclose);

    return inflate_file(ifile.get(), ofile.get());
}

int zl::zlib_facade::deflate_file(FILE *source, FILE *dest, int level) {
    int flush;
    unsigned have;
    z_stream strm;
    unsigned char in[this->chunk_sz];
    unsigned char out[this->chunk_sz];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    auto ret = deflateInit(&strm, level);
    if (ret != Z_OK)
	return ret;

    do {
	strm.avail_in = fread(in, 1, this->chunk_sz, source);

	if (ferror(source)) {
	    static_cast<void>(deflateEnd(&strm));
	    return Z_ERRNO;
	}

	flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
	strm.next_in = in;

	do {
	    strm.avail_out = this->chunk_sz;
	    strm.next_out = out;

	    ret = deflate(&strm, flush);
	    assert(ret != Z_STREAM_ERROR);

	    have = this->chunk_sz - strm.avail_out;
	    if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
		static_cast<void>(deflateEnd(&strm));
		return Z_ERRNO;
	    }
	} while (strm.avail_out == 0);
	
	assert(strm.avail_in == 0);
    } while (flush != Z_FINISH);

    assert(ret == Z_STREAM_END);
    static_cast<void>(deflateEnd(&strm));

    return Z_OK;
}

int zl::zlib_facade::inflate_file(FILE *source, FILE *dest) {
    unsigned have;
    z_stream strm;
    unsigned char in[this->chunk_sz];
    unsigned char out[this->chunk_sz];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    auto ret = inflateInit(&strm);
    if (ret != Z_OK)
	return ret;

    do {
	strm.avail_in = fread(in, 1, this->chunk_sz, source);

	if (ferror(source)) {
	    static_cast<void>(inflateEnd(&strm));
	    return Z_ERRNO;
	}

	if (strm.avail_in == 0)
	    break;
	strm.next_in = in;

	do {
	    strm.avail_out = this->chunk_sz;
	    strm.next_out = out;

	    ret = inflate(&strm, Z_NO_FLUSH);
	    assert(ret != Z_STREAM_ERROR);

	    switch (ret) {
	    case Z_NEED_DICT:
		ret = Z_DATA_ERROR;
	    case Z_DATA_ERROR:
	    case Z_MEM_ERROR:
		static_cast<void>(inflateEnd(&strm));
		return ret;
	    }

	    have = this->chunk_sz - strm.avail_out;
	    if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
		static_cast<void>(inflateEnd(&strm));
		return Z_ERRNO;
	    }
	} while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    static_cast<void>(inflateEnd(&strm));
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
