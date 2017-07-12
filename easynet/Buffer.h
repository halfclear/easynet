// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_BUFFER_H_
#define _EASYNET_BUFFER_H_

#include <cstring>
#include <list>
#include <memory>

/// Buffer:
/// +-------------------+------------------+------------------+
/// |     free space    |      CONTENT     |   free space     |
/// |<--- @misalign_ -->|<----- @off_ ---->|                  |
/// +-------------------+------------------+------------------+
/// 0                                                      @bufSize_

namespace easynet
{

class Buffer
{
public:
	static const size_t kBufferSizeDefault = 512;
	
	Buffer()
		: bufSize_(kBufferSizeDefault),
	      misalign_(0),
	      off_(0),
	      buf_(new char[bufSize_])
	{}

	Buffer(const Buffer &rhs)
		: bufSize_(rhs.bufSize_),
		  misalign_(0),
		  off_(rhs.off_),
		  buf_(new char[bufSize_])
	{
		std::memcpy(data(), rhs.data(), off_);
	}

	Buffer(Buffer &&rhs) noexcept
		: bufSize_(0),
		  misalign_(0),
		  off_(0)
	{
		swap(rhs);
	}

	~Buffer() = default;

	Buffer& operator=(const Buffer &rhs);
	Buffer& operator=(Buffer &&rhs) noexcept;

	void swap(Buffer &rhs)
	{
		std::swap(bufSize_, rhs.bufSize_);
		std::swap(misalign_, rhs.misalign_);
		std::swap(off_, rhs.off_);
		buf_.swap(rhs.buf_);
	}

	void append(const char *data, size_t len);
	void take(char *buf, size_t len);
	void takeAll(char *buf);

	// return the begining of data stored in the Buffer
	char* data() const  { return buf_.get() + misalign_; }
	char* begin() const { return data(); }
	char* end() const   { return data() + off_; }

	// return the size of data stored in the Buffer
	size_t size() const { return off_; }
	size_t capacity() const { return bufSize_; }
	bool empty() const { return off_ == 0; }

    // to ensure there are at least @len bytes space available in the buffer
    void expand(size_t len);
	void deleteBegin(size_t len);
	void addSize(size_t len) { off_ += len; }
	void clear();

private:
	void align();

	size_t bufSize_;     // the size of @buf_
	size_t misalign_;    // unused space at the begining of @buf_
	size_t off_;         // the total number of bytes actually stored in @buf_
	std::unique_ptr<char[]> buf_;
};

}

#endif
