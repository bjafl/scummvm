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

#include "engines/myst3/dds_decompress.h"
#include "engines/myst3/gfx.h"

namespace Myst3 {

// Decode a 16-bit 5:6:5 RGB color to 8-bit RGB
static void decodeColor565(uint16 color, byte &r, byte &g, byte &b) {
	r = ((color >> 11) & 0x1F) * 255 / 31;
	g = ((color >> 5) & 0x3F) * 255 / 63;
	b = (color & 0x1F) * 255 / 31;
}

// Decompress a single DXT1 block (4x4 pixels)
static void decompressDXT1Block(const byte *src, byte *dst, uint dstPitch, bool hasAlpha) {
	uint16 color0 = src[0] | (src[1] << 8);
	uint16 color1 = src[2] | (src[3] << 8);

	byte r0, g0, b0, r1, g1, b1;
	decodeColor565(color0, r0, g0, b0);
	decodeColor565(color1, r1, g1, b1);

	// Build color palette
	byte palette[4][4]; // RGBA

	palette[0][0] = r0;
	palette[0][1] = g0;
	palette[0][2] = b0;
	palette[0][3] = 255;

	palette[1][0] = r1;
	palette[1][1] = g1;
	palette[1][2] = b1;
	palette[1][3] = 255;

	if (color0 > color1 || !hasAlpha) {
		// 4-color mode
		palette[2][0] = (2 * r0 + r1) / 3;
		palette[2][1] = (2 * g0 + g1) / 3;
		palette[2][2] = (2 * b0 + b1) / 3;
		palette[2][3] = 255;

		palette[3][0] = (r0 + 2 * r1) / 3;
		palette[3][1] = (g0 + 2 * g1) / 3;
		palette[3][2] = (b0 + 2 * b1) / 3;
		palette[3][3] = 255;
	} else {
		// 3-color + transparent mode
		palette[2][0] = (r0 + r1) / 2;
		palette[2][1] = (g0 + g1) / 2;
		palette[2][2] = (b0 + b1) / 2;
		palette[2][3] = 255;

		palette[3][0] = 0;
		palette[3][1] = 0;
		palette[3][2] = 0;
		palette[3][3] = 0; // Transparent
	}

	// Decode 4x4 block
	uint32 indices = src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24);

	for (int y = 0; y < 4; y++) {
		byte *row = dst + y * dstPitch;
		for (int x = 0; x < 4; x++) {
			int idx = indices & 3;
			indices >>= 2;

			row[x * 4 + 0] = palette[idx][0];
			row[x * 4 + 1] = palette[idx][1];
			row[x * 4 + 2] = palette[idx][2];
			row[x * 4 + 3] = palette[idx][3];
		}
	}
}

// Decompress a single DXT3 block (4x4 pixels) - explicit alpha
static void decompressDXT3Block(const byte *src, byte *dst, uint dstPitch) {
	// First 8 bytes are explicit alpha (4 bits per pixel)
	uint64 alphaBlock = 0;
	for (int i = 0; i < 8; i++) {
		alphaBlock |= (uint64)src[i] << (i * 8);
	}

	// Decompress color (same as DXT1 without alpha)
	decompressDXT1Block(src + 8, dst, dstPitch, false);

	// Apply explicit alpha
	for (int y = 0; y < 4; y++) {
		byte *row = dst + y * dstPitch;
		for (int x = 0; x < 4; x++) {
			int alphaIdx = y * 4 + x;
			byte alpha4 = (alphaBlock >> (alphaIdx * 4)) & 0xF;
			row[x * 4 + 3] = alpha4 * 17; // Scale 0-15 to 0-255
		}
	}
}

// Decompress a single DXT5 block (4x4 pixels) - interpolated alpha
static void decompressDXT5Block(const byte *src, byte *dst, uint dstPitch) {
	// First 2 bytes are alpha endpoints
	byte alpha0 = src[0];
	byte alpha1 = src[1];

	// Build alpha palette
	byte alphaPalette[8];
	alphaPalette[0] = alpha0;
	alphaPalette[1] = alpha1;

	if (alpha0 > alpha1) {
		// 8-alpha mode
		alphaPalette[2] = (6 * alpha0 + 1 * alpha1) / 7;
		alphaPalette[3] = (5 * alpha0 + 2 * alpha1) / 7;
		alphaPalette[4] = (4 * alpha0 + 3 * alpha1) / 7;
		alphaPalette[5] = (3 * alpha0 + 4 * alpha1) / 7;
		alphaPalette[6] = (2 * alpha0 + 5 * alpha1) / 7;
		alphaPalette[7] = (1 * alpha0 + 6 * alpha1) / 7;
	} else {
		// 6-alpha mode
		alphaPalette[2] = (4 * alpha0 + 1 * alpha1) / 5;
		alphaPalette[3] = (3 * alpha0 + 2 * alpha1) / 5;
		alphaPalette[4] = (2 * alpha0 + 3 * alpha1) / 5;
		alphaPalette[5] = (1 * alpha0 + 4 * alpha1) / 5;
		alphaPalette[6] = 0;
		alphaPalette[7] = 255;
	}

	// Next 6 bytes are 3-bit alpha indices (48 bits for 16 pixels)
	uint64 alphaIndices = 0;
	for (int i = 0; i < 6; i++) {
		alphaIndices |= (uint64)src[2 + i] << (i * 8);
	}

	// Decompress color (same as DXT1 without alpha)
	decompressDXT1Block(src + 8, dst, dstPitch, false);

	// Apply interpolated alpha
	for (int y = 0; y < 4; y++) {
		byte *row = dst + y * dstPitch;
		for (int x = 0; x < 4; x++) {
			int alphaIdx = y * 4 + x;
			int idx = (alphaIndices >> (alphaIdx * 3)) & 7;
			row[x * 4 + 3] = alphaPalette[idx];
		}
	}
}

