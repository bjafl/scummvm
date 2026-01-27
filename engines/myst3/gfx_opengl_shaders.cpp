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

// Matrix calculations taken from the glm library
// Which is covered by the MIT license
// And has this additional copyright note:
/* Copyright (c) 2005 - 2012 G-Truc Creation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include "engines/myst3/rect.h"
#include "common/textconsole.h"

#if defined(USE_OPENGL_SHADERS)

#include "graphics/surface.h"

#include "math/glmath.h"
#include "math/vector2d.h"
#include "math/rect2d.h"
#include "math/quat.h"

#include "graphics/opengl/context.h"
#include "graphics/opengl/shader.h"

#include "engines/myst3/effects.h"
#include "engines/myst3/gfx.h"
#include "engines/myst3/gfx_opengl_texture.h"
#include "engines/myst3/gfx_opengl_shaders.h"
#include "engines/myst3/state.h"

namespace Myst3 {

Renderer *CreateGfxOpenGLShader(OSystem *system) {
	return new ShaderRenderer(system);
}

static const GLfloat boxVertices[] = {
	// XS   YT
	0.0, 0.0,
	1.0, 0.0,
	0.0, 1.0,
	1.0, 1.0,
};

void ShaderRenderer::setupQuadEBO() {
	unsigned short quadIndices[6 * 100];

	unsigned short start = 0;
	for (unsigned short *p = quadIndices; p < &quadIndices[6 * 100]; p += 6) {
		p[0] = p[3] = start++;
		p[1] = start++;
		p[2] = p[4] = start++;
		p[5] = start++;
	}

	_quadEBO = OpenGL::Shader::createBuffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
}

// Math::Vector2d ShaderRenderer::scaled(float x, float y) const {
// 	return Math::Vector2d(x / _currentViewport.width(), y / _currentViewport.height());
// }

ShaderRenderer::ShaderRenderer(OSystem *system) :
		Renderer(system),
		_prevText(""),
		_prevTextPosition(0,0),
		_currentViewport(kOriginalWidth, kOriginalHeight),
		_boxShader(nullptr),
		_cubeShader(nullptr),
		_cubeEffectsShader(nullptr),
		_frameEffectsShader(nullptr),
		_rect3dShader(nullptr),
		_textShader(nullptr),
		_boxVBO(0),
		_cubeVBO(0),
		_rect3dVBO(0),
		_textVBO(0),
		_quadEBO(0) {
}

ShaderRenderer::~ShaderRenderer() {
	OpenGL::Shader::freeBuffer(_boxVBO);
	OpenGL::Shader::freeBuffer(_cubeVBO);
	OpenGL::Shader::freeBuffer(_rect3dVBO);
	OpenGL::Shader::freeBuffer(_textVBO);
	OpenGL::Shader::freeBuffer(_quadEBO);

	delete _boxShader;
	delete _cubeShader;
	delete _cubeEffectsShader;
	delete _frameEffectsShader;
	delete _rect3dShader;
	delete _textShader;
}

// void ShaderRenderer::setViewport(const Rect &viewport, bool is3d) {
// 	int32 screenHeight = _system->getHeight();
// 	glViewport(viewport.left, screenHeight - viewport.bottom(), viewport.width(), viewport.height());
// }

Texture *ShaderRenderer::createTexture3D(const Graphics::Surface *surface) {
	return new OpenGLTexture(surface);
}

bool ShaderRenderer::supportsCompressedTextures() const {
	// Check for S3TC/DXT extension support
	return GLAD_GL_EXT_texture_compression_s3tc;
}

Texture *ShaderRenderer::createTextureFromDDS(const DDS &dds) {
	if (!supportsCompressedTextures()) {
		return nullptr;
	}

	switch (dds.dataFormat()) {
	case DDS::kDataFormatRawBC1Unorm:
		return new OpenGLTexture(dds.width(), dds.height(), GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, dds.rawData(), dds.rawDataSize());
	case DDS::kDataFormatRawBC2Unorm:
		return new OpenGLTexture(dds.width(), dds.height(), GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, dds.rawData(), dds.rawDataSize());
	case DDS::kDataFormatRawBC3Unorm:
		return new OpenGLTexture(dds.width(), dds.height(), GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, dds.rawData(), dds.rawDataSize());
	case DDS::kDataFormatRawBC7Unorm:
		// BC7 (BPTC) not supported in this OpenGL context - fall through to software decode
		return nullptr;
	case DDS::kDataFormatMipMaps:
		// Uncompressed DDS - use standard path
		return new OpenGLTexture(&dds.getMipMaps()[0]);
	default:
		return nullptr;
	}
}

void ShaderRenderer::init() {
	debug("Initializing OpenGL Renderer with shaders");

	// computeScreenViewport();

	glEnable(GL_DEPTH_TEST);

	static const char* attributes[] = { "position", "texcoord", nullptr };
	_boxShader = OpenGL::Shader::fromFiles("myst3_box", attributes);
	_boxVBO = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices);
	_boxShader->enableVertexAttribute("position", _boxVBO, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), 0);
	_boxShader->enableVertexAttribute("texcoord", _boxVBO, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), 0);

	_cubeShader = OpenGL::Shader::fromFiles("myst3_cube", attributes);
	_cubeVBO = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices);
	_cubeShader->enableVertexAttribute("texcoord", _cubeVBO, 2, GL_FLOAT, GL_TRUE, 5 * sizeof(float), 0);
	_cubeShader->enableVertexAttribute("position", _cubeVBO, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 2 * sizeof(float));

	_rect3dShader = OpenGL::Shader::fromFiles("myst3_cube", attributes);
	_rect3dVBO = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, 20 * sizeof(float), nullptr);
	_rect3dShader->enableVertexAttribute("texcoord", _rect3dVBO, 2, GL_FLOAT, GL_TRUE, 5 * sizeof(float), 0);
	_rect3dShader->enableVertexAttribute("position", _rect3dVBO, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 2 * sizeof(float));

	_textShader = OpenGL::Shader::fromFiles("myst3_text", attributes);
	_textVBO = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, 100 * 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	_textShader->enableVertexAttribute("texcoord", _textVBO, 2, GL_FLOAT, GL_TRUE, 4 * sizeof(float), 0);
	_textShader->enableVertexAttribute("position", _textVBO, 2, GL_FLOAT, GL_TRUE, 4 * sizeof(float), 2 * sizeof(float));

	// Initialize effect shaders for GPU-accelerated water/lava/magnet/shield effects
	_cubeEffectsShader = OpenGL::Shader::fromFiles("myst3_cube_effects", attributes);
	_cubeEffectsShader->enableVertexAttribute("texcoord", _cubeVBO, 2, GL_FLOAT, GL_TRUE, 5 * sizeof(float), 0);
	_cubeEffectsShader->enableVertexAttribute("position", _cubeVBO, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 2 * sizeof(float));

	_frameEffectsShader = OpenGL::Shader::fromFiles("myst3_frame_effects", attributes);
	_frameEffectsShader->enableVertexAttribute("position", _boxVBO, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), 0);
	_frameEffectsShader->enableVertexAttribute("texcoord", _boxVBO, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), 0);

	setupQuadEBO();
}

void ShaderRenderer::clear() {
	glClearColor(0.f, 0.f, 0.f, 1.f); // Solid black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ShaderRenderer::selectTargetWindow(Window *window, bool is3D, bool scaled) {
	if (!window) {
		// No window found ...
		if (scaled) {
			// ... in scaled mode draw in the original game screen area
			Rect vp = viewport();
			glViewport(vp.left, _system->getHeight() - vp.top - vp.height(), vp.width(), vp.height());
			_currentViewport = Rect(kOriginalWidth, kOriginalHeight);
		} else {
			// ... otherwise, draw on the whole screen
			glViewport(0, 0, _system->getWidth(), _system->getHeight());
			_currentViewport = Rect(_system->getWidth(), _system->getHeight());
		}
	} else {
		// Found a window, draw inside it
		Rect vp = window->getPosition();
		glViewport(vp.left, _system->getHeight() - vp.top - vp.height(), vp.width(), vp.height());

		if (scaled) {
			_currentViewport = window->getOriginalPosition();
		} else {
			_currentViewport = vp;
		}
	}
}

void ShaderRenderer::drawRect2D(const Rect &screenRect,  uint8 a, uint8 r, uint8 g, uint8 b) {
	_boxShader->use();
	_boxShader->setUniform("textured", false);
	_boxShader->setUniform("color", Math::Vector4d(r / 255.0, g / 255.0, b / 255.0, a / 255.0));
	_boxShader->setUniform("verOffsetXY", Math::Vector2d(screenRect.left, screenRect.top));
	_boxShader->setUniform("verSizeWH", Math::Vector2d(screenRect.width(), screenRect.height()));
	_boxShader->setUniform("flipY", false);

	glDepthMask(GL_FALSE);

	if (a != 255) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

void ShaderRenderer::drawTexturedRect2D(const Rect &screenRect, const Rect &textureRect, Texture *texture,
	                        			float transparency, bool additiveBlending) {
	OpenGLTexture *glTexture = static_cast<OpenGLTexture *>(texture);

	const float tLeft   = textureRect.left   * glTexture->width  / (float)glTexture->internalWidth;
	const float tWidth  = textureRect.width()  * glTexture->width  / (float)glTexture->internalWidth;
	const float tTop    = textureRect.top    * glTexture->height / (float)glTexture->internalHeight;
	const float tHeight = textureRect.height() * glTexture->height / (float)glTexture->internalHeight;

	// const float sLeft = screenRect.left;
	// const float sTop = screenRect.top;
	// const float sWidth = screenRect.width();
	// const float sHeight = screenRect.height();

	if (transparency >= 0.0) {
		if (additiveBlending) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		} else {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		glEnable(GL_BLEND);
	} else {
		transparency = 1.0;
	}

	_boxShader->use();
	_boxShader->setUniform("textured", true);
	_boxShader->setUniform("color", Math::Vector4d(1.0f, 1.0f, 1.0f, transparency));
	_boxShader->setUniform("verOffsetXY", Math::Vector2d(screenRect.left, screenRect.top));
	_boxShader->setUniform("verSizeWH", Math::Vector2d(screenRect.width(), screenRect.height()));
	_boxShader->setUniform("texOffsetXY", Math::Vector2d(tLeft, tTop));
	_boxShader->setUniform("texSizeWH", Math::Vector2d(tWidth, tHeight));
	_boxShader->setUniform("flipY", glTexture->upsideDown);

	glDepthMask(GL_FALSE);

	glBindTexture(GL_TEXTURE_2D, glTexture->id);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

void ShaderRenderer::draw2DText(const Common::String &text, const Point &position) {
	OpenGLTexture *glFont = static_cast<OpenGLTexture *>(_font);

	// The font only has uppercase letters
	Common::String textToDraw = text;
	textToDraw.toUppercase();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	if (_prevText != textToDraw || _prevTextPosition != position) {
		_prevText = textToDraw;
		_prevTextPosition = position;

		float x = position.x / (float) _currentViewport.width();
		float y = position.y / (float) _currentViewport.height();

		float *bufData = new float[16 * textToDraw.size()];
		float *cur = bufData;

		for (uint i = 0; i < textToDraw.size(); i++) {
			Rect textureRect = getFontCharacterRect(textToDraw[i]);
			float w = textureRect.width() / (float) _currentViewport.width();
			float h = textureRect.height() / (float) _currentViewport.height();

			float cw = textureRect.width() / (float)glFont->internalWidth;
			float ch = textureRect.height() / (float)glFont->internalHeight;
			float cx = textureRect.left / (float)glFont->internalWidth;
			float cy = textureRect.top / (float)glFont->internalHeight;

			const float charData[] = {
				cx,      cy + ch, x,     y,
				cx + cw, cy + ch, x + w, y,
				cx + cw, cy,      x + w, y + h,
				cx,      cy,      x,     y + h,
			};

			memcpy(cur, charData, 16 * sizeof(float));
			cur += 16;

			x += (textureRect.width() - 3) / (float) _currentViewport.width();
		}

		glBindBuffer(GL_ARRAY_BUFFER, _textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, textToDraw.size() * 16 * sizeof(float), bufData);
		delete[] bufData;
	}

	_textShader->use();
	glBindTexture(GL_TEXTURE_2D, glFont->id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	glDrawElements(GL_TRIANGLES, 6 * textToDraw.size(), GL_UNSIGNED_SHORT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

void ShaderRenderer::drawCube(Texture **textures) {
	OpenGLTexture *texture0 = static_cast<OpenGLTexture *>(textures[0]);

	glDepthMask(GL_FALSE);

	_cubeShader->use();
	_cubeShader->setUniform1f("texScale", texture0->width / (float) texture0->internalWidth);
	_cubeShader->setUniform("mvpMatrix", _mvpMatrix);

	glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[0])->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[1])->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);

	glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[2])->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);

	glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[3])->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);

	glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[4])->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);

	glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[5])->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

	glDepthMask(GL_TRUE);
}

void ShaderRenderer::setupEffectsShader(OpenGL::Shader &shader, uint faceId, Texture **effectMasks,
                                        Texture *shieldPattern, const Common::Array<Effect *> &effects, GameState *state) {
	shader.setUniform("faceId", (int)faceId);
	shader.setUniform("waterEffect", false);
	shader.setUniform("lavaEffect", false);
	shader.setUniform("magnetEffect", false);
	shader.setUniform("shieldEffect", false);

	for (uint j = 0; j < effects.size(); j++) {
		Effect *effect = effects[j];

		if (!effect->hasFace(faceId))
			continue;

		switch (effect->type()) {
		case kEffectWater: {
			OpenGLTexture *faceMaskTexture = effectMasks[faceId] ? static_cast<OpenGLTexture *>(effectMasks[faceId]) : nullptr;
			if (!faceMaskTexture) {
				break;
			}

			uint32 currentTime = g_system->getMillis();
			uint position = (currentTime * state->getWaterEffectSpeed() / state->getWaterEffectMaxStep()) % 1000;

			shader.setUniform("waterEffect", true);
			shader.setUniform1f("waterEffectPosition", position / 1000.f);
			shader.setUniform1f("waterEffectAttenuation", 1.f - state->getWaterEffectAttenuation() / 640.f);
			shader.setUniform1f("waterEffectFrequency", state->getWaterEffectFrequency() / 10.f);
			shader.setUniform1f("waterEffectAmpl", state->getWaterEffectAmpl() / 20.f);
			shader.setUniform1f("waterEffectAmplOffset", state->getWaterEffectAmplOffset() / 255.f);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, faceMaskTexture->id);
			break;
		}
		case kEffectLava: {
			OpenGLTexture *faceMaskTexture = effectMasks[faceId] ? static_cast<OpenGLTexture *>(effectMasks[faceId]) : nullptr;
			if (!faceMaskTexture) {
				break;
			}

			uint32 currentTime = g_system->getMillis();
			uint position = (currentTime * state->getLavaEffectSpeed() / 256) % 1000;

			float ampl = state->getLavaEffectAmpl() / 10.f;

			shader.setUniform("lavaEffect", true);
			shader.setUniform1f("lavaEffectPosition", position / 1000.f);
			shader.setUniform1f("lavaEffectAmpl", ampl);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, faceMaskTexture->id);
			break;
		}
		case kEffectMagnet: {
			OpenGLTexture *faceMaskTexture = effectMasks[faceId] ? static_cast<OpenGLTexture *>(effectMasks[faceId]) : nullptr;
			if (!faceMaskTexture) {
				break;
			}

			uint32 currentTime = g_system->getMillis();
			uint position = (currentTime * state->getMagnetEffectSpeed() / 10) % 1000;

			float ampl = (state->getMagnetEffectUnk1() + state->getMagnetEffectUnk3())
					/ (float)state->getMagnetEffectUnk2();

			shader.setUniform("magnetEffect", true);
			shader.setUniform1f("magnetEffectPosition", position / 1000.f);
			shader.setUniform1f("magnetEffectAmpl", ampl);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, faceMaskTexture->id);
			break;
		}
		case kEffectShield: {
			OpenGLTexture *faceMaskTexture = effectMasks[faceId] ? static_cast<OpenGLTexture *>(effectMasks[faceId]) : nullptr;
			OpenGLTexture *patternTexture = shieldPattern ? static_cast<OpenGLTexture *>(shieldPattern) : nullptr;
			if (!faceMaskTexture || !patternTexture) {
				break;
			}

			uint32 currentTime = g_system->getMillis();
			uint position = (currentTime / 4) % 1000;

			float ampl = sin((currentTime % 11520) * 2.f * (float)M_PI / 11520.f) * 1.5f + 2.5f;

			shader.setUniform("shieldEffect", true);
			shader.setUniform1f("shieldEffectPosition", position / 1000.f);
			shader.setUniform1f("shieldEffectAmpl", ampl);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, faceMaskTexture->id);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, patternTexture->id);
			break;
		}
		default:
			break;
		}
	}
}

void ShaderRenderer::drawCubeWithEffects(Texture **textures, Texture **effectMasks, Texture *shieldPattern,
                                         const Common::Array<Effect *> &effects, GameState *state) {
	OpenGLTexture *texture0 = static_cast<OpenGLTexture *>(textures[0]);

	glDepthMask(GL_FALSE);

	_cubeEffectsShader->use();
	_cubeEffectsShader->setUniform1f("texScale", texture0->width / (float) texture0->internalWidth);
	_cubeEffectsShader->setUniform("mvpMatrix", _mvpMatrix);
	_cubeEffectsShader->setUniform("texImage", 0);
	_cubeEffectsShader->setUniform("texEffect1", 1);
	_cubeEffectsShader->setUniform("texEffect2", 2);
	_cubeEffectsShader->setUniform("texEffectPattern", 3);
	_cubeEffectsShader->setUniform("frame", false);

	for (uint faceId = 0; faceId < 6; faceId++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLTexture *>(textures[faceId])->id);

		setupEffectsShader(*_cubeEffectsShader, faceId, effectMasks, shieldPattern, effects, state);

		glDrawArrays(GL_TRIANGLE_STRIP, 4 * faceId, 4);
	}

	glActiveTexture(GL_TEXTURE0);
	glDepthMask(GL_TRUE);
}

void ShaderRenderer::drawTexturedRect3D(const Math::Vector3d &topLeft, const Math::Vector3d &bottomLeft,
	                                const Math::Vector3d &topRight, const Math::Vector3d &bottomRight, Texture *texture) {
	OpenGLTexture *glTexture = static_cast<OpenGLTexture *>(texture);

	const float w = glTexture->width / (float)glTexture->internalWidth;
	const float h = glTexture->height / (float)glTexture->internalHeight;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	glBindTexture(GL_TEXTURE_2D, glTexture->id);

	const GLfloat vertices[] = {
		// S   T   X                  Y                 Z
		   0,  0,  -topLeft.x(),      topLeft.y(),      topLeft.z(),
		   0,  h,  -bottomLeft.x(),   bottomLeft.y(),   bottomLeft.z(),
		   w,  0,  -topRight.x(),     topRight.y(),     topRight.z(),
		   w,  h,  -bottomRight.x(),  bottomRight.y(),  bottomRight.z(),
	};

	_rect3dShader->use();
	_rect3dShader->setUniform1f("texScale", 1.0f);
	_rect3dShader->setUniform("mvpMatrix", _mvpMatrix);
	glBindBuffer(GL_ARRAY_BUFFER, _rect3dVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 20 * sizeof(float), vertices);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

Graphics::Surface *ShaderRenderer::getScreenshot() {
	Rect screen = viewport();

	Graphics::Surface *s = new Graphics::Surface();
	s->create(screen.width(), screen.height(), Texture::getRGBAPixelFormat());

	g_system->presentBuffer();
	glReadPixels(screen.left, screen.top, screen.width(), screen.height(), GL_RGBA, GL_UNSIGNED_BYTE, s->getPixels());

	flipVertical(s);

	return s;
}

Texture *ShaderRenderer::copyScreenshotToTexture() {
	OpenGLTexture *texture = new OpenGLTexture();

	Rect screen = viewport();
	texture->copyFromFramebuffer(screen);

	return texture;
}

} // End of namespace Myst3

#endif
