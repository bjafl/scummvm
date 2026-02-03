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

#ifndef GFX_OPENGL_SHADERS_H_
#define GFX_OPENGL_SHADERS_H_

#include "common/array.h"
#include "engines/myst3/rect.h"

#include "math/rect2d.h"

#include "graphics/opengl/shader.h"

#include "engines/myst3/gfx.h"

namespace Myst3 {

class Effect;
class GameState;

class ShaderRenderer : public Renderer {
public:
	ShaderRenderer(OSystem *_system);
	virtual ~ShaderRenderer();

	void init() override;
	// void setViewport(const Rect &viewport, bool is3d) override;

	void clear() override;
	void selectTargetWindow(Window *window, bool is3D, bool scaled) override;

	Texture *createTexture3D(const Graphics::Surface *surface) override;
	Texture *createTextureFromDDS(const DDS &dds) override;
	bool supportsCompressedTextures() const override;

	void drawRect2D(const RectF &screenRect, uint8 a, uint8 r, uint8 g, uint8 b) override;
	virtual void drawTexturedRect2D(const RectF &screenRect, const RectF &textureRect, Texture *texture,
	                        		float transparency = -1.0, bool additiveBlending = false) override;
	virtual void drawTexturedRect3D(const Math::Vector3d &topLeft, const Math::Vector3d &bottomLeft,
	                                const Math::Vector3d &topRight, const Math::Vector3d &bottomRight,
	                                Texture *texture) override;

	void drawCube(Texture **textures) override;
	void drawCubeWithEffects(Texture **textures, Texture **effectMasks, Texture *shieldPattern,
	                         const Common::Array<Effect *> &effects, GameState *state);
	void draw2DText(const Common::String &text, const PointF &position) override;

	Graphics::Surface *getScreenshot() override;
	Texture *copyScreenshotToTexture() override;

	/** Check if GPU-based effects are supported */
	bool supportsShaderEffects() const { return _cubeEffectsShader != nullptr; }

private:
	void setupQuadEBO();
	Math::Vector2d scaled(float x, float y) const;
	void setupEffectsShader(OpenGL::Shader &shader, uint faceId, Texture **effectMasks,
	                        Texture *shieldPattern, const Common::Array<Effect *> &effects, GameState *state);

	OpenGL::Shader *_boxShader;
	OpenGL::Shader *_cubeShader;
	OpenGL::Shader *_cubeEffectsShader;
	OpenGL::Shader *_frameEffectsShader;
	OpenGL::Shader *_rect3dShader;
	OpenGL::Shader *_textShader;

	GLuint _boxVBO;
	GLuint _cubeVBO;
	GLuint _rect3dVBO;
	GLuint _textVBO;
	GLuint _quadEBO;

	RectF _currentViewport;

	Common::String _prevText;
	PointF _prevTextPosition;
};

} // End of namespace Myst3

#endif
