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

#include "engines/myst3/gfx.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/rect.h"
#include "engines/myst3/resource_loader.h"

#include "engines/util.h"

#include "common/config-manager.h"

#include "graphics/renderer.h"
#include "graphics/surface.h"

#if defined(USE_OPENGL_GAME) || defined(USE_OPENGL_SHADERS)
#include "graphics/opengl/context.h"
#endif

#include "math/glmath.h"

namespace Myst3 {

const float Renderer::cubeVertices[] = {
	// S     T      X      Y      Z
	0.0f, 1.0f, -320.0f, -320.0f, -320.0f,
	1.0f, 1.0f, 320.0f, -320.0f, -320.0f,
	0.0f, 0.0f, -320.0f, 320.0f, -320.0f,
	1.0f, 0.0f, 320.0f, 320.0f, -320.0f,
	0.0f, 1.0f, 320.0f, -320.0f, -320.0f,
	1.0f, 1.0f, -320.0f, -320.0f, -320.0f,
	0.0f, 0.0f, 320.0f, -320.0f, 320.0f,
	1.0f, 0.0f, -320.0f, -320.0f, 320.0f,
	0.0f, 1.0f, 320.0f, -320.0f, 320.0f,
	1.0f, 1.0f, -320.0f, -320.0f, 320.0f,
	0.0f, 0.0f, 320.0f, 320.0f, 320.0f,
	1.0f, 0.0f, -320.0f, 320.0f, 320.0f,
	0.0f, 1.0f, 320.0f, -320.0f, -320.0f,
	1.0f, 1.0f, 320.0f, -320.0f, 320.0f,
	0.0f, 0.0f, 320.0f, 320.0f, -320.0f,
	1.0f, 0.0f, 320.0f, 320.0f, 320.0f,
	0.0f, 1.0f, -320.0f, -320.0f, 320.0f,
	1.0f, 1.0f, -320.0f, -320.0f, -320.0f,
	0.0f, 0.0f, -320.0f, 320.0f, 320.0f,
	1.0f, 0.0f, -320.0f, 320.0f, -320.0f,
	0.0f, 1.0f, 320.0f, 320.0f, 320.0f,
	1.0f, 1.0f, -320.0f, 320.0f, 320.0f,
	0.0f, 0.0f, 320.0f, 320.0f, -320.0f,
	1.0f, 0.0f, -320.0f, 320.0f, -320.0f};

Renderer::Renderer(OSystem *system)
	: _system(system),
	  _font(nullptr) {
	// Compute the cube faces Axis Aligned Bounding Boxes
	for (uint i = 0; i < ARRAYSIZE(_cubeFacesAABB); i++) {
		for (uint j = 0; j < 4; j++) {
			_cubeFacesAABB[i].expand(Math::Vector3d(cubeVertices[5 * (4 * i + j) + 2], cubeVertices[5 * (4 * i + j) + 3], cubeVertices[5 * (4 * i + j) + 4]));
		}
	}
}

Renderer::~Renderer() {
}

void Renderer::toggleFullscreen() {
	bool oldFullscreen = _system->getFeatureState(OSystem::kFeatureFullscreenMode);
	_system->setFeatureState(OSystem::kFeatureFullscreenMode, !oldFullscreen);
}
// void Renderer::initFont(const Graphics::Surface *surface) {
// 	_font = createTexture2D(surface);
// }
void Renderer::initFont(ResourceLoader *resourceLoader) {

	ResourceDescription fontDesc = resourceLoader->getRawData("GLOB", 1206);
	if (!fontDesc.isValid())
		error("The font texture, GLOB-1206 was not found");

	TextureLoader textureLoader(*this);
	_font = textureLoader.load(fontDesc, TextureLoader::kImageFormatTEX);
}

void Renderer::freeFont() {
	if (_font) {
		delete _font;
		_font = nullptr;
	}
}

Texture *Renderer::createTextureFromDDS(const DDS &dds) {
	// Default implementation: no GPU compression support
	// Subclasses can override to provide hardware decompression
	return nullptr;
}

Texture *Renderer::copyScreenshotToTexture() {
	Graphics::Surface *surface = getScreenshot();

	Texture *texture = createTexture2D(surface);

	surface->free();
	delete surface;

	return texture;
}

Rect Renderer::getFontCharacterRect(uint8 character) {
	uint index = 0;

	if (character == ' ')
		index = 0;
	else if (character >= '0' && character <= '9')
		index = 1 + character - '0';
	else if (character >= 'A' && character <= 'Z')
		index = 1 + 10 + character - 'A';
	else if (character == '|')
		index = 1 + 10 + 26;
	else if (character == '/')
		index = 2 + 10 + 26;
	else if (character == ':')
		index = 3 + 10 + 26;

	return Rect(16 * index, 0, 16 * (index + 1), 32);
}

Rect Renderer::viewport() const {
	return _screenViewport;
}

Rect Renderer::topBorder() const {
	PointF scale = getScale();
	return Rect(_screenViewport.width(), kTopBorderHeight * scale.y);
}
Rect Renderer::bottomBorder() const {
	PointF scale = getScale();
	return Rect(_screenViewport.width(), kBottomBorderHeight * scale.y);
}

Rect Renderer::frameViewport() const {
	PointF scale = getScale();
	int topBorderHeight = kTopBorderHeight * scale.y;
	int bottomBorderHeight = kBottomBorderHeight * scale.y;
	Rect frame(_screenViewport.width(), _screenViewport.height() - topBorderHeight - bottomBorderHeight);
	frame.translate(0, topBorderHeight);
	return frame;
}

Rect Renderer::origAspectRatioViewport() const {
	// int32 screenWidth = _system->getWidth();
	// int32 screenHeight = _system->getHeight();
	// // Aspect ratio correction
	// int32 viewportWidth = MIN<int32>(screenWidth, screenHeight * kOriginalWidth / kOriginalHeight);
	// int32 viewportHeight = MIN<int32>(screenHeight, screenWidth * kOriginalHeight / kOriginalWidth);
	// Rect frame(viewportWidth, viewportHeight);

	// // Pillarboxing
	// frame.translate((screenWidth - viewportWidth) / 2,
	// 				(screenHeight - viewportHeight) / 2);
	// return frame;
	Rect screen(_system->getWidth(), _system->getHeight());
	Rect origFrame(kOriginalWidth, kOriginalHeight);
	Rect scaledFrame = origFrame.fitInside(screen);
	debugC(kDebugGraphics, "OrigAspectRatioViewport - screen (%dx%d), orig (%dx%d), scaled (%dx%d)", screen.width(), screen.height(), origFrame.width(), origFrame.height(), scaledFrame.width(), scaledFrame.height());
	return scaledFrame;
}

void Renderer::computeScreenViewport() {
	if (ConfMan.getBool("widescreen_mod")) {
		int32 screenWidth = _system->getWidth();
		int32 screenHeight = _system->getHeight();
		_screenViewport = Rect(screenWidth, screenHeight);
	} else {
		_screenViewport = origAspectRatioViewport();
	}
}

Math::Matrix4 Renderer::makeProjectionMatrix(float fov) const {
	static const float nearClipPlane = 1.0;
	static const float farClipPlane = 10000.0;

	float aspectRatio = kOriginalWidth / (float)kFrameHeight;

	float xmaxValue = nearClipPlane * tan(fov * M_PI / 360.0);
	float ymaxValue = xmaxValue / aspectRatio;

	return Math::makeFrustumMatrix(-xmaxValue, xmaxValue, -ymaxValue, ymaxValue, nearClipPlane, farClipPlane);
}

void Renderer::setupCameraPerspective(float pitch, float heading, float fov) {
	_projectionMatrix = makeProjectionMatrix(fov);
	_modelViewMatrix = Math::Matrix4(180.0f - heading, pitch, 0.0f, Math::EO_YXZ);

	Math::Matrix4 proj = _projectionMatrix;
	Math::Matrix4 model = _modelViewMatrix;
	proj.transpose();
	model.transpose();

	_mvpMatrix = proj * model;

	_frustum.setup(_mvpMatrix);

	_mvpMatrix.transpose();
}

bool Renderer::isCubeFaceVisible(uint face) {
	assert(face < 6);

	return _frustum.isInside(_cubeFacesAABB[face]);
}

void Renderer::flipVertical(Graphics::Surface *s) {
	for (int y = 0; y < s->h / 2; ++y) {
		// Flip the lines
		byte *line1P = (byte *)s->getBasePtr(0, y);
		byte *line2P = (byte *)s->getBasePtr(0, s->h - y - 1);

		for (int x = 0; x < s->pitch; ++x)
			SWAP(line1P[x], line2P[x]);
	}
}

Renderer *createRenderer(OSystem *system) {
	Common::String rendererConfig = ConfMan.get("renderer");
	Graphics::RendererType desiredRendererType = Graphics::Renderer::parseTypeCode(rendererConfig);
	Graphics::RendererType matchingRendererType = Graphics::Renderer::getBestMatchingAvailableType(desiredRendererType,
#if defined(USE_OPENGL_GAME)
																								   Graphics::kRendererTypeOpenGL |
#endif
#if defined(USE_OPENGL_SHADERS)
																									   Graphics::kRendererTypeOpenGLShaders |
#endif
#if defined(USE_TINYGL)
																									   Graphics::kRendererTypeTinyGL |
#endif
																									   0);

	bool isAccelerated = matchingRendererType != Graphics::kRendererTypeTinyGL;

	uint width;
	uint height = Renderer::kOriginalHeight;
	if (ConfMan.getBool("widescreen_mod")) {
		width = Renderer::kOriginalWidth * Renderer::kOriginalHeight / Renderer::kFrameHeight;
	} else {
		width = Renderer::kOriginalWidth;
	}

	if (isAccelerated) {
		initGraphics3d(width, height);
	} else {
		initGraphics(width, height, nullptr);
	}

#if defined(USE_OPENGL_SHADERS)
	if (matchingRendererType == Graphics::kRendererTypeOpenGLShaders) {
		return CreateGfxOpenGLShader(system);
	}
#endif
#if defined(USE_OPENGL_GAME)
	if (matchingRendererType == Graphics::kRendererTypeOpenGL) {
		return CreateGfxOpenGL(system);
	}
#endif
#if defined(USE_TINYGL)
	if (matchingRendererType == Graphics::kRendererTypeTinyGL) {
		return CreateGfxTinyGL(system);
	}
#endif
	/* We should never end up here, getBestMatchingRendererType would have failed before */
	error("Unable to create a renderer");
}

void Renderer::renderDrawable(Drawable *drawable, Window *window) {
	if (drawable->isConstrainedToWindow()) {
		selectTargetWindow(window, drawable->is3D(), drawable->isScaled());
	} else {
		selectTargetWindow(nullptr, drawable->is3D(), drawable->isScaled());
	}
	drawable->draw();
}

void Renderer::renderDrawableOverlay(Drawable *drawable, Window *window) {
	// Overlays are always 2D
	if (drawable->isConstrainedToWindow()) {
		selectTargetWindow(window, drawable->is3D(), drawable->isScaled());
	} else {
		selectTargetWindow(nullptr, drawable->is3D(), drawable->isScaled());
	}
	drawable->drawOverlay();
}

void Renderer::renderWindow(Window *window) {
	renderDrawable(window, window);
}

void Renderer::renderWindowOverlay(Window *window) {
	renderDrawableOverlay(window, window);
}

PointF Renderer::getScale() const {
	return PointF(
		_screenViewport.width() / (float)kOriginalWidth,
		_screenViewport.height() / (float)kOriginalHeight);
}

Rect Renderer::scaleRect(const Rect &rect) {
	PointF scale = getScale();
	return rect * scale;
}

// TODO: check centerOnScreen functions in other classes for refractooring opportunities
Rect Renderer::centerOnViewport(const Rect &rect, bool useFrameViewport) {
	Rect r(rect);
	Rect screenViewport = useFrameViewport ? frameViewport() : viewport();
	r.translate((screenViewport.width() - rect.width()) / 2,
				(screenViewport.height() - rect.height()) / 2);
	return r;
}

Rect Renderer::createScaledRect(int16 w, int16 h, bool centerOnViewport, bool useFrameViewport) {
	Rect rect = Rect(w, h) * getScale();
	if (centerOnViewport) {
		rect = Renderer::centerOnViewport(rect, useFrameViewport);
	}
	return rect;
}

Drawable::Drawable() : _isConstrainedToWindow(true),
					   _is3D(false),
					   _scaled(true) {
}

Point Window::getCenter() const {
	Rect frame = getPosition();
	return frame.center();
}

Point Window::screenPosToWindowPos(const Point &screen, bool clip) const {
	Rect frame = getPosition();

	Point translated = screen - frame.origin();
	if (clip) {
		translated.x = CLIP<int16>(translated.x, 0, frame.width());
		translated.y = CLIP<int16>(translated.y, 0, frame.height());
	}
	return translated;
}

Point Window::scalePoint(const Point &screen) const {
	Point windowPos = screenPosToWindowPos(screen, true);
	Rect viewport = getPosition();
	if (_scaled) {
		windowPos.x *= Renderer::kOriginalWidth / (float)viewport.width();
		windowPos.y *= Renderer::kOriginalHeight / (float)viewport.height();
	}
	return windowPos;
}

const Graphics::PixelFormat Texture::getRGBAPixelFormat() {
	return Graphics::PixelFormat::createFormatRGBA32();
}

} // End of namespace Myst3
