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

#ifndef MYST3_DDS_DECOMPRESS_H
#define MYST3_DDS_DECOMPRESS_H

#include "graphics/surface.h"

namespace Myst3 {

/**
 * Decompress DXT1 (BC1) compressed texture data to RGBA
 * @param src Compressed data
 * @param srcSize Size of compressed data
 * @param width Image width
 * @param height Image height
 * @return Decompressed surface in RGBA format, caller must free
 */
Graphics::Surface *decompressDXT1(const byte *src, uint srcSize, uint width, uint height);

/**
 * Decompress DXT3 (BC2) compressed texture data to RGBA
 * @param src Compressed data
 * @param srcSize Size of compressed data
 * @param width Image width
 * @param height Image height
 * @return Decompressed surface in RGBA format, caller must free
 */
Graphics::Surface *decompressDXT3(const byte *src, uint srcSize, uint width, uint height);

/**
 * Decompress DXT5 (BC3) compressed texture data to RGBA
 * @param src Compressed data
 * @param srcSize Size of compressed data
 * @param width Image width
 * @param height Image height
 * @return Decompressed surface in RGBA format, caller must free
 */
Graphics::Surface *decompressDXT5(const byte *src, uint srcSize, uint width, uint height);

} // End of namespace Myst3

#endif // MYST3_DDS_DECOMPRESS_H
