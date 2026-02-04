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

#ifndef GFX_OPENGL_TEXTURE_H
#define GFX_OPENGL_TEXTURE_H

#include "graphics/surface.h"
#include "graphics/opengl/system_headers.h"
#include "common/textconsole.h"

#include "engines/myst3/gfx.h"

namespace Myst3 {

class OpenGLTexture : public Texture {
public:
	OpenGLTexture(const Graphics::Surface *surface);
	OpenGLTexture();
	virtual ~OpenGLTexture();

	/**
	 * Create a texture from compressed data (S3TC/DXT)
	 * @param w Width of the texture
	 * @param h Height of the texture
	 * @param internalFmt OpenGL internal format (e.g., GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
	 * @param data Pointer to the compressed data
	 * @param dataSize Size of the compressed data in bytes
	 */
	OpenGLTexture(float w, float h, GLuint internalFmt, const byte *data, uint dataSize);

	void update(const Graphics::Surface *surface) override;
	void updatePartial(const Graphics::Surface *surface, const RectF &rect) override;

	void copyFromFramebuffer(const RectF &screen);

	GLuint id;
	GLuint internalFormat;
	GLuint sourceFormat;
	uint32 internalWidth;
	uint32 internalHeight;
	bool upsideDown;
	bool isCompressed;

private:
	void updateTexture(const Graphics::Surface *surface, const RectF &rect);
};

} // End of namespace Myst3

#endif
