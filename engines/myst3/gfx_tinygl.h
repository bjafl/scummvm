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

#ifndef GFX_TINYGL_H_
#define GFX_TINYGL_H_

#include "engines/myst3/rect.h"
#include "common/system.h"

#include "math/vector3d.h"

#include "engines/myst3/gfx.h"

#include "graphics/tinygl/tinygl.h"

namespace Myst3 {

class TinyGLRenderer : public Renderer {
public:
	TinyGLRenderer(OSystem *_system);
	virtual ~TinyGLRenderer();

	void init() override;
	// void setViewport(const Rect &viewport, bool is3d) override;

	void clear() override;
	void selectTargetWindow(Window *window, bool is3D, bool scaled) override;

	Texture *createTexture2D(const Graphics::Surface *surface) override;
	Texture *createTexture3D(const Graphics::Surface *surface) override;

	// void drawRect2D(const Rect &rect, uint8 a, uint8 r, uint8 g, uint8 b) override;
	void drawRect2D(const RectF &screenRect, uint8 a, uint8 r, uint8 g, uint8 b) override;
	
	virtual void drawTexturedRect2D(const RectF &screenRect, const RectF &textureRect,
                                        Texture *texture, float transparency, bool additiveBlending) override;
	virtual void drawTexturedRect3D(const Math::Vector3d &topLeft, const Math::Vector3d &bottomLeft,
	                                const Math::Vector3d &topRight, const Math::Vector3d &bottomRight,
	                                Texture *texture) override;

	void drawCube(Texture **textures) override;
	void draw2DText(const Common::String &text, const PointF &position, bool posIsNormalized = false) override;

	Graphics::Surface *getScreenshot() override;

	void flipBuffer() override;
private:
	void drawFace(uint face, Texture *texture);

	RectF _viewport;
};

} // End of namespace Myst3

#endif // GFX_H_
