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

#ifndef GFX_H_
#define GFX_H_

#include "engines/myst3/dds.h"
#include "engines/myst3/resource_loader.h"
#include "engines/myst3/rect.h"

//#include "engines/myst3/rect.h"
#include "common/system.h"

#include "math/frustum.h"
#include "math/matrix4.h"
#include "math/vector3d.h"
#include "math/vector2d.h"

namespace Myst3 {

class Effect;
class GameState;
class Renderer;

class Drawable {
public:
	Drawable();
	virtual ~Drawable() {}

	virtual void draw() {}
	virtual void drawOverlay() {}

	/** Should the drawable be drawn inside the active window, or is it allowed to draw on the entire screen? */
	bool isConstrainedToWindow() const { return _isConstrainedToWindow; }

	/** Whether to setup the renderer state for 2D or 3D when processing the drawable */
	bool is3D() const { return _is3D; }

protected:
	bool _isConstrainedToWindow;
	bool _is3D;
};

/**
 * Game screen window
 *
 * A window represents a game screen pane.
 * It allows abstracting the rendering position from the behavior.
 */
class Window : public Drawable {
public:
	/**
	 * Get the window position in screen coordinates
	 */
	virtual Rect getPosition() const = 0;

	/**
	 * Get the window position in original (640x480) screen coordinates
	 */
	virtual Rect getOriginalPosition() const = 0;

	/**
	 * Get the window center in screen coordinates
	 */
	Point getCenter() const;

	/**
	 * Convert screen coordinates to window coordinates
	 */
	Point screenPosToWindowPos(const Point &screen, bool clip = false) const;

	/**
	 * Transform a point from screen coordinates to scaled window coordinates
	 */
	virtual Point scalePoint(const Point &screen) const;

};

class Texture {
public:
	virtual ~Texture() {}

	uint width;
	uint height;
	Graphics::PixelFormat format;

	Rect size() const { return Rect(width, height); }

	virtual void update(const Graphics::Surface *surface) = 0;
	virtual void updatePartial(const Graphics::Surface *surface, const Rect &rect) = 0;

	static const Graphics::PixelFormat getRGBAPixelFormat();
};

class Renderer {
public:
	Renderer(OSystem *system);
	virtual ~Renderer();

	virtual void init() = 0;
	// virtual void setViewport(const Rect &viewport, bool is3d) = 0;
	virtual void clear() = 0;
	void toggleFullscreen();

	/**
	 *  Swap the buffers, making the drawn screen visible
	 */
	virtual void flipBuffer() { }

	// virtual void initFont(const Graphics::Surface *surface);
	virtual void initFont(ResourceLoader *resourceLoader);
	virtual void freeFont();

	virtual Texture *createTexture3D(const Graphics::Surface *surface) = 0;
	virtual Texture *createTexture2D(const Graphics::Surface *surface) { return createTexture3D(surface); }

	/**
	 * Create a texture from a DDS file, using GPU compression if supported
	 * @param dds The loaded DDS texture
	 * @return A texture, or nullptr if the format is not supported
	 */
	virtual Texture *createTextureFromDDS(const DDS &dds);

	/**
	 * Check if the renderer supports GPU-native compressed textures (S3TC/DXT)
	 */
	virtual bool supportsCompressedTextures() const { return false; }

	virtual void drawRect2D(const Rect &screenRect, uint8 a, uint8 r, uint8 g, uint8 b) = 0;

	virtual void drawTexturedRect2D(const Rect &screenRect, const Rect &textureRect, Texture *texture,
									float transparency = -1.0, bool additiveBlending = false) = 0;

	virtual void drawTexturedRect3D(const Math::Vector3d &topLeft, const Math::Vector3d &bottomLeft,
									const Math::Vector3d &topRight, const Math::Vector3d &bottomRight,
									Texture *texture) = 0;

	virtual void drawCube(Texture **textures) = 0;
	virtual void drawCubeWithEffects(Texture **textures, Texture **effectMasks, Texture *shieldPattern,
	                                 const Common::Array<Effect *> &effects, GameState *state) {}
	virtual void draw2DText(const Common::String &text, const Point &position) = 0;

	/** Check if GPU-based effects are supported */
	virtual bool supportsShaderEffects() const { return false; }

	virtual Graphics::Surface *getScreenshot() = 0;
	virtual Texture *copyScreenshotToTexture();

	/** Render a Drawable in the specified window */
	void renderDrawable(Drawable *drawable, Window *window);

	/** Render a Drawable overlay in the specified window */
	void renderDrawableOverlay(Drawable *drawable, Window *window);

	/** Render the main Drawable of a Window */
	void renderWindow(Window *window);

	/** Render the main Drawable overlay of a Window */
	void renderWindowOverlay(Window *window);

	Rect viewport() const;
	Rect frameViewport() const;
	Rect origAspectRatioViewport() const;
	Rect topBorder() const;
	Rect bottomBorder() const;

	/**
	 * Select the window where to render
	 *
	 * This also sets the viewport. When a window is provided, the ortho projection
	 * uses the window's original coordinates. When no window is provided, scaled
	 * determines whether to use original (640x480) or screen coordinates.
	 */
	virtual void selectTargetWindow(Window *window, bool is3D, bool scaled = true) = 0;

	void setupCameraPerspective(float pitch, float heading, float fov);

	bool isCubeFaceVisible(uint face);

	Math::Matrix4 getMvpMatrix() const { return _mvpMatrix; }

	void flipVertical(Graphics::Surface *s);

	static const int kOriginalWidth = 640;
	static const int kOriginalHeight = 480;
	static const int kTopBorderHeight = 30;
	static const int kBottomBorderHeight = 90;
	static const int kFrameHeight = 360;

	void computeScreenViewport();

	PointF getScale() const;
	Rect scaleRect(const Rect &rect);
	Rect createScaledRect(int16 w, int16 h, bool centerOnViewport = false, bool useFrameViewport = false);
	Rect centerOnViewport(const Rect &rect, bool useFrameViewport = false);

protected:
	OSystem *_system;
	Texture *_font;

	Rect _screenViewport;

	Math::Matrix4 _projectionMatrix;
	Math::Matrix4 _modelViewMatrix;
	Math::Matrix4 _mvpMatrix;

	Math::Frustum _frustum;

	static const float cubeVertices[5 * 6 * 4];
	Math::AABB _cubeFacesAABB[6];

	Rect getFontCharacterRect(uint8 character);

	Math::Matrix4 makeProjectionMatrix(float fov) const;

};

Renderer *CreateGfxOpenGL(OSystem *system);
Renderer *CreateGfxOpenGLShader(OSystem *system);
Renderer *CreateGfxTinyGL(OSystem *system);
Renderer *createRenderer(OSystem *system);

} // End of namespace Myst3

#endif // GFX_H_
