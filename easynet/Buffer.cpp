// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "Buffer.h"

using namespace easynet;

const size_t Buffer::kBufferSizeDefault;

Buffer& Buffer::operator=(const Buffer &rhs)
{
	if (this != &rhs)
	{	
		bufSize_  = rhs.bufSize_;
		misalign_ = 0;
		off_ = rhs.off_;
		buf_.reset(new char[bufSize_]);

		std::memcpy(data(), rhs.data(), off_);
	}

	return *this;
}

Buffer& Buffer::operator=(Buffer &&rhs) noexcept
{
	if (this != &rhs)
	{	
		bufSize_  = 0;
		misalign_ = 0;
		off_ = 0;
		buf_.reset();

		swap(rhs);
	}

	return *this;
}

// append @len bytes @data to @buf_
void Buffer::append(const char *data, size_t len)
{
	expand(len);
	std::memcpy(end(), data, len);
	off_ += len;
}

// to ensure that there are at least @len bytes available space at the ending of @buf_
void Buffer::expand(size_t len)
{
	size_t remain = bufSize_ - misalign_ - off_;
	// available space is enough
	if (remain >= len)
	{
		 return;
	}

	// if available space will be enough after align @buf_
	remain = bufSize_ - off_;
	if (remain >= len && size() < bufSize_ / 2)
	{
		align();
		return;
	}

	// the space is not enough
	size_t toAlloc = bufSize_ << 1;
	while (toAlloc < size() + len)
	{
		toAlloc <<= 1;
	}
	std::unique_ptr<char[]> buf(new char[toAlloc]);
	std::memcpy(buf.get(), data(), off_);
	
	buf_.swap(buf);
	misalign_ = 0;
	bufSize_  = toAlloc;
}

// realigns the in _buf so that _misalign is 0
void Buffer::align()
{
	std::memmove(buf_.get(), data(), off_);
	misalign_ = 0;
}

void Buffer::take(char *buf, size_t len)
{
	if (len == 0)
	{
		return;
	}

	if (len > off_)
	{
		len = off_;
	}

	std::memcpy(buf, data(), len);
	deleteBegin(len);
}

void Buffer::takeAll(char *buf)
{
	take(buf, off_);
}

void Buffer::deleteBegin(size_t len)
{
	if (len == 0)
	{
		return;
	}

	if (len > off_)
	{
		len = off_;
	}

	misalign_ += len;
	off_ -= len;
	if (off_ == 0)
	{
		misalign_ = 0;
	}
}

void Buffer::clear()
{
	misalign_ = 0;
	off_ = 0;
}