Graphics::Surface *decompressDXT1(const byte *src, uint srcSize, uint width, uint height) {
	Graphics::Surface *surface = new Graphics::Surface();
	surface->create(width, height, Texture::getRGBAPixelFormat());

	uint blocksX = (width + 3) / 4;
	uint blocksY = (height + 3) / 4;
	uint blockSize = 8; // DXT1 block is 8 bytes

	for (uint by = 0; by < blocksY; by++) {
		for (uint bx = 0; bx < blocksX; bx++) {
			uint srcOffset = (by * blocksX + bx) * blockSize;
			if (srcOffset + blockSize > srcSize) break;

			uint dstX = bx * 4;
			uint dstY = by * 4;

			// Decompress to a temporary 4x4 buffer
			byte block[4 * 4 * 4]; // 4x4 RGBA
			decompressDXT1Block(src + srcOffset, block, 4 * 4, true);

			// Copy to surface (handle edge cases)
			for (uint y = 0; y < 4 && dstY + y < height; y++) {
				byte *dstRow = (byte *)surface->getBasePtr(dstX, dstY + y);
				for (uint x = 0; x < 4 && dstX + x < width; x++) {
					dstRow[x * 4 + 0] = block[y * 16 + x * 4 + 0];
					dstRow[x * 4 + 1] = block[y * 16 + x * 4 + 1];
					dstRow[x * 4 + 2] = block[y * 16 + x * 4 + 2];
					dstRow[x * 4 + 3] = block[y * 16 + x * 4 + 3];
				}
			}
		}
	}

	return surface;
}

Graphics::Surface *decompressDXT3(const byte *src, uint srcSize, uint width, uint height) {
	Graphics::Surface *surface = new Graphics::Surface();
	surface->create(width, height, Texture::getRGBAPixelFormat());

	uint blocksX = (width + 3) / 4;
	uint blocksY = (height + 3) / 4;
	uint blockSize = 16; // DXT3 block is 16 bytes

	for (uint by = 0; by < blocksY; by++) {
		for (uint bx = 0; bx < blocksX; bx++) {
			uint srcOffset = (by * blocksX + bx) * blockSize;
			if (srcOffset + blockSize > srcSize) break;

			uint dstX = bx * 4;
			uint dstY = by * 4;

			// Decompress to a temporary 4x4 buffer
			byte block[4 * 4 * 4]; // 4x4 RGBA
			decompressDXT3Block(src + srcOffset, block, 4 * 4);

			// Copy to surface (handle edge cases)
			for (uint y = 0; y < 4 && dstY + y < height; y++) {
				byte *dstRow = (byte *)surface->getBasePtr(dstX, dstY + y);
				for (uint x = 0; x < 4 && dstX + x < width; x++) {
					dstRow[x * 4 + 0] = block[y * 16 + x * 4 + 0];
					dstRow[x * 4 + 1] = block[y * 16 + x * 4 + 1];
					dstRow[x * 4 + 2] = block[y * 16 + x * 4 + 2];
					dstRow[x * 4 + 3] = block[y * 16 + x * 4 + 3];
				}
			}
		}
	}

	return surface;
}

Graphics::Surface *decompressDXT5(const byte *src, uint srcSize, uint width, uint height) {
	Graphics::Surface *surface = new Graphics::Surface();
	surface->create(width, height, Texture::getRGBAPixelFormat());

	uint blocksX = (width + 3) / 4;
	uint blocksY = (height + 3) / 4;
	uint blockSize = 16; // DXT5 block is 16 bytes

	for (uint by = 0; by < blocksY; by++) {
		for (uint bx = 0; bx < blocksX; bx++) {
			uint srcOffset = (by * blocksX + bx) * blockSize;
			if (srcOffset + blockSize > srcSize) break;

			uint dstX = bx * 4;
			uint dstY = by * 4;

			// Decompress to a temporary 4x4 buffer
			byte block[4 * 4 * 4]; // 4x4 RGBA
			decompressDXT5Block(src + srcOffset, block, 4 * 4);

			// Copy to surface (handle edge cases)
			for (uint y = 0; y < 4 && dstY + y < height; y++) {
				byte *dstRow = (byte *)surface->getBasePtr(dstX, dstY + y);
				for (uint x = 0; x < 4 && dstX + x < width; x++) {
					dstRow[x * 4 + 0] = block[y * 16 + x * 4 + 0];
					dstRow[x * 4 + 1] = block[y * 16 + x * 4 + 1];
					dstRow[x * 4 + 2] = block[y * 16 + x * 4 + 2];
					dstRow[x * 4 + 3] = block[y * 16 + x * 4 + 3];
				}
			}
		}
	}

	return surface;
}

} // End of namespace Myst3
