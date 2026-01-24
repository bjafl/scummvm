/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Based on lzokay library by Jack Andersen (MIT License)

#ifndef MYST3_LZO_H
#define MYST3_LZO_H

#include "common/scummsys.h"

namespace Myst3 {

enum LzoResult {
	kLzoLookbehindOverrun = -4,
	kLzoOutputOverrun = -3,
	kLzoInputOverrun = -2,
	kLzoError = -1,
	kLzoSuccess = 0,
	kLzoInputNotConsumed = 1
};

/**
 * Decompress LZO1X compressed data
 * @param src Source compressed data
 * @param srcSize Size of source data
 * @param dst Destination buffer
 * @param dstSize On input: size of destination buffer. On output: actual decompressed size
 * @return LzoResult indicating success or failure
 */
LzoResult lzoDecompress(const uint8 *src, size_t srcSize,
                        uint8 *dst, size_t dstSize,
                        size_t &outSize);

/**
 * Compress data using LZO1X
 * @param src Source data
 * @param srcSize Size of source data
 * @param dst Destination buffer
 * @param dstSize On input: size of destination buffer. On output: actual compressed size
 * @return LzoResult indicating success or failure
 */
LzoResult lzoCompress(const uint8 *src, size_t srcSize,
                      uint8 *dst, size_t dstSize,
                      size_t &outSize);

/**
 * Calculate worst-case compressed size for a given input size
 */
inline size_t lzoCompressWorstSize(size_t s) {
	return s + s / 16 + 64 + 3;
}

} // namespace Myst3

#endif // MYST3_LZO_H
